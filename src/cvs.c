/*
 * Copyright (C) 2009, 2010, Gregory P. Ward and contributors.
 *
 * This is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "cvs.h"

static int
cvs_probe(vccontext_t* context)
{
    return isfile("CVS/Entries");
}

static result_t*
cvs_get_info(vccontext_t* context)
{
    result_t* result = init_result();
    char buf[1024];

    if (!read_first_line("CVS/Tag", buf, 1024)) {
        debug("unable to read CVS/Tag: assuming trunk");
        result_set_branch(result, "trunk");
    }
    else {
        debug("read first line of CVS/Tag: '%s'", buf);
        if (buf[0] == 'T') {
            /* there is a sticky tag and it's a branch tag */
            result_set_branch(result, buf + 1);
        }
        else {
            /* non-branch sticky tag or sticky date */            
           result_set_branch(result, "(unknown)");
        }
    }
    return result;
}

vccontext_t* get_cvs_context(options_t* options)
{
    return init_context("cvs", options, cvs_probe, cvs_get_info);
}
