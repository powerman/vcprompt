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

typedef struct {
    int debug;
    char* format;                       /* e.g. "[%b%u%m]" */
    int show_branch;                    /* show current branch? */
    int show_revision;                  /* show current revision? */
    int show_unknown;                   /* show ? if unknown files? */
    int show_modified;                  /* show ! if local changes? */
} options_t;

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

void set_options(options_t*);
vccontext_t* init_context(const char *name,
                          options_t* options,
                          int (*probe)(vccontext_t*),
                          result_t* (*get_info)(vccontext_t*));
void free_context(vccontext_t* context);
    
result_t* init_result();
void free_result(result_t*);

void debug(char*, ...);

int isdir(char*);
int isfile(char*);
int read_first_line(char*, char*, int);
int read_last_line(char*, char*, int);
int read_file(const char*, char*, int);
void chop_newline(char*);

void dump_hex(const char* data, char* buf, int datasize);

/* Copy up to n chars from src to dest, stopping at the first newline
 * and terminating dest with a NUL char.  It is guaranteed that dest
 * will not contain a newline and that strlen(dest) <= n.  If src
 * contains no newlines, dest will be an empty string.  Caller must
 * allocate n+1 chars for dest.
 */
void
get_till_eol(char *dest, const char *src, int n);

#endif
