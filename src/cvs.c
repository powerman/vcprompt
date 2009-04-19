#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "cvs.h"

static int
cvs_probe(vccontext_t* context)
{
    return isdir("CVS");
}

static result_t*
cvs_get_info(vccontext_t* context)
{
    result_t* result = init_result();
    char buf[1024];

    if (!read_first_line("CVS/Tag", buf, 1024)) {
        debug("unable to read CVS/Tag: assuming trunk");
        result->branch = "trunk";
    }
    else {
        debug("read first line of CVS/Tag: '%s'", buf);
        if (buf[0] == 'T') {
            /* there is a sticky tag and it's a branch tag */
            result->branch = strdup(buf+1); /* XXX mem leak! */
        }
        else {
            /* non-branch sticky tag or sticky date */            
            result->branch = "(unknown)";
        }
    }
    return result;
}

vccontext_t* get_cvs_context(options_t* options)
{
    return init_context("cvs", options, cvs_probe, cvs_get_info);
}
