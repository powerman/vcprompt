/*
 * Copyright (C) 2012-2013, Gregory P. Ward and contributors.
 *
 * This is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "capture.h"
#include "common.h"

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <sys/types.h>

static void
init_dynbuf(dynbuf *dbuf, int bufsize)
{
    dbuf->size = bufsize;
    dbuf->len = 0;
    dbuf->buf = malloc(bufsize); /* caller handles NULL */
    dbuf->eof = 0;
}

static ssize_t
read_dynbuf(int fd, dynbuf *dbuf)
{
    size_t avail = dbuf->size - dbuf->len;
    if (avail < 1024) {
        dbuf->size *= 2;
        dbuf->buf = realloc(dbuf->buf, dbuf->size);
        avail = dbuf->size - dbuf->len;
    }
    /* read avail-1 bytes to leave room for termininating \0 */
    ssize_t nread = read(fd, dbuf->buf + dbuf->len, avail - 1);
    if (nread < 0)
        return nread;
    else if (nread == 0) {
        dbuf->buf[dbuf->len] = '\0';
        dbuf->eof = 1;
        //debug("capture: eof on fd %d; total read = %d bytes", fd, dbuf->len);
        return 0;
    }
    //debug("capture: read %d bytes from child via fd %d", nread, fd);
    dbuf->len += nread;
    return nread;
}

capture_t *
new_capture()
{
    int bufsize = 4096;
    capture_t *result = malloc(sizeof(capture_t));
    if (result == NULL)
        goto err;
    init_dynbuf(&result->childout, bufsize);
    if (result->childout.buf == NULL)
        goto err;

    init_dynbuf(&result->childerr, bufsize);
    if (result->childerr.buf == NULL)
        goto err;

    return result;

 err:
    free_capture(result);
    return NULL;
}

void
free_capture(capture_t *result)
{
    if (result != NULL) {
        if (result->childout.buf != NULL)
            free(result->childout.buf);
        if (result->childerr.buf != NULL)
            free(result->childerr.buf);
        free(result);
    }
}

static void
print_cmd(char *const argv[])
{
    int bufsize = 100;
    char cmd[bufsize];
    int offs = 0;
    int i;

    for (i = 0; argv[i] != NULL; i++) {
        if (i > 0 && offs + 1 < bufsize) {
            cmd[offs++] = ' ';
            cmd[offs] = '\0';
        }
        int arglen = strlen(argv[i]);
        /* + 4 to leave room for " ..." */
        if (offs + arglen + 4 < bufsize) {
            strcpy(cmd+offs, argv[i]);
            offs += arglen;
        }
        else {
            strcpy(cmd+offs, "...");
            break;
        }
    }
    debug("spawning child process: %s", cmd);
}

capture_t *
capture_child(const char *file, char *const argv[])
{
    int stdout_pipe[] = {-1, -1};
    int stderr_pipe[] = {-1, -1};
    capture_t *result = NULL;
    if (pipe(stdout_pipe) < 0)
        goto err;
    if (pipe(stderr_pipe) < 0)
        goto err;

    if (debug_mode())
        print_cmd(argv);
    pid_t pid = fork();
    if (pid < 0) {
        goto err;
    }
    if (pid == 0) {             /* in the child */
        close(stdout_pipe[0]);  /* don't need the read ends of the pipes */
        close(stderr_pipe[0]);
        if (dup2(stdout_pipe[1], STDOUT_FILENO) < 0)
            _exit(1);
        if (dup2(stderr_pipe[1], STDERR_FILENO) < 0)
            _exit(1);

        execvp(file, argv);
        debug("error executing %s: %s\n", file, strerror(errno));
        _exit(127);
    }

    /* parent: don't need write ends of the pipes */
    close(stdout_pipe[1]);
    close(stderr_pipe[1]);

    result = new_capture();
    if (result == NULL)
        goto err;

    int cstdout = stdout_pipe[0];
    int cstderr = stderr_pipe[0];

    int done = 0;
    while (!done) {
        int maxfd = -1;
        fd_set child_fds;
        FD_ZERO(&child_fds);
        if (!result->childout.eof) {
            FD_SET(cstdout, &child_fds);
            maxfd = cstdout;
        }
        if (!result->childerr.eof) {
            FD_SET(cstderr, &child_fds);
            maxfd = cstderr;
        }
        int numavail = select(maxfd+1, &child_fds, NULL, NULL, NULL);
        if (numavail < 0)
            goto err;
        else if (numavail == 0) /* EOF on both pipes */
            break;

        if (FD_ISSET(cstdout, &child_fds)) {
            if (read_dynbuf(cstdout, &result->childout) < 0)
                goto err;
        }
        if (FD_ISSET(cstderr, &child_fds)) {
            if (read_dynbuf(cstderr, &result->childerr) < 0)
                goto err;
        }
        done = result->childout.eof && result->childerr.eof;
    }

    int status;
    waitpid(pid, &status, 0);
    result->status = result->signal = 0;
    if (WIFEXITED(status))
        result->status = WEXITSTATUS(status);
    else if (WIFSIGNALED(status))
        result->signal = WTERMSIG(status);

    if (result->status != 0)
        debug("child process %s exited with status %d",
              file, result->status);
    if (result->signal != 0)
        debug("child process %s killed by signal %d",
              file, result->signal);
    if (result->childerr.len > 0)
        debug("child process %s wrote to stderr:\n%s",
              file, result->childerr.buf);

    return result;
 err:
    if (stdout_pipe[0] > -1)
        close(stdout_pipe[0]);
    if (stdout_pipe[1] > -1)
        close(stdout_pipe[1]);
    if (stderr_pipe[0] > -1)
        close(stderr_pipe[0]);
    if (stderr_pipe[1] > -1)
        close(stderr_pipe[1]);
    free_capture(result);
    return NULL;
}

#if 0
int
capture_failed(capture_t *capture)
{
    return (capture == NULL || capture->status > 0 || capture->signal > 0);
}
#endif

/*
 * To build a standalone executable for testing:
 *    make src/capture
 *
 * Then to actually test, commands like this are useful:
 *    ./src/capture ls -l              # stdout only
 *    ./src/capture ls -l asdf fsda    # stderr only
 *    ./src/capture ls -l . fdsa / asdf # mix of stdout and stderr
 *    ./src/capture sh -c "echo -n foobar ; echo -n bipbop >&2; echo whee ; echo fnorb >&2"
 *    ./src/capture find /etc -type f
 * ...and so forth.
 */
#ifdef TEST_CAPTURE
#include <stdio.h>

int
main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "usage: %s prog arg...\n", argv[0]);
        return 2;
    }
    options_t options = {debug: 1};
    set_options(&options);

    capture_t *result = capture_child(argv[1], argv+1);
    int status;
    if (result == NULL) {
        perror("capture failed");
        return 1;
    }
    printf("read %ld bytes from child stdout: >%s<\n",
           result->childout.len, result->childout.buf);
    printf("read %ld bytes from child stderr: >%s<\n",
           result->childerr.len, result->childerr.buf);
    status = result->status;
    printf("child status = %d, signal =%d\n", status, result->signal);
    free_capture(result);
    return status;
}
#endif
