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
#include "SDL_config.h"

/* This file provides a general interface for SDL to read and write
   data sources.  It can easily be extended to files, memory, etc.
*/

#include "SDL_endian.h"
#include "SDL_rwops.h"

#ifdef ANDROID
#include "../core/android/SDL_android.h"
#endif


#ifdef HAVE_STDIO_H

/* Functions to read/write stdio file pointers */

static long SDLCALL
stdio_seek(SDL_RWops * context, long offset, int whence)
{
    if (fseek(context->hidden.stdio.fp, offset, whence) == 0) {
        return (ftell(context->hidden.stdio.fp));
    } else {
        SDL_Error(SDL_EFSEEK);
        return (-1);
    }
}

static size_t SDLCALL
stdio_read(SDL_RWops * context, void *ptr, size_t size, size_t maxnum)
{
    size_t nread;

    nread = fread(ptr, size, maxnum, context->hidden.stdio.fp);
    if (nread == 0 && ferror(context->hidden.stdio.fp)) {
        SDL_Error(SDL_EFREAD);
    }
    return (nread);
}

static size_t SDLCALL
stdio_write(SDL_RWops * context, const void *ptr, size_t size, size_t num)
{
    size_t nwrote;

    nwrote = fwrite(ptr, size, num, context->hidden.stdio.fp);
    if (nwrote == 0 && ferror(context->hidden.stdio.fp)) {
        SDL_Error(SDL_EFWRITE);
    }
    return (nwrote);
}

static int SDLCALL
stdio_close(SDL_RWops * context)
{
    int status = 0;
    if (context) {
        if (context->hidden.stdio.autoclose) {
            /* WARNING:  Check the return value here! */
            if (fclose(context->hidden.stdio.fp) != 0) {
                SDL_Error(SDL_EFWRITE);
                status = -1;
            }
        }
        SDL_FreeRW(context);
    }
    return status;
}
#endif /* !HAVE_STDIO_H */

/* Functions to read/write memory pointers */

static long SDLCALL
mem_seek(SDL_RWops * context, long offset, int whence)
{
    Uint8 *newpos;

    switch (whence) {
    case RW_SEEK_SET:
        newpos = context->hidden.mem.base + offset;
        break;
    case RW_SEEK_CUR:
        newpos = context->hidden.mem.here + offset;
        break;
    case RW_SEEK_END:
        newpos = context->hidden.mem.stop + offset;
        break;
    default:
        SDL_SetError("Unknown value for 'whence'");
        return (-1);
    }
    if (newpos < context->hidden.mem.base) {
        newpos = context->hidden.mem.base;
    }
    if (newpos > context->hidden.mem.stop) {
        newpos = context->hidden.mem.stop;
    }
    context->hidden.mem.here = newpos;
    return (long)(context->hidden.mem.here - context->hidden.mem.base);
}

static size_t SDLCALL
mem_read(SDL_RWops * context, void *ptr, size_t size, size_t maxnum)
{
    size_t total_bytes;
    size_t mem_available;

    total_bytes = (maxnum * size);
    if ((maxnum <= 0) || (size <= 0)
        || ((total_bytes / maxnum) != (size_t) size)) {
        return 0;
    }

    mem_available = (context->hidden.mem.stop - context->hidden.mem.here);
    if (total_bytes > mem_available) {
        total_bytes = mem_available;
    }

    SDL_memcpy(ptr, context->hidden.mem.here, total_bytes);
    context->hidden.mem.here += total_bytes;

    return (total_bytes / size);
}

static size_t SDLCALL
mem_write(SDL_RWops * context, const void *ptr, size_t size, size_t num)
{
    if ((context->hidden.mem.here + (num * size)) > context->hidden.mem.stop) {
        num = (context->hidden.mem.stop - context->hidden.mem.here) / size;
    }
    SDL_memcpy(context->hidden.mem.here, ptr, num * size);
    context->hidden.mem.here += num * size;
    return (num);
}

static size_t SDLCALL
mem_writeconst(SDL_RWops * context, const void *ptr, size_t size, size_t num)
{
    SDL_SetError("Can't write to read-only memory");
    return (-1);
}

static int SDLCALL
mem_close(SDL_RWops * context)
{
    if (context) {
        SDL_FreeRW(context);
    }
    return (0);
}


/* Functions to create SDL_RWops structures from various data sources */

SDL_RWops *
SDL_RWFromFile(const char *file, const char *mode)
{
    SDL_RWops *rwops = NULL;
    if (!file || !*file || !mode || !*mode) {
        SDL_SetError("SDL_RWFromFile(): No file or no mode specified");
        return NULL;
    }
#if defined(ANDROID)
    rwops = SDL_AllocRW();
    if (!rwops)
        return NULL;            /* SDL_SetError already setup by SDL_AllocRW() */
    if (Android_JNI_FileOpen(rwops, file, mode) < 0) {
        SDL_FreeRW(rwops);
        return NULL;
    }
    rwops->seek = Android_JNI_FileSeek;
    rwops->read = Android_JNI_FileRead;
    rwops->write = Android_JNI_FileWrite;
    rwops->close = Android_JNI_FileClose;

#elif defined(__WIN32__)
    rwops = SDL_AllocRW();
    if (!rwops)
        return NULL;            /* SDL_SetError already setup by SDL_AllocRW() */
    if (windows_file_open(rwops, file, mode) < 0) {
        SDL_FreeRW(rwops);
        return NULL;
    }
    rwops->seek = windows_file_seek;
    rwops->read = windows_file_read;
    rwops->write = windows_file_write;
    rwops->close = windows_file_close;

#elif HAVE_STDIO_H
    {
    	#ifdef __APPLE__
    	FILE *fp = SDL_OpenFPFromBundleOrFallback(file, mode);
        #else
    	FILE *fp = fopen(file, mode);
    	#endif
    	if (fp == NULL) {
            SDL_SetError("Couldn't open %s", file);
        } else {
            rwops = SDL_RWFromFP(fp, 1);
        }
    }
#else
    SDL_SetError("SDL not compiled with stdio support");
#endif /* !HAVE_STDIO_H */

    return (rwops);
}

#ifdef HAVE_STDIO_H
SDL_RWops *
SDL_RWFromFP(FILE * fp, SDL_bool autoclose)
{
    SDL_RWops *rwops = NULL;

#if 0
/*#ifdef __NDS__*/
    /* set it up so we can use stdio file function */
    fatInitDefault();
    printf("called fatInitDefault()");
#endif /* __NDS__ */

    rwops = SDL_AllocRW();
    if (rwops != NULL) {
        rwops->seek = stdio_seek;
        rwops->read = stdio_read;
        rwops->write = stdio_write;
        rwops->close = stdio_close;
        rwops->hidden.stdio.fp = fp;
        rwops->hidden.stdio.autoclose = autoclose;
    }
    return (rwops);
}
#else
SDL_RWops *
SDL_RWFromFP(void * fp, SDL_bool autoclose)
{
    SDL_SetError("SDL not compiled with stdio support");
    return NULL;
}
#endif /* HAVE_STDIO_H */

SDL_RWops *
SDL_RWFromMem(void *mem, int size)
{
    SDL_RWops *rwops;

    rwops = SDL_AllocRW();
    if (rwops != NULL) {
        rwops->seek = mem_seek;
        rwops->read = mem_read;
        rwops->write = mem_write;
        rwops->close = mem_close;
        rwops->hidden.mem.base = (Uint8 *) mem;
        rwops->hidden.mem.here = rwops->hidden.mem.base;
        rwops->hidden.mem.stop = rwops->hidden.mem.base + size;
    }
    return (rwops);
}

SDL_RWops *
SDL_RWFromConstMem(const void *mem, int size)
{
    SDL_RWops *rwops;

    rwops = SDL_AllocRW();
    if (rwops != NULL) {
        rwops->seek = mem_seek;
        rwops->read = mem_read;
        rwops->write = mem_writeconst;
        rwops->close = mem_close;
        rwops->hidden.mem.base = (Uint8 *) mem;
        rwops->hidden.mem.here = rwops->hidden.mem.base;
        rwops->hidden.mem.stop = rwops->hidden.mem.base + size;
    }
    return (rwops);
}

SDL_RWops *
SDL_AllocRW(void)
{
    SDL_RWops *area;

    area = (SDL_RWops *) SDL_malloc(sizeof *area);
    if (area == NULL) {
        SDL_OutOfMemory();
    }
    return (area);
}

void
SDL_FreeRW(SDL_RWops * area)
{
    SDL_free(area);
}

/* Functions for dynamically reading and writing endian-specific values */

Uint16
SDL_ReadLE16(SDL_RWops * src)
{
    Uint16 value;

    SDL_RWread(src, &value, (sizeof value), 1);
    return (SDL_SwapLE16(value));
}

Uint16
SDL_ReadBE16(SDL_RWops * src)
{
    Uint16 value;

    SDL_RWread(src, &value, (sizeof value), 1);
    return (SDL_SwapBE16(value));
}

Uint32
SDL_ReadLE32(SDL_RWops * src)
{
    Uint32 value;

    SDL_RWread(src, &value, (sizeof value), 1);
    return (SDL_SwapLE32(value));
}

Uint32
SDL_ReadBE32(SDL_RWops * src)
{
    Uint32 value;

    SDL_RWread(src, &value, (sizeof value), 1);
    return (SDL_SwapBE32(value));
}

Uint64
SDL_ReadLE64(SDL_RWops * src)
{
    Uint64 value;

    SDL_RWread(src, &value, (sizeof value), 1);
    return (SDL_SwapLE64(value));
}

Uint64
SDL_ReadBE64(SDL_RWops * src)
{
    Uint64 value;

    SDL_RWread(src, &value, (sizeof value), 1);
    return (SDL_SwapBE64(value));
}

size_t
SDL_WriteLE16(SDL_RWops * dst, Uint16 value)
{
    value = SDL_SwapLE16(value);
    return (SDL_RWwrite(dst, &value, (sizeof value), 1));
}

size_t
SDL_WriteBE16(SDL_RWops * dst, Uint16 value)
{
    value = SDL_SwapBE16(value);
    return (SDL_RWwrite(dst, &value, (sizeof value), 1));
}

size_t
SDL_WriteLE32(SDL_RWops * dst, Uint32 value)
{
    value = SDL_SwapLE32(value);
    return (SDL_RWwrite(dst, &value, (sizeof value), 1));
}

size_t
SDL_WriteBE32(SDL_RWops * dst, Uint32 value)
{
    value = SDL_SwapBE32(value);
    return (SDL_RWwrite(dst, &value, (sizeof value), 1));
}

size_t
SDL_WriteLE64(SDL_RWops * dst, Uint64 value)
{
    value = SDL_SwapLE64(value);
    return (SDL_RWwrite(dst, &value, (sizeof value), 1));
}

size_t
SDL_WriteBE64(SDL_RWops * dst, Uint64 value)
{
    value = SDL_SwapBE64(value);
    return (SDL_RWwrite(dst, &value, (sizeof value), 1));
}

/* vi: set ts=4 sw=4 expandtab: */
