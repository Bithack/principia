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

/* NSOpenGL implementation of SDL OpenGL support */

#if SDL_VIDEO_OPENGL_CGL
#include "SDL_cocoavideo.h"

#include <OpenGL/CGLTypes.h>
#include <OpenGL/OpenGL.h>
#include <OpenGL/CGLRenderers.h>

#include "SDL_loadso.h"
#include "SDL_opengl.h"


#define DEFAULT_OPENGL  "/System/Library/Frameworks/OpenGL.framework/Libraries/libGL.dylib"

int
Cocoa_GL_LoadLibrary(_THIS, const char *path)
{
    /* Load the OpenGL library */
    if (path == NULL) {
        path = SDL_getenv("SDL_OPENGL_LIBRARY");
    }
    if (path == NULL) {
        path = DEFAULT_OPENGL;
    }
    _this->gl_config.dll_handle = SDL_LoadObject(path);
    if (!_this->gl_config.dll_handle) {
        return -1;
    }
    SDL_strlcpy(_this->gl_config.driver_path, path,
                SDL_arraysize(_this->gl_config.driver_path));
    return 0;
}

void *
Cocoa_GL_GetProcAddress(_THIS, const char *proc)
{
    return SDL_LoadFunction(_this->gl_config.dll_handle, proc);
}

void
Cocoa_GL_UnloadLibrary(_THIS)
{
    SDL_UnloadObject(_this->gl_config.dll_handle);
    _this->gl_config.dll_handle = NULL;
}

SDL_GLContext
Cocoa_GL_CreateContext(_THIS, SDL_Window * window)
{
    NSAutoreleasePool *pool;
    SDL_VideoDisplay *display = SDL_GetDisplayForWindow(window);
    SDL_DisplayData *displaydata = (SDL_DisplayData *)display->driverdata;
    NSOpenGLPixelFormatAttribute attr[32];
    NSOpenGLPixelFormat *fmt;
    NSOpenGLContext *context;
    int i = 0;

    pool = [[NSAutoreleasePool alloc] init];

#ifndef FULLSCREEN_TOGGLEABLE
    if (window->flags & SDL_WINDOW_FULLSCREEN) {
        attr[i++] = NSOpenGLPFAFullScreen;
    }
#endif

    attr[i++] = NSOpenGLPFAColorSize;
    attr[i++] = SDL_BYTESPERPIXEL(display->current_mode.format)*8;

    attr[i++] = NSOpenGLPFADepthSize;
    attr[i++] = _this->gl_config.depth_size;

    if (_this->gl_config.double_buffer) {
        attr[i++] = NSOpenGLPFADoubleBuffer;
    }

    if (_this->gl_config.stereo) {
        attr[i++] = NSOpenGLPFAStereo;
    }

    if (_this->gl_config.stencil_size) {
        attr[i++] = NSOpenGLPFAStencilSize;
        attr[i++] = _this->gl_config.stencil_size;
    }

    if ((_this->gl_config.accum_red_size +
         _this->gl_config.accum_green_size +
         _this->gl_config.accum_blue_size +
         _this->gl_config.accum_alpha_size) > 0) {
        attr[i++] = NSOpenGLPFAAccumSize;
        attr[i++] = _this->gl_config.accum_red_size + _this->gl_config.accum_green_size + _this->gl_config.accum_blue_size + _this->gl_config.accum_alpha_size;
    }

    if (_this->gl_config.multisamplebuffers) {
        attr[i++] = NSOpenGLPFASampleBuffers;
        attr[i++] = _this->gl_config.multisamplebuffers;
    }

    if (_this->gl_config.multisamplesamples) {
        attr[i++] = NSOpenGLPFASamples;
        attr[i++] = _this->gl_config.multisamplesamples;
        attr[i++] = NSOpenGLPFANoRecovery;
    }

    if (_this->gl_config.accelerated >= 0) {
        if (_this->gl_config.accelerated) {
            attr[i++] = NSOpenGLPFAAccelerated;
        } else {
            attr[i++] = NSOpenGLPFARendererID;
            attr[i++] = kCGLRendererGenericFloatID;
        }
    }

    attr[i++] = NSOpenGLPFAScreenMask;
    attr[i++] = CGDisplayIDToOpenGLDisplayMask(displaydata->display);
    attr[i] = 0;

    fmt = [[NSOpenGLPixelFormat alloc] initWithAttributes:attr];
    if (fmt == nil) {
        SDL_SetError ("Failed creating OpenGL pixel format");
        [pool release];
        return NULL;
    }

    context = [[NSOpenGLContext alloc] initWithFormat:fmt shareContext:nil];

    [fmt release];

    if (context == nil) {
        SDL_SetError ("Failed creating OpenGL context");
        [pool release];
        return NULL;
    }

    /*
     * Wisdom from Apple engineer in reference to UT2003's OpenGL performance:
     *  "You are blowing a couple of the internal OpenGL function caches. This
     *  appears to be happening in the VAO case.  You can tell OpenGL to up
     *  the cache size by issuing the following calls right after you create
     *  the OpenGL context.  The default cache size is 16."    --ryan.
     */

    #ifndef GLI_ARRAY_FUNC_CACHE_MAX
    #define GLI_ARRAY_FUNC_CACHE_MAX 284
    #endif

    #ifndef GLI_SUBMIT_FUNC_CACHE_MAX
    #define GLI_SUBMIT_FUNC_CACHE_MAX 280
    #endif

    {
        GLint cache_max = 64;
        CGLContextObj ctx = [context CGLContextObj];
        CGLSetParameter (ctx, GLI_SUBMIT_FUNC_CACHE_MAX, &cache_max);
        CGLSetParameter (ctx, GLI_ARRAY_FUNC_CACHE_MAX, &cache_max);
    }

    /* End Wisdom from Apple Engineer section. --ryan. */

    [pool release];

    if ( Cocoa_GL_MakeCurrent(_this, window, context) < 0 ) {
        Cocoa_GL_DeleteContext(_this, context);
        return NULL;
    }

    return context;
}

int
Cocoa_GL_MakeCurrent(_THIS, SDL_Window * window, SDL_GLContext context)
{
    NSAutoreleasePool *pool;

    pool = [[NSAutoreleasePool alloc] init];

    if (context) {
        SDL_WindowData *windowdata = (SDL_WindowData *)window->driverdata;
        NSOpenGLContext *nscontext = (NSOpenGLContext *)context;

        if (window->flags & SDL_WINDOW_SHOWN) {
#ifndef FULLSCREEN_TOGGLEABLE
            if (window->flags & SDL_WINDOW_FULLSCREEN) {
                [nscontext setFullScreen];
            } else
#endif
            {
                [nscontext setView:[windowdata->nswindow contentView]];
                [nscontext update];
            }
        }
        [nscontext makeCurrentContext];
    } else {
        [NSOpenGLContext clearCurrentContext];
    }

    [pool release];
    return 0;
}

int
Cocoa_GL_SetSwapInterval(_THIS, int interval)
{
    NSAutoreleasePool *pool;
    NSOpenGLContext *nscontext;
    GLint value;
    int status;

    pool = [[NSAutoreleasePool alloc] init];

    nscontext = [NSOpenGLContext currentContext];
    if (nscontext != nil) {
        value = interval;
        [nscontext setValues:&value forParameter:NSOpenGLCPSwapInterval];
        status = 0;
    } else {
        SDL_SetError("No current OpenGL context");
        status = -1;
    }

    [pool release];
    return status;
}

int
Cocoa_GL_GetSwapInterval(_THIS)
{
    NSAutoreleasePool *pool;
    NSOpenGLContext *nscontext;
    GLint value;
    int status = 0;

    pool = [[NSAutoreleasePool alloc] init];

    nscontext = [NSOpenGLContext currentContext];
    if (nscontext != nil) {
        [nscontext getValues:&value forParameter:NSOpenGLCPSwapInterval];
        status = (int)value;
    }

    [pool release];
    return status;
}

void
Cocoa_GL_SwapWindow(_THIS, SDL_Window * window)
{
    NSAutoreleasePool *pool;
    NSOpenGLContext *nscontext;

    pool = [[NSAutoreleasePool alloc] init];

    /* FIXME: Do we need to get the context for the window? */
    nscontext = [NSOpenGLContext currentContext];
    if (nscontext != nil) {
        [nscontext flushBuffer];
    }

    [pool release];
}

void
Cocoa_GL_DeleteContext(_THIS, SDL_GLContext context)
{
    NSAutoreleasePool *pool;
    NSOpenGLContext *nscontext = (NSOpenGLContext *)context;

    pool = [[NSAutoreleasePool alloc] init];

    [nscontext clearDrawable];
    [nscontext release];

    [pool release];
}

#endif /* SDL_VIDEO_OPENGL_CGL */

/* vi: set ts=4 sw=4 expandtab: */
