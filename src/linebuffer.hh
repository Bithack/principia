#pragma once

#include <tms/cpp.hh>

#define LINEBUFFER_MAX 512

class linebuffer
{
  public:
    static void _init();
    static void reset();
    static void upload();
    static void add(
            float x1, float y1, float z1,
            float x2, float y2, float z2,
            float r1, float g1, float b1, float a1,
            float r2, float g2, float b2, float a2,
            float w1, float w2
            );
    static void add2(
            float x1, float y1, float z1,
            float x2, float y2, float z2,
            float r1, float g1, float b1, float a1,
            float r2, float g2, float b2, float a2,
            float w1, float w2
            );
    static tms::entity *get_entity();
    static tms::entity *get_entity2();

  private:
    static tms::gbuffer *verts;
    static tms::gbuffer *verts2;
    static tms::gbuffer *indices;
    static tms::varray *va;
    static tms::varray *va2;
    static tms::mesh *mesh;
    static tms::mesh *mesh2;
    static tms::entity *e;
    static tms::entity *e2;

    static int n;
    static int n2;
};
