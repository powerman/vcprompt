#include <stdio.h>
#include <string.h>

#include "common.h"
#include "cvs.h"
#include "git.h"
#include "hg.h"
/*
#include "svn.h"
#include "bzr.h"
*/


void parse_format(options_t* options)
{
    int i;

    options->show_branch = 0;
    options->show_unknown = 0;
    options->show_modified = 0;

    char* format = options->format;
    for (i = 0; i < strlen(format); i++) {
        if (format[i] == '%') {
            i++;
            switch (format[i]) {
                case '\0':              /* at end of string: ignore */
                    break;
                case 'b':
                    options->show_branch = 1;
                    break;
                case 'u':
                    options->show_unknown = 1;
                    break;
                case 'm':
                    options->show_modified = 1;
                    break;
                case '%':
                    break;
                default:
                    fprintf(stderr,
                            "error: invalid format string: %%%c\n",
                            format[i]);
                    break;
            }
        }
    }
}

void print_result(options_t* options, result_t* result)
{
    int i;
    char* format = options->format;

    for (i = 0; i < strlen(format); i++) {
        if (format[i] == '%') {
            i++;
            switch (format[i]) {
                case '0':               /* end of string */
                case '%':               /* escaped % */
                    putc('%', stdout);
                    break;
                case 'b':
                    fputs(result->branch, stdout);
                    break;
                case 'u':
                    if (result->unknown)
                        putc('?', stdout);
                    break;
                case 'm':
                    if (result->modified)
                        putc('!', stdout);
                    break;
                default:                /* %x printed as x */
                    putc(format[i], stdout);
            }
        }
        else {
            putc(format[i], stdout);
        }
    }
}

int main(int argc, char** argv)
{
    options_t options = { 0,            /* debug */
                          "[%b%m%u] ",  /* format string */
                          0,            /* show branch */
                          0,            /* show unknown */
                          0,            /* show local changes */
    };

    parse_format(&options);
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
