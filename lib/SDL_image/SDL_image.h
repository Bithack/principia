/*
  SDL_image:  An example image loading library for use with SDL
  Copyright (C) 1997-2024 Sam Lantinga <slouken@libsdl.org>

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
 *  \file SDL_image.h
 *
 *  Header file for SDL_image library
 *
 * A simple library to load images of various formats as SDL surfaces
 */
#ifndef SDL_IMAGE_H_
#define SDL_IMAGE_H_

#include "SDL.h"
#include "SDL_version.h"
#include "begin_code.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/**
 * Printable format: "%d.%d.%d", MAJOR, MINOR, PATCHLEVEL
 */
#define SDL_IMAGE_MAJOR_VERSION 2
#define SDL_IMAGE_MINOR_VERSION 9
#define SDL_IMAGE_PATCHLEVEL    0

/**
 * This macro can be used to fill a version structure with the compile-time
 * version of the SDL_image library.
 */
#define SDL_IMAGE_VERSION(X)                        \
{                                                   \
    (X)->major = SDL_IMAGE_MAJOR_VERSION;           \
    (X)->minor = SDL_IMAGE_MINOR_VERSION;           \
    (X)->patch = SDL_IMAGE_PATCHLEVEL;              \
}

#if SDL_IMAGE_MAJOR_VERSION < 3 && SDL_MAJOR_VERSION < 3
/**
 *  This is the version number macro for the current SDL_image version.
 *
 *  In versions higher than 2.9.0, the minor version overflows into
 *  the thousands digit: for example, 2.23.0 is encoded as 4300.
 *  This macro will not be available in SDL 3.x or SDL_image 3.x.
 *
 *  Deprecated, use SDL_IMAGE_VERSION_ATLEAST or SDL_IMAGE_VERSION instead.
 */
#define SDL_IMAGE_COMPILEDVERSION \
    SDL_VERSIONNUM(SDL_IMAGE_MAJOR_VERSION, SDL_IMAGE_MINOR_VERSION, SDL_IMAGE_PATCHLEVEL)
#endif /* SDL_IMAGE_MAJOR_VERSION < 3 && SDL_MAJOR_VERSION < 3 */

/**
 *  This macro will evaluate to true if compiled with SDL_image at least X.Y.Z.
 */
#define SDL_IMAGE_VERSION_ATLEAST(X, Y, Z) \
    ((SDL_IMAGE_MAJOR_VERSION >= X) && \
     (SDL_IMAGE_MAJOR_VERSION > X || SDL_IMAGE_MINOR_VERSION >= Y) && \
     (SDL_IMAGE_MAJOR_VERSION > X || SDL_IMAGE_MINOR_VERSION > Y || SDL_IMAGE_PATCHLEVEL >= Z))

/**
 * This function gets the version of the dynamically linked SDL_image library.
 *
 * it should NOT be used to fill a version structure, instead you should use
 * the SDL_IMAGE_VERSION() macro.
 *
 * \returns SDL_image version
 */
extern DECLSPEC const SDL_version * SDLCALL IMG_Linked_Version(void);

/**
 * Initialization flags
 */
typedef enum
{
    IMG_INIT_JPG    = 0x00000001,
    IMG_INIT_PNG    = 0x00000002
} IMG_InitFlags;

/**
 * Initialize SDL_image.
 */
extern DECLSPEC int SDLCALL IMG_Init(int flags);

/**
 * Deinitialize SDL_image.
 */
extern DECLSPEC void SDLCALL IMG_Quit(void);

/**
 * Load an image from an SDL data source into a software surface.
 */
extern DECLSPEC SDL_Surface * SDLCALL IMG_LoadTyped_RW(SDL_RWops *src, int freesrc, const char *type);

/**
 * Load an image from a filesystem path into a software surface.
 */
extern DECLSPEC SDL_Surface * SDLCALL IMG_Load(const char *file);

/**
 * Load an image from an SDL data source into a software surface.
 */
extern DECLSPEC SDL_Surface * SDLCALL IMG_Load_RW(SDL_RWops *src, int freesrc);

/**
 * Detect JPG image data on a readable/seekable SDL_RWops.
 */
extern DECLSPEC int SDLCALL IMG_isJPG(SDL_RWops *src);

/**
 * Detect PNG image data on a readable/seekable SDL_RWops.
 */
extern DECLSPEC int SDLCALL IMG_isPNG(SDL_RWops *src);

/**
 * Load a JPG image directly.
 */
extern DECLSPEC SDL_Surface * SDLCALL IMG_LoadJPG_RW(SDL_RWops *src);

/**
 * Load a PNG image directly.
 */
extern DECLSPEC SDL_Surface * SDLCALL IMG_LoadPNG_RW(SDL_RWops *src);

/**
 * Report SDL_image errors
 *
 * \sa IMG_GetError
 */
#define IMG_SetError    SDL_SetError

/**
 * Get last SDL_image error
 *
 * \sa IMG_SetError
 */
#define IMG_GetError    SDL_GetError

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif
#include "close_code.h"

#endif /* SDL_IMAGE_H_ */
