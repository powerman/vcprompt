#include <string.h>
#include "common.h"
#include "hg.h"

static int
hg_probe(vccontext_t* context)
{
    return isdir(".hg");
}

static result_t*
hg_get_info(vccontext_t* context)
{
    result_t* result = init_result();
    char buf[1024];

    if (read_first_line(".hg/branch", buf, 1024)) {
        debug("read first line from .hg/branch: '%s'", buf);
        result->branch = strdup(buf);   /* XXX mem leak */
    }
    else {
        debug("failed to read from .hg/branch: assuming not an hg repo");
        return NULL;
    }

    return result;
}

vccontext_t* get_hg_context(options_t* options)
{
    return init_context(options, hg_probe, hg_get_info);
}
