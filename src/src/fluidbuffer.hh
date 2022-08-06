#ifndef _FLUIDBUFFER__H_
#define _FLUIDBUFFER__H_

#include "entity.hh"

#define FLUIDBUFFER_MAX 4096

class fluidbuffer
{
  public:
    static void _init();
    static void reset();
    static void upload();
    //static void add(float x, float y, float z, float r, float g, float b, float a, float w, float h);
    static void add(float x, float y, float z, float pressure, float w, float h);
    static tms::entity *get_entity();
};

#endif
