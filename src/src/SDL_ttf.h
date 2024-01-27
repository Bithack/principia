/*
  SDL_ttf:  A companion library to SDL for working with TrueType (tm) fonts
  Copyright (C) 2001-2012 Sam Lantinga <slouken@libsdl.org>

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

/* This library is a wrapper around the excellent FreeType 2.0 library,
   available at:
	http://www.freetype.org/
*/

#ifndef _SDL_TTF_H
#define _SDL_TTF_H

#include "SDL.h"
#include "begin_code.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/* Printable format: "%d.%d.%d", MAJOR, MINOR, PATCHLEVEL
*/
#define SDL_TTF_MAJOR_VERSION	2
#define SDL_TTF_MINOR_VERSION	0
#define SDL_TTF_PATCHLEVEL	12

/* ZERO WIDTH NO-BREAKSPACE (Unicode byte order mark) */
#define UNICODE_BOM_NATIVE	0xFEFF
#define UNICODE_BOM_SWAPPED	0xFFFE

/* The internal structure containing font information */
typedef struct _TTF_Font TTF_Font;

/* Initialize the TTF engine - returns 0 if successful, -1 on error */
extern DECLSPEC int SDLCALL TTF_Init(void);

/* Open a font file and create a font of the specified point size.
 * Some .fon fonts will have several sizes embedded in the file, so the
 * point size becomes the index of choosing which size.  If the value
 * is too high, the last indexed size will be the default. */
extern DECLSPEC TTF_Font * SDLCALL TTF_OpenFont(const char *file, int ptsize);

/* Get the recommended spacing between lines of text for this font */
extern DECLSPEC int SDLCALL TTF_FontLineSkip(const TTF_Font *font);

/* Get the dimensions of a rendered string of text */
extern DECLSPEC int SDLCALL TTF_SizeUTF8(TTF_Font *font, const char *text, int *w, int *h);
extern DECLSPEC int SDLCALL TTF_SizeUNICODE(TTF_Font *font, const Uint16 *text, int *w, int *h);

/* Create an 8-bit palettized surface and render the given text at
   high quality with the given font and colors.  The 0 pixel is background,
   while other pixels have varying degrees of the foreground color.
   This function returns the new surface, or NULL if there was an error.
*/
extern DECLSPEC SDL_Surface * SDLCALL TTF_RenderUTF8_Shaded(TTF_Font *font,
				const char *text, SDL_Color fg, SDL_Color bg);
extern DECLSPEC SDL_Surface * SDLCALL TTF_RenderUNICODE_Shaded(TTF_Font *font,
				const Uint16 *text, SDL_Color fg, SDL_Color bg);

/* Close an opened font file */
extern DECLSPEC void SDLCALL TTF_CloseFont(TTF_Font *font);

/* De-initialize the TTF engine */
extern DECLSPEC void SDLCALL TTF_Quit(void);

/* We'll use SDL for reporting errors */
#define TTF_SetError	SDL_SetError
#define TTF_GetError	SDL_GetError

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif
#include "close_code.h"

#endif /* _SDL_TTF_H */
