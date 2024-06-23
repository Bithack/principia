#include "debugdraw.hh"
#include "entity.hh"

debugdraw::debugdraw()
{
    this->ddraw = tms_ddraw_alloc();

    this->SetFlags(
            e_shapeBit | e_centerOfMassBit | e_jointBit/* | e_aabbBit*/
            | e_staticBit
        );
}

void debugdraw::begin(tms::camera *cam)
{
    glDepthMask(GL_FALSE);
    glDisable(GL_DEPTH_TEST);

    tms_ddraw_set_matrices(this->ddraw, cam->view, cam->projection);
    tms_ddraw_set_color(this->ddraw, 1.f, 1.f, 1.f, 1.f);
}

void debugdraw::end()
{
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
}

void debugdraw::DrawPolygon(const b2Vec2 *vertices, int32 num_v, const b2Color &color)
{
    tms_ddraw_set_color(this->ddraw, color.r, color.g, color.b, 1.f);

    if (this->current_fixture != 0) {
        if (this->current_fixture->IsSensor())
            tms_ddraw_set_color(this->ddraw, .4f, .4f, .8f, 1.f);

        entity *e = (entity*)this->current_fixture->GetUserData();
        float z = (e ? e->get_layer() : 0) * LAYER_DEPTH;
        for (int x=0; x<num_v; x++) {
            tms_ddraw_line3d(this->ddraw,
                    vertices[x].x, vertices[x].y, z,
                    vertices[(x+1)%num_v].x, vertices[(x+1)%num_v].y, z
                );
        }
    } else {
        for (int x=0; x<num_v; x++) {
            tms_ddraw_line(this->ddraw,
                    vertices[x].x, vertices[x].y,
                    vertices[(x+1)%num_v].x, vertices[(x+1)%num_v].y
                );
        }
    }
}

void debugdraw::DrawSolidPolygon(const b2Vec2 *vertices, int32 num_v, const b2Color &color)
{
    this->DrawPolygon(vertices, num_v, color);
}

void debugdraw::DrawCircle(const b2Vec2 &center, float32 radius, const b2Color &color)
{
    tms_ddraw_set_color(this->ddraw, color.r, color.g, color.b, 1.f);

    float z = 0.f;
    if (this->current_fixture != 0) {
        entity *e = (entity*)this->current_fixture->GetUserData();
        z = (e ? e->get_layer() : 0) * LAYER_DEPTH;
    }

    tms_ddraw_lcircle3d(this->ddraw, center.x, center.y, z, radius, radius);
}

void debugdraw::DrawSolidCircle(const b2Vec2 &center, float32 radius, const b2Vec2 &axis, const b2Color &color)
{
    this->DrawCircle(center,radius,color);
}

void debugdraw::DrawSegment(const b2Vec2 &p1, const b2Vec2 &p2, const b2Color &color)
{
    tms_ddraw_set_color(this->ddraw, color.r, color.g, color.b, 1.f);

    tms_ddraw_line(this->ddraw, p1.x, p1.y, p2.x, p2.y);
}

void debugdraw::DrawTransform(const b2Transform &xf)
{
    tms_ddraw_set_color(this->ddraw, 0, 0, 0, 1.f);
    tms_ddraw_circle(this->ddraw, xf.p.x, xf.p.y, .1f, .1f);
}

void debugdraw::DrawParticles(const b2Vec2 *p, float32 a, const b2ParticleColor *col, int32 i)
{
    tms_ddraw_set_color(this->ddraw, 0.f, 0.f, 0.f, 1.f);

    for (int x=0; x<i; x++) {
        tms_ddraw_circle(this->ddraw, p[x].x, p[x].y, a, a);
    }
}

