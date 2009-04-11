#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

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

int isdir(options_t* options, char* name)
{
    struct stat statbuf;
    if (stat(name, &statbuf) < 0) {
        debug(options, "failed to stat() '%s': %s", name, strerror(errno));
        return 0;
    }
    if (!S_ISDIR(statbuf.st_mode)) {
        debug(options, "'%s' not a directory", name);
        return 0;
    }
    return 1;
}
