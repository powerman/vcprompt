#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "cvs.h"

int cvs_probe()
{
    return isdir("CVS");
}

result_t* cvs_get_info(options_t* options)
{
    result_t* result = init_result();
    FILE* tagfile;
    char buf[1024];

    tagfile = fopen("CVS/Tag", "r");
    if (tagfile == NULL) {
        debug("could not read CVS/Tag (%s): assuming trunk",
              strerror(errno));
        result->branch = "trunk";
        return result;
    }
    
    fgets(buf, 1024, tagfile);
    if (buf[0] == 'T') {                /* sticky tag and it's a branch tag */
        int len = strlen(buf);
        if (buf[len-1] == '\n')
            buf[len-1] = '\0';
        result->branch = strdup(buf+1);  /* XXX leak! */
    }
    else {                              /* sticky non-branch tag or date */
        result->branch = "(unknown)";
    }
    return result;
}
