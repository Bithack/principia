#include "misc.hh"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef TMS_BACKEND_WINDOWS
#include <windows.h>

static time_t
filetime_to_timet(FILETIME & ft)
{
    ULARGE_INTEGER ull;
    ull.LowPart = ft.dwLowDateTime;
    ull.HighPart = ft.dwHighDateTime;
    return ull.QuadPart / 10000000ULL - 11644473600ULL;
}
#endif

time_t
get_mtime(const char *path)
{
    time_t mtime;
    char date[21];

#if defined(TMS_BACKEND_WINDOWS) && 0
    WIN32_FILE_ATTRIBUTE_DATA data;

    GetFileAttributesEx((LPCWSTR)(path), GetFileExInfoStandard, &data);

    FILETIME time = data.ftLastWriteTime;
    SYSTEMTIME sys_time, local_time;

    FileTimeToSystemTime(&time, &sys_time);
    SystemTimeToTzSpecificLocalTime(0, &sys_time, &local_time);
    snprintf(date, 20, "%04d-%02d-%02d %02d:%02d:%02d", local_time.wYear, local_time.wMonth, local_time.wDay, local_time.wHour, local_time.wMinute, local_time.wSecond);
    mtime = filetime_to_timet(time);
#else
    struct stat st;
    stat(path, &st);
    strftime(date, 20, "%Y-%m-%d %H:%M:%S", gmtime((time_t*)&(st.st_mtime)));
    mtime = st.st_mtime;
#endif

    return mtime;
}

/** 
 * http://en.wikibooks.org/wiki/Algorithm_Implementation/Miscellaneous/Base64#C
 **/
int
base64encode(const void* data_buf, size_t dataLength, char* result, size_t resultSize)
{
    const char base64chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    const uint8_t *data = (const uint8_t *)data_buf;
    size_t resultIndex = 0;
    size_t x;
    uint32_t n = 0;
    int padCount = dataLength % 3;
    uint8_t n0, n1, n2, n3;

    /* increment over the length of the string, three characters at a time */
    for (x = 0; x < dataLength; x += 3)
    {
        /* these three 8-bit (ASCII) characters become one 24-bit number */
        n = data[x] << 16;

        if((x+1) < dataLength)
            n += data[x+1] << 8;

        if((x+2) < dataLength)
            n += data[x+2];

        /* this 24-bit number gets separated into four 6-bit numbers */
        n0 = (uint8_t)(n >> 18) & 63;
        n1 = (uint8_t)(n >> 12) & 63;
        n2 = (uint8_t)(n >> 6) & 63;
        n3 = (uint8_t)n & 63;

        /*
         * if we have one byte available, then its encoding is spread
         * out over two characters
         */
        if(resultIndex >= resultSize) return 0;   /* indicate failure: buffer too small */
        result[resultIndex++] = base64chars[n0];
        if(resultIndex >= resultSize) return 0;   /* indicate failure: buffer too small */
        result[resultIndex++] = base64chars[n1];

        /*
         * if we have only two bytes available, then their encoding is
         * spread out over three chars
         */
        if((x+1) < dataLength)
        {
            if(resultIndex >= resultSize) return 0;   /* indicate failure: buffer too small */
            result[resultIndex++] = base64chars[n2];
        }

        /*
         * if we have all three bytes available, then their encoding is spread
         * out over four characters
         */
        if((x+2) < dataLength)
        {
            if(resultIndex >= resultSize) return 0;   /* indicate failure: buffer too small */
            result[resultIndex++] = base64chars[n3];
        }
    }

    /*
     * create and add padding that is required if we did not have a multiple of 3
     * number of characters available
     */
    if (padCount > 0)
    {
        for (; padCount < 3; padCount++)
        {
            if(resultIndex >= resultSize) return 0;   /* indicate failure: buffer too small */
            result[resultIndex++] = '=';
        }
    }
    if(resultIndex >= resultSize) return 0;   /* indicate failure: buffer too small */
    result[resultIndex] = 0;
    return 1;   /* indicate success */
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
