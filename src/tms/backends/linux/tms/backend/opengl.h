#ifndef _TMS_GLINC__H_
#define _TMS_GLINC__H_

//#define GL3_PROTOTYPES

#include <GL/glew.h>


#include <SDL.h>

#ifdef GL_RGB565
#undef GL_RGB565
#endif

#define GL_RGB565 GL_RGB

#endif
