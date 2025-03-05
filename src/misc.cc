#include "misc.hh"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

time_t
get_mtime(const char *path)
{
    time_t mtime;
    char date[21];

    struct stat st;
    stat(path, &st);
    strftime(date, 20, "%Y-%m-%d %H:%M:%S", gmtime((time_t*)&(st.st_mtime)));
    mtime = st.st_mtime;

    return mtime;
}

std::vector<char*>
p_split(const char *str, size_t len, const char *delim)
{
    char *tmp = strdup(str);
    std::vector<char*> ret;

    char *pch = strtok(tmp, ";");

    if (pch == NULL && len > 0) {
        ret.push_back(strdup(tmp));
    } else {
        while (pch != NULL) {
            if (strlen(pch)) {
                ret.push_back(strdup(pch));
            }
            pch = strtok(NULL, ";");
        }
    }

    free(tmp);

    return ret;
}
