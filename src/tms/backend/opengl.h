#ifndef _TMS_BACKEND_OPENGL__H_
#define _TMS_BACKEND_OPENGL__H_


#ifdef TMS_BACKEND_WINDOWS
    #include <windows.h>
#endif

#ifdef TMS_USE_GLES
    #define GL_GLEXT_PROTOTYPES 1
    #include <GLES2/gl2.h>
#else
    #ifdef TMS_USE_GLEW
        #include <GL/glew.h>
    #else
        // e.g. screenshotter doesn't use GLEW
        #define GL_GLEXT_PROTOTYPES 1
        #include <GL/gl.h>
        #include <GL/glext.h>
        #include <GL/glx.h>
    #endif
#endif

#ifdef TMS_BACKEND_WINDOWS
    #include <GL/wglew.h>
#endif

#include <SDL.h>


#endif
