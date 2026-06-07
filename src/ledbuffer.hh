#pragma once

#include <tms/cpp.hh>

#define LEDBUFFER_MAX 512

#define LED_Z_OFFSET .05f

class ledbuffer
{
  public:
    static void _init();
    static void reset();
    static void upload();
    static void add(float x, float y, float z, float col);
    static tms::entity *get_entity();

  private:
    static tms::gbuffer *verts;
    static tms::gbuffer *indices;
    static tms::varray *va;
    static tms::mesh *mesh;
    static tms::entity *e;

    static int n;
    static tvec4 base[9];
};
