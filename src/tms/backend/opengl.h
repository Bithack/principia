#ifndef _TMS_BACKEND_OPENGL__H_
#define _TMS_BACKEND_OPENGL__H_

#if defined(TMS_BACKEND_ANDROID)

    #define GL_GLEXT_PROTOTYPES 1

    #include <GLES2/gl2.h>
    #include "SDL.h"

#elif defined(TMS_BACKEND_IOS)

    // not tested

    #include <OpenGLES/ES2/gl.h>
    #include <OpenGLES/ES2/glext.h>

#elif defined(TMS_BACKEND_LINUX_SS)

    // Screenshotter doesn't use GLEW

    #define GL_GLEXT_PROTOTYPES 1

    #include <GL/gl.h>
    #include <GL/glext.h>
    #include <GL/glx.h>
    #include <SDL.h>

    #ifndef GL_RGB565
    #define GL_RGB565 GL_RGB
    #endif

#else

    // Any other platform (Linux, Windows, etc...)

    #ifdef TMS_BACKEND_WINDOWS
        #include <windows.h>
    #endif

    #include <GL/glew.h>

    #ifdef TMS_BACKEND_WINDOWS
        #include <GL/wglew.h>
    #endif

    #include <SDL.h>

    #ifdef GL_RGB565
    #undef GL_RGB565
    #endif

    #define GL_RGB565 GL_RGB

#endif

#endif
