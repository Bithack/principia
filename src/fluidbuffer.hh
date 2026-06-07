#pragma once

#include <tms/cpp.hh>

#define FLUIDBUFFER_MAX_1_5_1  4096  // Used in <= 1.5.1
#define FLUIDBUFFER_MAX        16384 // Used in >= 1.5.2

class fluidbuffer
{
  public:
    static void _init();
    static void reset();
    static void upload();
    //static void add(float x, float y, float z, float r, float g, float b, float a, float w, float h);
    static void add(float x, float y, float z, float pressure, float w, float h);
    static tms::entity *get_entity();

  private:
    static tms::gbuffer *verts;
    static tms::gbuffer *indices;
    static tms::varray *va;
    static tms::mesh *mesh;
    static tms::entity *e;
    static tms::entity *e2;

    static uint32_t n;
    static struct fluidbuf_vert base[4];
};
