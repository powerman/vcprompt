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
        int rev;
        // Check the version
        if (fgets(line, sizeof(line), fp)) {
            if(isdigit(line[0])) {
                // Custom file format (working copy created by svn >= 1.4)
                fgets(line, sizeof(line), fp); // Get the name
                fgets(line, sizeof(line), fp); // Get the entries kind
                if(fgets(line, sizeof(line), fp)) { // Get the rev numver
                    int len = strlen(line);
                    if (line[len-1] == '\n')
                        line[len-1] = '\0';
                    result->revision = strdup(line);
                    debug("read a svn revision from .svn/entries: '%s'", line);
                }
            } else {
                // XML file format (working copy created by svn < 1.4)
                while (fgets(line, sizeof(line),fp))
                    if (strstr(line, "revision=")) {
                        break;
                    }
                if (sscanf(line, " %*[^\"]\"%d%*[^\n]", &rev) == 1) {
                    result->revision = strdup((char *)rev);
                    debug("read a svn revision from .svn/entries: '%s'", rev);
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
