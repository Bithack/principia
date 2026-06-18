/*
  SDL_image:  An example image loading library for use with SDL
  Copyright (C) 1997-2026 Sam Lantinga <slouken@libsdl.org>

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

#include <SDL_image.h>

/* Load an image from an SDL datasource, optionally specifying the type */
SDL_Surface *IMG_LoadTyped_IO(SDL_IOStream *src, bool closeio, const char *type)
{
    SDL_Surface *image;

    /* Make sure there is something to do.. */
    if (!src) {
        SDL_InvalidParamError("src");
        return NULL;
    }

    /* See whether or not this data source can handle seeking */
    if (SDL_SeekIO(src, 0, SDL_IO_SEEK_CUR) < 0) {
        SDL_SetError("Can't seek in this data source");
        if (closeio) {
            SDL_CloseIO(src);
        }
        return NULL;
    }

    if (type) {
        if (SDL_strcasecmp(type, "JPEG") == 0 || SDL_strcasecmp(type, "JPG") == 0) {
            image = IMG_LoadJPG_IO(src);
            if (closeio) {
                SDL_CloseIO(src);
            }
            return image;
        }
        if (SDL_strcasecmp(type, "PNG") == 0) {
            image = IMG_LoadPNG_IO(src);
            if (closeio) {
                SDL_CloseIO(src);
            }
            return image;
        }
    }

    if (closeio) {
        SDL_CloseIO(src);
    }
    SDL_SetError("Unsupported image format");
    return NULL;
}

/* Load an image from a file */
SDL_Surface *IMG_Load(const char *file)
{
    SDL_IOStream *src = SDL_IOFromFile(file, "rb");
    if (!src) {
        /* The error message has been set in SDL_IOFromFile */
        return NULL;
    }

    const char *ext = SDL_strrchr(file, '.');
    if (ext) {
        ext++;
    }
    return IMG_LoadTyped_IO(src, true, ext);
}
