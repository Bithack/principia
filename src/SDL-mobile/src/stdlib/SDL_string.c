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

/* This file contains portable string manipulation functions for SDL */

#include "SDL_stdinc.h"


#define SDL_isupperhex(X)   (((X) >= 'A') && ((X) <= 'F'))
#define SDL_islowerhex(X)   (((X) >= 'a') && ((X) <= 'f'))

#define UTF8_IsLeadByte(c) ((c) >= 0xC0 && (c) <= 0xF4)
#define UTF8_IsTrailingByte(c) ((c) >= 0x80 && (c) <= 0xBF)

static int UTF8_TrailingBytes(unsigned char c)
{
    if (c >= 0xC0 && c <= 0xDF)
        return 1;
    else if (c >= 0xE0 && c <= 0xEF)
        return 2;
    else if (c >= 0xF0 && c <= 0xF4)
        return 3;
    else
        return 0;
}



size_t SDL_utf8strlcpy(char *dst, const char *src, size_t dst_bytes)
{
    size_t src_bytes = SDL_strlen(src);
    size_t bytes = SDL_min(src_bytes, dst_bytes - 1);
    size_t i = 0;
    char trailing_bytes = 0;
    if (bytes)
    {
        unsigned char c = (unsigned char)src[bytes - 1];
        if (UTF8_IsLeadByte(c))
            --bytes;
        else if (UTF8_IsTrailingByte(c))
        {
            for (i = bytes - 1; i != 0; --i)
            {
                c = (unsigned char)src[i];
                trailing_bytes = UTF8_TrailingBytes(c);
                if (trailing_bytes)
                {
                    if (bytes - i != trailing_bytes + 1)
                        bytes = i;

                    break;
                }
            }
        }
        SDL_memcpy(dst, src, bytes);
    }
    dst[bytes] = '\0';
    return bytes;
}
