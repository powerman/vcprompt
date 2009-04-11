#include <stdio.h>

#include "common.h"
#include "cvs.h"
#include "git.h"
#include "hg.h"
/*
#include "svn.h"
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

    set_options(&options);

    vccontext_t* contexts[] = {
        get_cvs_context(&options),
        get_git_context(&options),
        get_hg_context(&options),
    };
    int num_contexts = sizeof(contexts) / sizeof(vccontext_t*);

    result_t* result = NULL;
    int i;
    for (i = 0; i < num_contexts; i++) {
        vccontext_t* context = contexts[i];
        if (context->probe(context))
            result = context->get_info(context);
    }
    if (result != NULL) {
        print_result(&options, result);
        free_result(result);
    }
    if (options.debug)
        putc('\n', stdout);
    return 0;
}
