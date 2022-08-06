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
#ifndef _SDL_uikitvideo_h
#define _SDL_uikitvideo_h

#include <UIKit/UIKit.h>

extern BOOL SDL_UIKit_supports_multiple_displays;

typedef struct SDL_DisplayData SDL_DisplayData;

struct SDL_DisplayData
{
    UIScreen *uiscreen;
    CGFloat scale;
};

typedef struct SDL_DisplayModeData SDL_DisplayModeData;

struct SDL_DisplayModeData
{
    UIScreenMode *uiscreenmode;
    CGFloat scale;
};

#endif /* _SDL_uikitvideo_h */

/* vi: set ts=4 sw=4 expandtab: */
