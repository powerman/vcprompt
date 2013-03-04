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

#if defined __BEOS__ && !defined __HAIKU__
#include <ByteOrder.h>
#else
#include <arpa/inet.h>
#endif

#include "common.h"
#include "hg.h"

#define NODEID_LEN 20

static int
hg_probe(vccontext_t *context)
{
    return isdir(".hg");
}

static int
sum_bytes(const unsigned char *data, int size)
{
    int i, sum = 0;
    for (i = 0; i < size; ++i) {
        sum += data[i];
    }
    return sum;
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
get_mq_patchname(char *dest, const char *nodeid, size_t n)
{
    char buf[1024];
    char status_filename[512] = ".hg/patches/status";
    static const char QQ_STATUS_FILE_PAT[] = ".hg/patches-%s/status";
    static const size_t MAX_QQ_NAME = sizeof(status_filename)
        - (sizeof(QQ_STATUS_FILE_PAT) - 2 - 1);  // - "%s" - '\0'

    // multiple patch queues, introduced in Mercurial 1.6
    if (read_first_line(".hg/patches.queue", buf, MAX_QQ_NAME) && buf[0]) {
        debug("read first line from .hg/patches.queue: '%s'", buf);
        sprintf(status_filename, QQ_STATUS_FILE_PAT, buf);
    }

    if (read_last_line(status_filename, buf, 1024)) {
        char nodeid_s[NODEID_LEN * 2 + 1], *p, *patch, *patch_nodeid_s;
        dump_hex(nodeid, nodeid_s, NODEID_LEN);

        debug("read last line from %s: '%s'", status_filename, buf);
        p = strchr(buf, ':');
        if (!p)
            return 0;
        *p = '\0';
        patch_nodeid_s = buf;
        patch = p + 1;
        debug("patch name found: '%s', nodeid: %s", patch, patch_nodeid_s);

        if (strcmp(patch_nodeid_s, nodeid_s))
            return 0;

        strncpy(dest, patch, n);
        dest[n - 1] = '\0';
        return strlen(dest);
    }
    else {
        debug("failed to read from .hg/patches/status: assuming no mq patch applied");
        return 0;
    }
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
        dump_hex(nodeid, p, SHORT_NODEID_LEN);
        p += SHORT_NODEID_LEN * 2;
    }
    return p - dest;
}

static void
read_parents(vccontext_t *context, result_t *result)
{
    char buf[NODEID_LEN * 2];
    size_t readsize;

    if (!context->options->show_revision)
        return;

    readsize = read_file(".hg/dirstate", buf, NODEID_LEN * 2);
    if (readsize == NODEID_LEN * 2) {
        char destbuf[1024] = {'\0'};
        char *p = destbuf;
        debug("read nodeids from .hg/dirstate");

        // first parent
        if (sum_bytes((unsigned char *) buf, NODEID_LEN)) {
            p += put_nodeid(p, buf);
        }

        // second parent
        if (sum_bytes((unsigned char *) buf + NODEID_LEN, NODEID_LEN)) {
            *p = ','; ++p;
            p += put_nodeid(p, buf + NODEID_LEN);
        }

        result_set_revision(result, destbuf, -1);
    }
    else {
        debug("failed to read from .hg/dirstate");
    }
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

    return result;
}

vccontext_t*
get_hg_context(options_t *options)
{
    return init_context("hg", options, hg_probe, hg_get_info);
}
