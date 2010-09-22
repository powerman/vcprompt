/*
 * Copyright (C) 2009, 2010, Gregory P. Ward and contributors.
 *
 * This is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include "git.h"

static int
git_probe(vccontext_t* context)
{
    return isdir(".git");
}

static result_t*
git_get_info(vccontext_t* context)
{
    result_t* result = init_result();
    char buf[1024];

    if (!read_first_line(".git/HEAD", buf, 1024)) {
        debug("unable to read .git/HEAD: assuming not a git repo");
        return NULL;
    }
    else {
        char* prefix = "ref: refs/heads/";
        int prefixlen = strlen(prefix);

        if (context->options->show_branch || context->options->show_revision) {
            int found_branch = 0;
            if (strncmp(prefix, buf, prefixlen) == 0) {
                /* yep, we're on a known branch */
                debug("read a head ref from .git/HEAD: '%s'", buf);
                if (result_set_branch(result, buf + prefixlen))
                    found_branch = 1;
            }
            else {
                /* if it's not a branch name, assume it is a commit ID */
                debug(".git/HEAD doesn't look like a head ref: unknown branch");
                result_set_branch(result, "(unknown)");
                result_set_revision(result, buf, 12);
            }
            if (context->options->show_revision && found_branch) {
                char buf[1024];
                char filename[1024];
                strcat(filename, ".git/refs/heads/");
                strncat(filename, result->branch, 1000);
                if (read_first_line(filename, buf, 1024)) {
                    result_set_revision(result, buf, 12);
                }
            }
        }
        if (context->options->show_modified) {
            int status = system("git diff --no-ext-diff --quiet --exit-code");
            if (WEXITSTATUS(status) == 1)       /* files modified */
                result->modified = 1;
            /* any other outcome (including failure to fork/exec,
               failure to run git, or diff error): assume no
               modifications */
        }
        if (context->options->show_unknown) {
            int status = system("test -n \"$(git ls-files --others --exclude-standard)\"");
            if (WEXITSTATUS(status) == 0)
                result->unknown = 1;
            /* again, ignore other errors and assume no unknown files */
        }
    }

    return result;
}

vccontext_t* get_git_context(options_t* options)
{
    return init_context("git", options, git_probe, git_get_info);
}
