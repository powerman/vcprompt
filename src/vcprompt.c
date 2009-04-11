#include <stdio.h>

#include "common.h"
#include "cvs.h"
/*
#include "svn.h"
#include "git.h"
#include "hg.h"
#include "bzr.h"
*/




void print_result(options_t* options, result_t* result)
{
    if (options->branch)
        fputs(result->branch, stdout);
    if (options->unknown && result->unknown)
        putc('?', stdout);
    if (options->modified && result->modified)
        putc('!', stdout);
}

int main(int argc, char** argv)
{
    options_t options = { 1,            /* debug */
                          1,            /* show branch */
                          0,            /* show unknown */
                          0,            /* show local changes */
    };
#if 0
    result_t result = { NULL,           /* current branch */
                        0,              /* unknown files? */
                        0,              /* local changes? */
    };
#endif
    result_t* result = NULL;

    set_options(&options);
    if (cvs_probe(&options))
        result = cvs_get_info(&options);
    /*
    else if (svn_probe())
        svn_get_info(options, &result);
    else if (git_probe())
        git_get_info(options, &result);
    else if (hg_probe())
        hg_get_info(options, &result);
    else if (bzr_probe())
        bzr_get_info(options, &result);
    */

    if (result != NULL) {
        print_result(&options, result);
        free_result(result);
    }
    if (options.debug)
        putc('\n', stdout);
    return 0;
}
