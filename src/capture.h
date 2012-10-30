#ifndef CAPTURE_H
#define CAPTURE_H

#include <sys/types.h>

typedef struct {
    size_t size;                /* bytes allocated */
    size_t len;                 /* bytes filled */
    char *buf;
    int eof;
} dynbuf;

typedef struct {
    dynbuf stdout;
    dynbuf stderr;
    int status;
} capture_t;

/* fork() and exec() a child process, capturing its entire stdout and
 * stderr to a capture object. capture->stdout.buf is the child's
 * stdout, and capture->stdout.len the number of bytes read (buf is
 * null terminated, so as long as the child's output is textual, you
 * can use buf as a string). Similarly, child's stderr is in
 * capture->stderr.buf and capture->stderr.len.
 */
capture_t *
capture_child(const char *file, char *const argv[]);

/* free all resources in the object returned by capture_child() */
void
free_capture(capture_t *capture);

#endif
