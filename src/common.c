#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

#include "common.h"

result_t* init_result()
{
    return (result_t*) calloc(1, sizeof(result_t));
}

void free_result(result_t* result)
{
    free(result);
}

void debug(options_t* options, char* fmt, ...)
{
    va_list args;

    if (!options->debug)
        return;

    va_start(args, fmt);
    fputs("vcprompt: debug: ", stdout);
    vfprintf(stdout, fmt, args);
    fputc('\n', stdout);
    va_end(args);
}
