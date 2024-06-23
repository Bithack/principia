#ifndef _TMS_UTIL__H_
#define _TMS_UTIL__H_

#include <stdint.h>

/* this header file will probably never be used outside tms,
 * thus no prefixes */

#define ARRAY_NEXT_SIZE(x) (((x)+16)*3/2)

/* ensure capacity in number of components, will thus not work with void* arrays */
#define ARRAY_ENSURE_CAPACITY(a, min, curr) \
    do { \
        if (curr < (min)) { \
            curr = (ARRAY_NEXT_SIZE(curr) < (min) ? (min) : ARRAY_NEXT_SIZE(curr)); \
            a = realloc((a), curr*sizeof(*(a))); \
        } \
    } while (0);

#endif
