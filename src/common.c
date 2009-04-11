#include <stdlib.h>
#include <string.h>
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

static options_t* _options = NULL;

void set_options(options_t* options)
{
    _options = options;
}

void debug(char* fmt, ...)
{
    va_list args;

    if (!_options->debug)
        return;

    va_start(args, fmt);
    fputs("vcprompt: debug: ", stdout);
    vfprintf(stdout, fmt, args);
    fputc('\n', stdout);
    va_end(args);
}

int isdir(char* name)
{
    struct stat statbuf;
    if (stat(name, &statbuf) < 0) {
        debug("failed to stat() '%s': %s", name, strerror(errno));
        return 0;
    }
    if (!S_ISDIR(statbuf.st_mode)) {
        debug("'%s' not a directory", name);
        return 0;
    }
    return 1;
}

int read_first_line(char* filename, char* buf, int size)
{
    FILE* file;

    file = fopen(filename, "r");
    if (file == NULL) {
        debug("error opening '%s': %s", filename, strerror(errno));
        return 0;
    }

    int ok = (fgets(buf, size, file) != NULL);
    fclose(file);
    if (!ok) {
        debug("error or EOF reading from '%s'", filename);
        return 0;
    }

    /* chop trailing newline */
    int len = strlen(buf);
    if (buf[len-1] == '\n')
        buf[len-1] = '\0';

    return 1;
}
 
