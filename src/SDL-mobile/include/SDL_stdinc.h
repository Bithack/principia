/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2012 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

/**
 *  \file SDL_stdinc.h
 *
 *  This is a general header that includes C language support.
 */

#ifndef _SDL_stdinc_h
#define _SDL_stdinc_h

#include "SDL_config.h"

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif
#if defined(STDC_HEADERS)
# include <stdlib.h>
# include <stddef.h>
# include <stdarg.h>
#else
# if defined(HAVE_STDLIB_H)
#  include <stdlib.h>
# elif defined(HAVE_MALLOC_H)
#  include <malloc.h>
# endif
# if defined(HAVE_STDDEF_H)
#  include <stddef.h>
# endif
# if defined(HAVE_STDARG_H)
#  include <stdarg.h>
# endif
#endif
#ifdef HAVE_STRING_H
# if !defined(STDC_HEADERS) && defined(HAVE_MEMORY_H)
#  include <memory.h>
# endif
# include <string.h>
#endif
#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif
#if defined(HAVE_INTTYPES_H)
# include <inttypes.h>
#elif defined(HAVE_STDINT_H)
# include <stdint.h>
#endif
#ifdef HAVE_CTYPE_H
# include <ctype.h>
#endif
#ifdef HAVE_MATH_H
# include <math.h>
#endif
#if defined(HAVE_ICONV) && defined(HAVE_ICONV_H)
# include <iconv.h>
#endif

/**
 *  The number of elements in an array.
 */
#define SDL_arraysize(array)	(sizeof(array)/sizeof(array[0]))
#define SDL_TABLESIZE(table)	SDL_arraysize(table)

/**
 *  \name Cast operators
 *
 *  Use proper C++ casts when compiled as C++ to be compatible with the option
 *  -Wold-style-cast of GCC (and -Werror=old-style-cast in GCC 4.2 and above).
 */
/*@{*/
#ifdef __cplusplus
#define SDL_reinterpret_cast(type, expression) reinterpret_cast<type>(expression)
#define SDL_static_cast(type, expression) static_cast<type>(expression)
#else
#define SDL_reinterpret_cast(type, expression) ((type)(expression))
#define SDL_static_cast(type, expression) ((type)(expression))
#endif
/*@}*//*Cast operators*/

/* Define a four character code as a Uint32 */
#define SDL_FOURCC(A, B, C, D) \
    ((SDL_static_cast(Uint32, SDL_static_cast(Uint8, (A))) << 0) | \
     (SDL_static_cast(Uint32, SDL_static_cast(Uint8, (B))) << 8) | \
     (SDL_static_cast(Uint32, SDL_static_cast(Uint8, (C))) << 16) | \
     (SDL_static_cast(Uint32, SDL_static_cast(Uint8, (D))) << 24))

/**
 *  \name Basic data types
 */
/*@{*/

typedef enum
{
    SDL_FALSE = 0,
    SDL_TRUE = 1
} SDL_bool;

/**
 * \brief A signed 8-bit integer type.
 */
typedef int8_t Sint8;
/**
 * \brief An unsigned 8-bit integer type.
 */
typedef uint8_t Uint8;
/**
 * \brief A signed 16-bit integer type.
 */
typedef int16_t Sint16;
/**
 * \brief An unsigned 16-bit integer type.
 */
typedef uint16_t Uint16;
/**
 * \brief A signed 32-bit integer type.
 */
typedef int32_t Sint32;
/**
 * \brief An unsigned 32-bit integer type.
 */
typedef uint32_t Uint32;

/**
 * \brief A signed 64-bit integer type.
 */
typedef int64_t Sint64;
/**
 * \brief An unsigned 64-bit integer type.
 */
typedef uint64_t Uint64;

/*@}*//*Basic data types*/


#define SDL_COMPILE_TIME_ASSERT(name, x)               \
       typedef int SDL_dummy_ ## name[(x) * 2 - 1]
/** \cond */
#ifndef DOXYGEN_SHOULD_IGNORE_THIS
SDL_COMPILE_TIME_ASSERT(uint8, sizeof(Uint8) == 1);
SDL_COMPILE_TIME_ASSERT(sint8, sizeof(Sint8) == 1);
SDL_COMPILE_TIME_ASSERT(uint16, sizeof(Uint16) == 2);
SDL_COMPILE_TIME_ASSERT(sint16, sizeof(Sint16) == 2);
SDL_COMPILE_TIME_ASSERT(uint32, sizeof(Uint32) == 4);
SDL_COMPILE_TIME_ASSERT(sint32, sizeof(Sint32) == 4);
SDL_COMPILE_TIME_ASSERT(uint64, sizeof(Uint64) == 8);
SDL_COMPILE_TIME_ASSERT(sint64, sizeof(Sint64) == 8);
#endif /* DOXYGEN_SHOULD_IGNORE_THIS */
/** \endcond */

/* Check to make sure enums are the size of ints, for structure packing.
   For both Watcom C/C++ and Borland C/C++ the compiler option that makes
   enums having the size of an int must be enabled.
   This is "-b" for Borland C/C++ and "-ei" for Watcom C/C++ (v11).
*/
/* Enable enums always int in CodeWarrior (for MPW use "-enum int") */
#ifdef __MWERKS__
#pragma enumsalwaysint on
#endif

/** \cond */
#ifndef DOXYGEN_SHOULD_IGNORE_THIS
#if !defined(__NINTENDODS__) && !defined(__ANDROID__)
   /* TODO: include/SDL_stdinc.h:174: error: size of array 'SDL_dummy_enum' is negative */
typedef enum
{
    DUMMY_ENUM_VALUE
} SDL_DUMMY_ENUM;

SDL_COMPILE_TIME_ASSERT(enum, sizeof(SDL_DUMMY_ENUM) == sizeof(int));
#endif
#endif /* DOXYGEN_SHOULD_IGNORE_THIS */
/** \endcond */

#include "begin_code.h"
/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
/* *INDENT-OFF* */
extern "C" {
/* *INDENT-ON* */
#endif

#define SDL_malloc	malloc
#define SDL_calloc	calloc
#define SDL_realloc	realloc
#define SDL_free	free

#include <alloca.h>
#define SDL_stack_alloc(type, count)    (type*)alloca(sizeof(type)*(count))
#define SDL_stack_free(data)

#define SDL_getenv	getenv
#define SDL_setenv	setenv
#define SDL_qsort	qsort
#define SDL_abs		abs

#define SDL_min(x, y)	(((x) < (y)) ? (x) : (y))
#define SDL_max(x, y)	(((x) > (y)) ? (x) : (y))

#define SDL_isdigit(X)  isdigit(X)
#define SDL_isspace(X)  isspace(X)
#define SDL_toupper(X)  toupper(X)
#define SDL_tolower(X)  tolower(X)

#define SDL_memset      memset

#define SDL_zero(x)	SDL_memset(&(x), 0, sizeof((x)))
#define SDL_zerop(x)	SDL_memset((x), 0, sizeof(*(x)))

#define SDL_memset4(dst, val, len)		\
do {						\
	unsigned _count = (len);		\
	unsigned _n = (_count + 3) / 4;		\
	Uint32 *_p = SDL_static_cast(Uint32 *, dst);		\
	Uint32 _val = (val);			\
	if (len == 0) break;			\
        switch (_count % 4) {			\
        case 0: do {    *_p++ = _val;		\
        case 3:         *_p++ = _val;		\
        case 2:         *_p++ = _val;		\
        case 1:         *_p++ = _val;		\
		} while ( --_n );		\
	}					\
} while(0)

#define SDL_memcpy      memcpy
#define SDL_memcpy4(dst, src, len)	SDL_memcpy((dst), (src), (len) << 2)
#define SDL_memmove     memmove
#define SDL_memcmp      memcmp
#define SDL_strlen      strlen
#define SDL_strlcpy     strlcpy

extern DECLSPEC size_t SDLCALL SDL_utf8strlcpy(char *dst, const char *src,
                                            size_t dst_bytes);

#define SDL_strlcat    strlcat
#define SDL_strdup     strdup
#define SDL_strchr      strchr
#define SDL_strrchr     strrchr
#define SDL_strstr      strstr
#define SDL_strtol      strtol
#define SDL_strtoul      strtoul
#define SDL_strtoll     strtoll
#define SDL_strtoull     strtoull
#define SDL_strtod      strtod
#define SDL_atoi        atoi
#define SDL_atof        atof
#define SDL_strcmp      strcmp
#define SDL_strncmp     strncmp
#define SDL_strcasecmp  strcasecmp
#define SDL_strncasecmp strncasecmp
#define SDL_sscanf      sscanf
#define SDL_snprintf    snprintf
#define SDL_vsnprintf   vsnprintf
#define SDL_atan        atan
#define SDL_atan2       atan2
#define SDL_ceil        ceil
#define SDL_copysign    copysign
#define SDL_cos         cos
#define SDL_cosf        cosf
#define SDL_fabs        fabs
#define SDL_floor       floor
#define SDL_log         log
#define SDL_pow         pow
#define SDL_scalbn      scalbn
#define SDL_sin         sin
#define SDL_sinf        sinf
#define SDL_sqrt        sqrt

/* The SDL implementation of iconv() returns these error codes */
#define SDL_ICONV_ERROR		(size_t)-1
#define SDL_ICONV_E2BIG		(size_t)-2
#define SDL_ICONV_EILSEQ	(size_t)-3
#define SDL_ICONV_EINVAL	(size_t)-4

#if defined(HAVE_ICONV) && defined(HAVE_ICONV_H)
#define SDL_iconv_t     iconv_t
#define SDL_iconv_open  iconv_open
#define SDL_iconv_close iconv_close
#else
typedef struct _SDL_iconv_t *SDL_iconv_t;
extern DECLSPEC SDL_iconv_t SDLCALL SDL_iconv_open(const char *tocode,
                                                   const char *fromcode);
extern DECLSPEC int SDLCALL SDL_iconv_close(SDL_iconv_t cd);
#endif
extern DECLSPEC size_t SDLCALL SDL_iconv(SDL_iconv_t cd, const char **inbuf,
                                         size_t * inbytesleft, char **outbuf,
                                         size_t * outbytesleft);
/**
 *  This function converts a string between encodings in one pass, returning a
 *  string that must be freed with SDL_free() or NULL on error.
 */
extern DECLSPEC char *SDLCALL SDL_iconv_string(const char *tocode,
                                               const char *fromcode,
                                               const char *inbuf,
                                               size_t inbytesleft);
#define SDL_iconv_utf8_locale(S)	SDL_iconv_string("", "UTF-8", S, SDL_strlen(S)+1)
#define SDL_iconv_utf8_ucs2(S)		(Uint16 *)SDL_iconv_string("UCS-2", "UTF-8", S, SDL_strlen(S)+1)
#define SDL_iconv_utf8_ucs4(S)		(Uint32 *)SDL_iconv_string("UCS-4", "UTF-8", S, SDL_strlen(S)+1)

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
/* *INDENT-OFF* */
}
/* *INDENT-ON* */
#endif
#include "close_code.h"

#endif /* _SDL_stdinc_h */

/* vi: set ts=4 sw=4 expandtab: */
