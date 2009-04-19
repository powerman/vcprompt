#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "common.h"
#include "cvs.h"
#include "git.h"
#include "hg.h"
/*
#include "svn.h"
#include "bzr.h"
*/

void parse_args(int argc, char** argv, options_t* options)
{
    int opt;
    while ((opt = getopt(argc, argv, "f:d")) != -1) {
        switch (opt) {
            case 'f':
                options->format = optarg;
                break;
            case 'd':
                options->debug = 1;
                break;
        }
    }
}

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
                case 'n':               /* name of VC system */
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

void print_result(vccontext_t* context, options_t* options, result_t* result)
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
                case 'n':
                    fputs(context->name, stdout);
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

vccontext_t* probe_all(vccontext_t** contexts, int num_contexts)
{
    int idx;
    for (idx = 0; idx < num_contexts; idx++) {
        vccontext_t* ctx = contexts[idx];
        if (ctx->probe(ctx)) {
            return ctx;
        }
    }
    return NULL;
}

/* walk up the directory tree until the probes work or we hit / */
vccontext_t* probe_parents(vccontext_t** contexts, int num_contexts)
{
    vccontext_t* context;
    struct stat rootdir;
    struct stat curdir;

    debug("no context claimed current dir: walking up the tree");
    stat("/", &rootdir);
    while (1) {
        context = probe_all(contexts, num_contexts);
        if (context != NULL) {
            debug("found a context");
            return context;
        }

        stat(".", &curdir);
        int isroot = (rootdir.st_dev == curdir.st_dev &&
                      rootdir.st_ino == curdir.st_ino);
        if (isroot) {
            return NULL;
        }
        chdir("..");
    }
}

int main(int argc, char** argv)
{
    options_t options = { 0,            /* debug */
                          "[%n:%b%m%u] ",  /* format string */
                          0,            /* show branch */
                          0,            /* show unknown */
                          0,            /* show local changes */
    };

    parse_args(argc, argv, &options);
    parse_format(&options);
    set_options(&options);

    vccontext_t* contexts[] = {
        get_cvs_context(&options),
        get_git_context(&options),
        get_hg_context(&options),
    };
    int num_contexts = sizeof(contexts) / sizeof(vccontext_t*);

    result_t* result = NULL;
    vccontext_t* context = NULL;

    /* Starting in the current dir, walk up the directory tree until
       someone claims that this is a working copy. */
    context = probe_parents(contexts, num_contexts);

    /* Nobody claimed it: bail now without printing anything. */
    if (context == NULL) {
        return 0;
    }

    /* Analyze the working copy metadata and print the result. */
    result = context->get_info(context);
    if (result != NULL) {
        print_result(context, &options, result);
        free_result(result);
        if (options.debug)
            putc('\n', stdout);
    }
    return 0;
}
