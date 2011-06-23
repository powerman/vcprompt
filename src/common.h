/*
 * Copyright (C) 2009, 2010, Gregory P. Ward and contributors.
 *
 * This is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef VCPROMPT_H
#define VCPROMPT_H

/* What the user asked for (environment + command-line).
 */
typedef struct {
    int debug;
    char* format;                       /* e.g. "[%b%u%m]" */
    int show_branch;                    /* show current branch? */
    int show_revision;                  /* show current revision? */
    int show_unknown;                   /* show ? if unknown files? */
    int show_modified;                  /* show + if local changes? */
} options_t;

/* What we figured out by analyzing the working dir: info that
 * will be printed to stdout for the shell to incorporate into
 * the user's prompt.
 */
typedef struct {
    char* branch;                       /* name of current branch */
    char* revision;                     /* current revision */
    int unknown;                        /* any unknown files? */
    int modified;                       /* any local changes? */
} result_t;

int result_set_revision(result_t* result, const char *revision, int len);
int result_set_branch(result_t* result, const char *branch);

typedef struct vccontext_t vccontext_t;
struct vccontext_t {
    const char *name;                   /* name of the VC system */
    options_t* options;

    /* context methods */
    int (*probe)(vccontext_t*);
    result_t* (*get_info)(vccontext_t*);
};

void
set_options(options_t*);
vccontext_t*
init_context(const char *name,
             options_t* options,
             int (*probe)(vccontext_t*),
             result_t* (*get_info)(vccontext_t*));
void
free_context(vccontext_t* context);
    
result_t*
init_result();

void
free_result(result_t*);

/* printf()-style output of fmt and other args to stdout, but only if
 * debug mode is on (e.g. from the command line -d).
 */
void
debug(char* fmt, ...);

/* stat() the specified file and return true if it is a directory, false
 * if stat() failed or it is not a directory.
 */
int
isdir(char* name);

/* stat() the specified file and return true if it is a regular file,
 * false if stat() failed or it is not a regular file.
 */
int
isfile(char* name);

/* Open the specified file, read the first line (up to size-1 chars) to
 * buf, and close the file.  buf will not contain a newline.  Caller
 * must allocate at least size chars for buf.  Return 1 on successful
 * read, 0 on any errors.  Error messages will be written with debug(),
 * i.e. only visible if running in debug mode.
 */
int
read_first_line(char* filename, char* buf, int size);

/* Open the specified file, reading and discarding every line except the
 * last.  The last line is written to buf (up to size-1 chars) without
 * the newline.  Caller must allocate at least size chars for buf.
 * Return value and error handling: same as read_first_line().
 */
int
read_last_line(char* filename, char* buf, int size);

/* Open and read the specified file to buf (up to size chars).  Caller
 * must allocate at least size chars for buf.  buf is assumed to be
 * binary data, i.e. not NUL-terminated.  Return value and error
 * handling: same as read_first_line().
 */
int
read_file(const char* filename, char* buf, int size);

/* If the last char of buf is '\n', replace it with '\0', i.e. terminate
 * the string one char earlier.
 */
void
chop_newline(char* buf);

/* Encode datasize bytes of binary data to hex chars in buf.  Caller
 * must allocate at least datasize*2 + 1 chars for buf.
 */
void
dump_hex(const char* data, char* buf, int datasize);

/* Copy up to nchars chars from src to dest, stopping at the first
 * newline and terminating dest with a NUL char.  On return, it is
 * guaranteed that dest will not contain a newline and that strlen(dest)
 * <= nchars.  Caller must allocate nchars+1 chars for dest.
 */
void
get_till_eol(char *dest, const char *src, int nchars);

#endif
