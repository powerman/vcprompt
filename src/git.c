#include <stdlib.h>
#include <string.h>
#include "git.h"

int git_probe()
{
    return isdir(".git");
}

result_t* git_get_info(options_t* options)
{
    result_t* result = init_result();
    char buf[1024];

    if (!read_first_line(".git/HEAD", buf, 1024)) {
        debug("unable to read .git/HEAD: unknown branch");
        result->branch = "(unknown)";
    }
    else {
        char* prefix = "ref: refs/heads/";
        int prefixlen = strlen(prefix);

        if (strncmp(prefix, buf, prefixlen) == 0) {
            /* yep, we're on a known branch */
            debug("read a head ref from .git/HEAD: '%s'", buf);
            result->branch = strdup(buf+prefixlen); /* XXX mem leak! */
        }
        else {
            debug(".git/HEAD doesn't look like a head ref: unknown branch");
            result->branch = "(unknown)";
        }
    }

    return result;
}
