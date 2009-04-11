#include <string.h>
#include "common.h"
#include "hg.h"

int hg_probe()
{
    return isdir(".hg");
}

result_t* hg_get_info(options_t* options)
{
    result_t* result = init_result();
    char buf[1024];

    if (read_first_line(".hg/branch", buf, 1024)) {
        debug("read first line from .hg/branch: '%s'", buf);
        result->branch = strdup(buf);   /* XXX mem leak */
    }
    else {
        debug("failed to read from .hg/branch: unknown branch");
        result->branch = "(unknown)";
    }

    return result;
}
