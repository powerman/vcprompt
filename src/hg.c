/*
 * Copyright (C) 2009-2013, Gregory P. Ward and contributors.
 *
 * This is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#if defined __BEOS__ && !defined __HAIKU__
#include <ByteOrder.h>
#else
#include <arpa/inet.h>
#endif

#include "capture.h"
#include "common.h"
#include "hg.h"

#define NODEID_LEN 20

static int
hg_probe(vccontext_t *context)
{
    return isdir(".hg");
}

/* return true if data contains any non-zero bytes */
static int
non_zero(const unsigned char *data, int size)
{
    int i;
    for (i = 0; i < size; i++) {
       if (data[i] != 0)
           return 1;
    }
    return 0;
}

static int
is_revlog_inlined(FILE *rlfile)
{
    const unsigned int REVLOGNGINLINEDATA = 1 << 16;
    int revlog_ver;
    size_t origpos, rlen;

    origpos = ftell(rlfile);
    rlen = fread(&revlog_ver, sizeof(revlog_ver), 1, rlfile);
    fseek(rlfile, origpos, SEEK_SET);

    revlog_ver = ntohl(revlog_ver);
    return (rlen == 1) ? revlog_ver & REVLOGNGINLINEDATA : 0;
}

typedef struct {
    char nodeid[NODEID_LEN];
    int rev;
    int istip;
} csinfo_t;

//! get changeset info for the specified nodeid
static csinfo_t
get_csinfo(const char *nodeid)
{
    // only supports RevlogNG. See mercurial/parsers.c for details.
    const char *REVLOG_FILENAME = ".hg/store/00changelog.i";
    const size_t ENTRY_LEN = 64, COMP_LEN_OFS = 8, NODEID_OFS = 32;

    char buf[ENTRY_LEN];
    FILE *rlfile;
    int inlined;
    csinfo_t csinfo = {"", -1, 0};
    int i;

    rlfile = fopen(REVLOG_FILENAME, "rb");
    if (!rlfile) {
        debug("error opening '%s': %s", REVLOG_FILENAME, strerror(errno));
        return csinfo;
    }

    inlined = is_revlog_inlined(rlfile);

    for (i = 0; !feof(rlfile); ++i) {
        size_t comp_len, rlen;

        rlen = fread(buf, 1, ENTRY_LEN, rlfile);
        if (rlen == 0) break;
        if (rlen != ENTRY_LEN) {
            debug("error while reading '%s': incomplete entry (read = %d)",
                  REVLOG_FILENAME, rlen);
            break;
        }

        // already found node but it's not the last one
        if (csinfo.rev >= 0) {
            csinfo.istip = 0;
            break;
        }

        comp_len = ntohl(*((uint32_t *) (buf + COMP_LEN_OFS)));
        if (memcmp(nodeid, buf + NODEID_OFS, NODEID_LEN) == 0) {
            memcpy(csinfo.nodeid, buf + NODEID_OFS, NODEID_LEN);
            csinfo.rev = i;  // FIXME
            csinfo.istip = 1;
        }

        if (inlined) fseek(rlfile, comp_len, SEEK_CUR);
    }

    fclose(rlfile);
    return csinfo;
}

static size_t
put_nodeid(char *dest, const char *nodeid)
{
    const size_t SHORT_NODEID_LEN = 6;  // size in binary repr
    char *p = dest;

    csinfo_t csinfo = get_csinfo(nodeid);
    if (csinfo.rev >= 0) {
        p += sprintf(p, "%d", csinfo.rev);
    }
    else {
        dump_hex(p, nodeid, SHORT_NODEID_LEN);
        p += SHORT_NODEID_LEN * 2;
    }
    return p - dest;
}

static void
read_parents(vccontext_t *context, result_t *result)
{
    if (!context->options->show_revision && !context->options->show_patch)
        return;

    char *parent_nodes;         /* two binary changeset IDs */
    size_t readsize;

    parent_nodes = malloc(NODEID_LEN * 2);
    if (!parent_nodes) {
        debug("malloc failed: out of memory");
        return;
    }
    result->full_revision = parent_nodes;

    debug("reading first %d bytes of dirstate to parent_nodes (%p)",
          NODEID_LEN * 2, parent_nodes);
    readsize = read_file(".hg/dirstate", parent_nodes, NODEID_LEN * 2);
    if (readsize != NODEID_LEN * 2) {
        return;
    }

    readsize = read_file(".hg/dirstate", parent_nodes, NODEID_LEN * 2);
    char destbuf[1024] = {'\0'};
    char *p = destbuf;

    // first parent
    if (non_zero((unsigned char *) parent_nodes, NODEID_LEN)) {
        p += put_nodeid(p, parent_nodes);
    }

    // second parent
    if (non_zero((unsigned char *) parent_nodes + NODEID_LEN, NODEID_LEN)) {
        *p++ = ',';
        p += put_nodeid(p, parent_nodes + NODEID_LEN);
    }

    result_set_revision(result, destbuf, -1);
}

static void
read_patch_name(vccontext_t *context, result_t *result)
{
    if (!context->options->show_patch)
        return;

    static const char default_status[] = ".hg/patches/status";
    static const char status_fmt[] = ".hg/patches-%s/status";

    struct stat statbuf;
    char *status_fn = NULL;
    char *last_line = NULL;

    if (stat(".hg/patches.queues", &statbuf) == 0) {
        /* The name of the current patch queue cannot possibly be
           longer than the name of all patch queues concatenated. */
        size_t max_qname = (size_t) statbuf.st_size;
        char *qname = malloc(max_qname + 1);
        int ok = read_first_line(".hg/patches.queue", qname, max_qname);
        if (ok && strlen(qname) > 0) {
            debug("read queue name from .hg/patches.queue: '%s'", qname);
            status_fn = malloc(strlen(default_status) + 1 + strlen(qname) + 1);
            sprintf(status_fn, status_fmt, qname);
        }
        free(qname);
    }

    /* Failed to read patches.queues and/or patches.queue: assume
       there is just a single patch queue. */
    if (status_fn == NULL) {
        status_fn = strdup(default_status);
    }

    if (stat(status_fn, &statbuf) < 0) {
        debug("failed to stat %s: assuming no patch applied", status_fn);
        goto done;
    }
    if (statbuf.st_size == 0) {
        debug("status file %s is empty: no patch applied", status_fn);
        goto done;
    }

    /* Last line of the file cannot possibly be longer than the whole
       file */
    size_t max_line = (size_t) statbuf.st_size;
    last_line = malloc(max_line + 1);
    if (!read_last_line(status_fn, last_line, max_line + 1)) {
        debug("failed to read from %s: assuming no mq patch applied", status_fn);
        goto done;
    }
    debug("read last line from %s: '%s'", status_fn, last_line);

    char nodeid_s[NODEID_LEN * 2 + 1];
    dump_hex(nodeid_s, result->full_revision, NODEID_LEN);

    if (strncmp(nodeid_s, last_line, NODEID_LEN * 2) == 0) {
        result->patch = strdup(last_line + NODEID_LEN * 2 + 1);
    }

 done:
    free(status_fn);
    free(last_line);
}

static void
read_modified_unknown(vccontext_t *context, result_t *result)
{
    // No easy way that we know to get the modified or unknown status
    // without forking an hg process. Replace this with a more efficient version
    // if you ever figure it out.
    if (!context->options->show_modified && !context->options->show_unknown)
        return;

    char *argv[] = {"hg", "--quiet", "status",
                    "--modified", "--added", "--removed",
                    "--unknown", NULL};
    if (!context->options->show_unknown) {
        // asking hg to search for unknown files can be expensive, so
        // skip it unless the user wants it
        argv[6] = NULL;
    }
    capture_t *capture = capture_child("hg", argv);
    if (capture == NULL) {
        debug("unable to execute 'hg status'");
        return;
    }
    char *cstdout = capture->childout.buf;
    for (char *ch = cstdout; *ch != 0; ch++) {
        if (ch == cstdout || *(ch-1) == '\n') {
            // at start of output or start of line: look for ?, M, etc.
            if (context->options->show_unknown && *ch == '?') {
                result->unknown = 1;
            }
            if (context->options->show_modified &&
                (*ch == 'M' || *ch == 'A' || *ch == 'R')) {
                result->modified = 1;
            }
        }
    }

    cstdout = NULL;
    free_capture(capture);
}

static result_t*
hg_get_info(vccontext_t *context)
{
    result_t *result = init_result();
    char buf[1024];

    // prefer bookmark because it tends to be more informative
    if (read_first_line(".hg/bookmarks.current", buf, 1024) && buf[0]) {
        debug("read first line from .hg/bookmarks.current: '%s'", buf);
        result_set_branch(result, buf);
    }
    else if (read_first_line(".hg/branch", buf, 1024)) {
        debug("read first line from .hg/branch: '%s'", buf);
        result_set_branch(result, buf);
    }
    else {
        debug("failed to read from .hg/branch: assuming default branch");
        result_set_branch(result, "default");
    }

    read_parents(context, result);
    read_patch_name(context, result);
    read_modified_unknown(context, result);

    return result;
}

vccontext_t*
get_hg_context(options_t *options)
{
    return init_context("hg", options, hg_probe, hg_get_info);
}
