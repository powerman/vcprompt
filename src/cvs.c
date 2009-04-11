#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#include "cvs.h"

int cvs_probe(options_t* options)
{
    struct stat statbuf;
    if (stat("CVS", &statbuf) < 0) {
        debug(options, "failed to stat() 'CVS': %s", strerror(errno));
        return 0;
    }
    if (!S_ISDIR(statbuf.st_mode)) {
        debug(options, "'CVS' not a directory");
        return 0;
    }
    return 1;
}

result_t* cvs_get_info(options_t* options)
{
    result_t* result = init_result();
    FILE* tagfile;
    char buf[1024];

    tagfile = fopen("CVS/Tag", "r");
    if (tagfile == NULL) {
        debug(options, "could not read CVS/Tag (%s): assuming trunk",
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
