/*
 * Copyright (C) 2011 Jan Spakula and contributors.
 *
 * This is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>

#include "fossil.h"
#include "common.h"
#include "capture.h"

static int
fossil_probe(vccontext_t *context)
{
    return isfile("_FOSSIL_") || isfile(".fslckout");
}

static result_t*
fossil_get_info(vccontext_t *context)
{
    result_t *result = init_result();
    char *t;
    int tab_len = 14;
    char buf2[81];

    // Since fossil stores info in SQLite databases, we're going to read
    // the output of 'fossil status' command and analyze it.  We need
    // enough to cover all the usual fields (note that 'comment:' can be
    // several lines long) plus eventual output indicating changes in
    // the repo.
    char *argv[] = {"fossil", "status", NULL};
    capture_t *capture = capture_child("fossil", argv);
    if (capture == NULL) {
        debug("unable to execute 'fossil status'");
        return NULL;
    }
    char *cstdout = capture->childout.buf;

    if (context->options->show_branch) {
        if ((t = strstr(cstdout, "\ntags:"))) {
            // This in fact shows also other tags than just the
            // propagating ones (=branches).  So either we show all
            // of them (as now), or we can show only the first one
            // (which should be the branch name); or we use one more
            // child process to read the output of 'fossil branch'.
            get_till_eol(buf2, t + tab_len + 1, 80);
            debug("found tag line: '%s'", buf2);
            result_set_branch(result, buf2);
        }
        else {
            debug("tag line not found in fossil output; unknown branch");
            result_set_branch(result, "(unknown)");
        }
    }
    if (context->options->show_revision) {
        if ((t = strstr(cstdout, "\ncheckout:"))) {
            get_till_eol(buf2, t + tab_len + 1, 80);
            debug("found revision line: '%s'", buf2);
            result_set_revision(result, buf2, 12);
        }
        else {
            debug("revision line not found in fossil output; unknown revision");
            result_set_revision(result, "unknown", 7);
        }
    }
    if (context->options->show_modified) {
        // This can be also done by checking if 'fossil changes'
        // prints anything, but we save a child process this way.
        result->modified = (strstr(cstdout, "\nEDITED") ||
                            strstr(cstdout, "\nADDED") ||
                            strstr(cstdout, "\nDELETED") ||
                            strstr(cstdout, "\nMISSING") ||
                            strstr(cstdout, "\nRENAMED") ||
                            strstr(cstdout, "\nNOT_A_FILE") ||
                            strstr(cstdout, "\nUPDATED") ||
                            strstr(cstdout, "\nMERGED"));
    }

    cstdout = NULL;
    free_capture(capture);

    if (context->options->show_unknown) {
        // This can't be read from 'fossil status' output
        char *argv[] = {"fossil", "extra", NULL};
        capture = capture_child("fossil", argv);
        if (capture == NULL) {
            debug("unable to execute 'fossil extra'");
            return NULL;
        }
        result->unknown = (capture->childout.len > 0);
        free_capture(capture);
    }

    return result;
}

vccontext_t*
get_fossil_context(options_t *options)
{
    return init_context("fossil", options, fossil_probe, fossil_get_info);
}
