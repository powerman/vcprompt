/*
 * Copyright (C) 2009-2013, Gregory P. Ward and contributors.
 *
 * This is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

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
    free(result->revision);
    free(result->branch);
    free(result);
}

static options_t* _options = NULL;

void set_options(options_t* options)
{
    _options = options;
}

int debug_mode()
{
    return _options->debug;
}

int result_set_revision(result_t* result, const char *revision, int len)
{
    if (result->revision)
        free(result->revision);
    if (len == -1)
        result->revision = strdup(revision);
    else {
        result->revision = malloc(len + 1);
        if (!result->revision)
            return 0;
        strncpy(result->revision, revision, len);
        result->revision[len] = 0;
    }
    return !!result->revision;
}

int result_set_branch(result_t* result, const char *branch)
{
    if (result->branch)
        free(result->branch);
    result->branch = strdup(branch);
    return !!result->branch;
}

vccontext_t*
init_context(const char *name,
             options_t* options,
             int (*probe)(vccontext_t*),
             result_t* (*get_info)(vccontext_t*))
{
    vccontext_t* context = (vccontext_t*) calloc(1, sizeof(vccontext_t));
    context->options = options;
    context->name = name;
    context->probe = probe;
    context->get_info = get_info;
    return context;
}

void
free_context(vccontext_t* context)
{
    free(context);
}

void
debug(char* fmt, ...)
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

static int
_testmode(char* name, mode_t bits, char what[])
{
    struct stat statbuf;
    if (stat(name, &statbuf) < 0) {
        debug("failed to stat() '%s': %s", name, strerror(errno));
        return 0;
    }
    if ((statbuf.st_mode & bits) == 0) {
        debug("'%s' not a %s", name, what);
	return 0;
    }
    return 1;
}

int
isdir(char* name)
{
    return _testmode(name, S_IFDIR, "directory");
}

int
isfile(char* name)
{
    return _testmode(name, S_IFREG, "regular file");
}

int
read_first_line(char* filename, char* buf, int size)
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
    chop_newline(buf);

    return 1;
}

int
read_last_line(char* filename, char* buf, int size)
{
    FILE* file;

    file = fopen(filename, "r");
    if (file == NULL) {
        debug("error opening '%s': %s", filename, strerror(errno));
        return 0;
    }

    buf[0] = '\0';
    while (fgets(buf, size, file));
    fclose(file);

    if (!buf[0]) {
        debug("empty line read from '%s'", filename);
        return 0;
    }

    /* chop trailing newline */
    chop_newline(buf);

    return 1;
}

int
read_file(const char* filename, char* buf, int size)
{
    FILE* file;
    int readsize;

    file = fopen(filename, "r");
    if (file == NULL) {
        debug("error opening '%s': %s", filename, strerror(errno));
        return 0;
    }

    readsize = fread(buf, sizeof(char), size, file);

    fclose(file);

    return readsize;
}

void
chop_newline(char* buf)
{
    int len = strlen(buf);
    if (buf[len-1] == '\n')
        buf[len-1] = '\0';
}

void
dump_hex(const char* data, char* buf, int datasize)
{
    const char HEXSTR[16] = {'0', '1', '2', '3', '4', '5', '6', '7',
                             '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
    int i;

    for (i = 0; i < datasize; ++i) {
        buf[i * 2] = HEXSTR[(unsigned char) data[i] >> 4];
        buf[i * 2 + 1] = HEXSTR[(unsigned char) data[i] & 0x0f];
    }

    buf[i * 2] = '\0';
}

void
get_till_eol(char *dest, const char *src, int nchars)
{
    char *newline = strchr(src, '\n');
    if (newline && newline - src < nchars)
        nchars = newline - src;
    strncpy(dest, src, nchars);
    dest[nchars] = '\0';
}
