#pragma once

#include "entity.hh"

#define SPRITEBUFFER_MAX 512

class spritebuffer
{
  public:
    static void _init();
    static void reset();
    static void upload();
    static void add(float x, float y, float z, float r, float g, float b, float a, float w, float h, int sprite);
    static void add(float x, float y, float z, float r, float g, float b, float a, float w, float h, int sprite, float rot);
    static void add2(float x, float y, float z, float r, float g, float b, float a, float w, float h, int sprite);
    static void add2(float x, float y, float z, float r, float g, float b, float a, float w, float h, int sprite, float rot);
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

    static struct spritebuf_vert base[4];
};
