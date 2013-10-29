/*
 * Copyright (C) 2009-2013, Gregory P. Ward and contributors.
 *
 * This is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "../config.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#if HAVE_SQLITE3_H
# include <sqlite3.h>
#endif

#include "common.h"
#include "svn.h"

#include <ctype.h>

static int
svn_probe(vccontext_t *context)
{
    return isdir(".svn");
}

#if HAVE_SQLITE3_H
static int
svn_read_sqlite(result_t *result)
{
    int ok = 0;
    int retval;
    sqlite3 *conn = NULL;
    sqlite3_stmt *res = NULL;
    const char *tail;

    retval = sqlite3_open(".svn/wc.db", &conn);
    if (retval != SQLITE_OK) {
        debug("error opening database in .svn/wc.db");
        goto err;
    }
    // unclear when wc_id is anything other than 1
    char *sql = ("select changed_revision from nodes "
                 "where wc_id = 1 and local_relpath = ''");
    retval = sqlite3_prepare_v2(conn, sql, 1000, &res, &tail);
    if (retval != SQLITE_OK) {
        debug("error running query");
        goto err;
    }
    char *buf = malloc(1024);
    sqlite3_step(res);
    sprintf(buf, "%s", sqlite3_column_text(res, 0));
    result->revision = buf;
    ok = 1;

 err:
    if (res != NULL)
        sqlite3_finalize(res);
    if (conn != NULL)
        sqlite3_close(conn);
    return ok;
}
#else
static int
svn_read_sqlite(result_t *result)
{
    debug("vcprompt built without sqlite3 (cannot support svn >= 1.7)");
    return 0;
}
#endif

static int
svn_read_custom(FILE *fp, char line[], int size, int line_num, result_t *result)
{
    // Caller has already read line 1. Read lines 2..5, discarding 2..4.
    while (line_num <= 5) {
        if (fgets(line, size, fp) == NULL) {
            debug(".svn/entries: early EOF (line %d empty)", line_num);
            return 0;
        }
        line_num++;
    }

    // Line 5 is the URL for the working dir, now in 'line'. Should
    // use this to get the branch name (not implemented yet).

    // Lines 6 .. 10 are also uninteresting.
    while (line_num <= 11) {
        if (fgets(line, size, fp) == NULL) {
            debug(".svn/entries: early EOF (line %d empty)", line_num);
            return 0;
        }
        line_num++;
    }

    // Line 11 is the revision number we care about, now in 'line'.
    chop_newline(line);
    result->revision = strdup(line);
    debug("read svn revision from .svn/entries: '%s'", line);
    return 1;
}

static int
svn_read_xml(FILE *fp, char line[], int size, int line_num, result_t *result)
{
    char rev[100];
    char *marker = "revision=";
    char *p = NULL;
    while (fgets(line, size, fp)) {
        if ((p = strstr(line, marker)) != NULL) {
            break;
        }
    }
    if (p == NULL) {
        debug("no 'revision=' line found in .svn/entries");
        return 0;
    }
    if (sscanf(p, " %*[^\"]\"%[0-9]\"", rev) == 1) {
        result_set_revision(result, rev, -1);
        debug("read svn revision from .svn/entries: '%s'", rev);
    }
    return 1;
}

static result_t*
svn_get_info(vccontext_t *context)
{
    result_t *result = init_result();
    FILE *fp = NULL;

    fp = fopen(".svn/entries", "r");
    if (!fp) {
        debug("failed to open .svn/entries: not an svn working copy");
        goto err;
    }
    char line[1024];
    int line_num = 1;                   // the line we're about to read

    if (fgets(line, sizeof(line), fp) == NULL) {
        debug(".svn/entries: empty file");
        goto err;
    }
    line_num++;

    int ok;
    if (access(".svn/wc.db", F_OK) == 0) {
        // SQLite file format (working copy created by svn >= 1.7)
        ok = svn_read_sqlite(result);
    }
    // First line of the file tells us what the format is.
    else if (isdigit(line[0])) {
        // Custom file format (working copy created by svn >= 1.4)
        ok = svn_read_custom(fp, line, sizeof(line), line_num, result);
    }
    else {
        // XML file format (working copy created by svn < 1.4)
        ok = svn_read_xml(fp, line, sizeof(line), line_num, result);
    }
    if (ok) {
        fclose(fp);
        return result;
    }

 err:
    free(result);
    if (fp)
        fclose(fp);
    return NULL;
}

vccontext_t*
get_svn_context(options_t *options)
{
    return init_context("svn", options, svn_probe, svn_get_info);
}
