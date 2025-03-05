#pragma once

#include <Box2D/Box2D.h>
#include <tms/bindings/cpp/cpp.hh>

class world;

class debugdraw : public b2Draw
{
  private:
    struct tms_ddraw *ddraw;

  public:
    debugdraw();
    void begin(tms::camera *cam);
    void end();

    void DrawPolygon(const b2Vec2 *vertices, int32 num_v, const b2Color &color);
    void DrawSolidPolygon(const b2Vec2 *vertices, int32 num_v, const b2Color &color);
    void DrawCircle(const b2Vec2 &center, float32 radius, const b2Color &color);
    void DrawSolidCircle(const b2Vec2 &center, float32 radius, const b2Vec2 &axis, const b2Color &color);
    void DrawSegment(const b2Vec2 &p1, const b2Vec2 &p2, const b2Color &color);
    void DrawTransform(const b2Transform &xf);
    void DrawParticles(const b2Vec2 *p, float32 a, const b2ParticleColor *col, int32 i);
};
