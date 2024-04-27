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

#ifndef _SDL_config_android_h
#define _SDL_config_android_h

#include "SDL_platform.h"

/**
 *  \file SDL_config_android.h
 *
 *  This is a configuration that can be used to build SDL for Android
 */

#include <stdarg.h>

#define HAVE_SYS_TYPES_H	1
#define HAVE_STDIO_H	1
#define STDC_HEADERS	1
#define HAVE_STRING_H	1
#define HAVE_INTTYPES_H	1
#define HAVE_STDINT_H	1
#define HAVE_CTYPE_H	1
#define HAVE_MATH_H	1
#define HAVE_SIGNAL_H	1

/* C library functions */

#define HAVE_SETJMP	1
#define HAVE_NANOSLEEP	1
#define HAVE_SYSCONF	1

#define SIZEOF_VOIDP 4

/* Enable various audio drivers */
#define SDL_AUDIO_DRIVER_ANDROID	1
#define SDL_AUDIO_DRIVER_DUMMY	0

/* Enable various input drivers */
#define SDL_JOYSTICK_ANDROID	1

/* Enable various shared object loading systems */
#define SDL_LOADSO_DLOPEN	1

/* Enable various threading systems */
#define SDL_THREAD_PTHREAD	1
#define SDL_THREAD_PTHREAD_RECURSIVE_MUTEX	1

/* Enable various timer systems */
#define SDL_TIMER_UNIX	1

/* Enable various video drivers */
#define SDL_VIDEO_DRIVER_ANDROID 1

/* Enable OpenGL ES */
#define SDL_VIDEO_OPENGL_ES	1
#define SDL_VIDEO_RENDER_OGL_ES	1
#define SDL_VIDEO_RENDER_OGL_ES2	1

#define SDL_RENDER_DISABLED 1

#endif /* _SDL_config_android_h */
