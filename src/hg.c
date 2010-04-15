#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "hg.h"

static const size_t NODEID_LEN = 20;

static int
hg_probe(vccontext_t* context)
{
    return isdir(".hg");
}

static int sum_bytes(const unsigned char* data, int size)
{
    int i, sum = 0;
    for (i = 0; i < size; ++i) {
        sum += data[i];
    }
    return sum;
}

static size_t get_mq_patchname(char* str, const char* nodeid, size_t n)
{
    char buf[1024];

    if (read_last_line(".hg/patches/status", buf, 1024)) {
        char nodeid_s[NODEID_LEN * 2 + 1], *p, *patch, *patch_nodeid_s;
        dump_hex(nodeid, nodeid_s, NODEID_LEN);

        debug("read last line from .hg/patches/status: '%s'", buf);
        p = strchr(buf, ':');
        if (!p) return 0;
        *p = '\0';
        patch_nodeid_s = buf;
        patch = p + 1;
        debug("patch name found: '%s', nodeid: %s", patch, patch_nodeid_s);

        if (strcmp(patch_nodeid_s, nodeid_s)) return 0;

        strncpy(str, patch, n);
        str[n - 1] = '\0';
        return strlen(str);
    }
    else {
        debug("failed to read from .hg/patches/status: assuming no mq patch applied");
        return 0;
    }
}

static size_t put_nodeid(char* str, const char* nodeid)
{
    const size_t SHORT_NODEID_LEN = 6;  // size in binary repr
    char buf[512], *p = str;
    size_t n;

    dump_hex(nodeid, p, SHORT_NODEID_LEN);
    p += SHORT_NODEID_LEN * 2;

    n = get_mq_patchname(buf, nodeid, sizeof(buf));
    if (n) {
        *p = '['; ++p;
        memcpy(p, buf, n); p += n;
        *p = ']'; ++p;
        *p = '\0';
    }

    return p - str;
}

static void
update_nodeid(vccontext_t* context, result_t* result)
{
    char buf[NODEID_LEN * 2];
    size_t readsize;

    if (!context->options->show_revision) return;

    readsize = read_file(".hg/dirstate", buf, NODEID_LEN * 2);
    if (readsize == NODEID_LEN * 2) {
        char destbuf[1024] = {'\0'}, *p;
        p = destbuf;
        debug("read nodeids from .hg/dirstate");

        // first parent
        if (sum_bytes((unsigned char *) buf, NODEID_LEN)) {
            p += put_nodeid(p, buf);
        }

        // second parent
        if (sum_bytes((unsigned char *) buf + NODEID_LEN, NODEID_LEN))
        {
            *p = ','; ++p;
            p += put_nodeid(p, buf + NODEID_LEN);
        }

        result->revision = strdup(destbuf);  /* XXX mem leak */
    }
    else {
        debug("failed to read from .hg/dirstate");
    }
}

static result_t*
hg_get_info(vccontext_t* context)
{
    result_t* result = init_result();
    char buf[1024];

    // prefers bookmark because it tends to be more informative
    if (read_first_line(".hg/bookmarks.current", buf, 1024) && buf[0]) {
        debug("read first line from .hg/bookmarks.current: '%s'", buf);
        result->branch = strdup(buf);  /* XXX mem leak */
    }
    else if (read_first_line(".hg/branch", buf, 1024)) {
        debug("read first line from .hg/branch: '%s'", buf);
        result->branch = strdup(buf);   /* XXX mem leak */
    }
    else {
        debug("failed to read from .hg/branch: assuming default branch");
        result->branch = "default";
    }

    update_nodeid(context, result);

    return result;
}

vccontext_t* get_hg_context(options_t* options)
{
    return init_context("hg", options, hg_probe, hg_get_info);
}
