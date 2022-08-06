#ifndef _TMS_UTIL__H_
#define _TMS_UTIL__H_

#include <stdint.h>

#ifndef TMS_STATIC_INLINE
#if defined(_MSC_VER)
#define TMS_STATIC_INLINE static __inline
#else
#define TMS_STATIC_INLINE static inline
#endif
#endif

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

#define tfunction(ret, block) ({ret __ block __;})

/** 
 * Round up to the nearest power of 2
 * works only on 32 bit integers
 **/
TMS_STATIC_INLINE uint32_t tnpo2_uint32(uint32_t i)
{
    i--;
    i |= i >> 1;
    i |= i >> 2;
    i |= i >> 4;
    i |= i >> 8;
    i |= i >> 16;
    i ++;

    return i;
}

#endif
