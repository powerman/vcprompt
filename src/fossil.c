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

static int
fossil_probe(vccontext_t* context)
{
    return isfile("_FOSSIL_");
}

char *get_till_eol(char *dest, const char *src, int n) {
    char *last = strchr(src, '\n');
    if (last) {
        int m = (last-src < n ? last-src : n);
        strncpy(dest, src, m);
        dest[m]='\0';
        return dest;
    }
    else {
        return NULL;
    }
}

static result_t*
fossil_get_info(vccontext_t* context)
{
    result_t* result = init_result();
    char buf[2048];
    char *t;
    FILE *stream;
    int tab_len = 14;
    char buf2[81];

    /* Since fossil stores info in sqlite databases,
     * we're going to read the output of 'fossil status'
     * command and analyze it.
     * We need enough to cover all the usual fields
     * (note that 'comment:' can be several lines long)
     * plus eventual output indicating changes in the repo.
     */
    if (!(stream = popen("fossil status","r"))) {
        debug("Unable to read output of 'fossil status'");
        return NULL;
    }
    else {
        fread(buf,sizeof(char),2048,stream);
        pclose(stream);

        if (context->options->show_branch) {
            if ((t = strstr(buf,"\ntags:"))) {
                /* This in fact shows also other tags than
                 * just the propagating ones (=branches).
                 * So either we show all of them (as now),
                 * or we can show only the first one (which
                 * should be the branch name); or we use
                 * one more 'system' call to read the output
                 * of 'fossil branch'.
                 */
                get_till_eol(buf2,t+tab_len+1,80);
                debug("found tag line: '%s'", buf2);
                result_set_branch(result, buf2);
            }
            else {
                debug("tag line not found in fossil output; unknown branch");
                result_set_branch(result, "(unknown)");
            }
        }
        if (context->options->show_revision) {
            if ((t = strstr(buf,"\ncheckout:"))) {
                get_till_eol(buf2,t+tab_len+1,80);
                debug("found revision line: '%s'", buf2);
                result_set_revision(result, buf2, 12);
            }
            else {
                debug("revision line not found in fossil output; unknown revision");
                result_set_revision(result, "unknown", 7);
            }
        }
        if (context->options->show_modified) {
            /* This can be also done by 'test -n'ing 'fossil changes',
             * but we save a system call this way.
             */
            if ( strstr(buf,"\nEDITED") || strstr(buf,"\nADDED")
                || strstr(buf,"\nDELETED") || strstr(buf,"\nMISSING")
                || strstr(buf,"\nRENAMED") || strstr(buf,"\nNOT_A_FILE")
                || strstr(buf,"\nUPDATED") || strstr(buf,"\nMERGED") )
                result->modified = 1;
        }
        if (context->options->show_unknown) {
            /* This can't be read from 'fossil status' output */
            int status = system("test -n \"$(fossil extra)\"");
            if (WEXITSTATUS(status) == 0)
                result->unknown = 1;
        }
    }

    return result;
}

vccontext_t* get_fossil_context(options_t* options)
{
    return init_context("fossil", options, fossil_probe, fossil_get_info);
}
