#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "common.h"
#include "svn.h"

#include <ctype.h>

static int
svn_probe(vccontext_t* context)
{
    return isdir(".svn");
}

static result_t*
svn_get_info(vccontext_t* context)
{
    result_t* result = init_result();
    char buf[1024];

    if (!read_first_line(".svn/entries", buf, 1024)) {
        debug("failed to read from .svn/entries: assuming not an svn repo");
        return NULL;
    }
    else {
        FILE *fp;
        fp = fopen(".svn/entries", "r");
        char line[1024];

        // Check the version
        if (fgets(line, sizeof(line), fp)) {
            if(isdigit(line[0])) {
                // Custom file format (working copy created by svn >= 1.4)

                // Read and discard line 2 (name), 3 (entries kind)
                if (fgets(line, sizeof(line), fp) == NULL ||
                    fgets(line, sizeof(line), fp) == NULL) {
                    debug("early EOF reading .svn/entries");
                    fclose(fp);
                    return NULL;
                }

                // Get the revision number
                if (fgets(line, sizeof(line), fp)) {
                    chop_newline(line);
                    result->revision = strdup(line);
                    debug("read a svn revision from .svn/entries: '%s'", line);
                }
                else {
                    debug("early EOF: expected revision number");
                    fclose(fp);
                    return NULL;
                }
            }
            else {
                // XML file format (working copy created by svn < 1.4)
                char rev[100];
                char* marker = "revision=";
                char* p = NULL;
                while (fgets(line, sizeof(line), fp))
                    if ((p = strstr(line, marker)) != NULL)
                        break;
                if (p == NULL) {
                    debug("no 'revision=' line found in .svn/entries");
                    return NULL;
                }
                if (sscanf(p, " %*[^\"]\"%[0-9]\"", rev) == 1) {
                    result_set_revision(result, rev, -1);
                    debug("read svn revision from .svn/entries: '%s'", rev);
                }
            }
        }
        fclose(fp);
    }
    return result;
}

vccontext_t* get_svn_context(options_t* options)
{
    return init_context("svn", options, svn_probe, svn_get_info);
}
