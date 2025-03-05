#include "linebuffer.hh"
#include "game.hh"

static tms::gbuffer *verts;
static tms::gbuffer *verts2;
static tms::gbuffer *indices;
static tms::varray *va;
static tms::varray *va2;
static tms::mesh *mesh;
static tms::mesh *mesh2;
static tms::entity *e;
static tms::entity *e2;

static float cam_x = 0.f;
static float cam_y = 0.f;

static int n = 0;
static int n2 = 0;

struct vert {
    tvec3 pos;
    tvec2 uv;
    tvec4 color;
};

void linebuffer::reset()
{
    n = 0;
    n2 = 0;

    if (G) {
      cam_x = G->cam->_position.x;
      cam_y = G->cam->_position.y;
    }

    if (e) {
        tmat4_load_identity(e->M);
        tmat4_translate(e->M, cam_x, cam_y, 0.f);
    }

    if (e2) {
        tmat4_load_identity(e2->M);
        tmat4_translate(e2->M, cam_x, cam_y, 0.f);
    }
}

tms::entity *
linebuffer::get_entity()
{
    if (e) return e;

    e = new tms::entity();
    mesh = new tms::mesh(va, indices);

    e->prio = 0;
    e->set_mesh(mesh);
    e->set_material(&m_linebuf);

    mesh->i_start = 0;
    mesh->i_count = n*9*3;

    return e;
}

tms::entity *
linebuffer::get_entity2()
{
    if (e2) return e2;

    e2 = new tms::entity();
    mesh2 = new tms::mesh(va2, indices);

    e2->prio = 0;
    e2->set_mesh(mesh2);
    e2->set_material(&m_linebuf2);

    mesh2->i_start = 0;
    mesh2->i_count = n2*9*3;

    return e2;
}

void linebuffer::_init()
{
    tms_infof("Initializing linebuffer");

    verts = new tms::gbuffer(4*LINEBUFFER_MAX*sizeof(struct vert));
    verts->usage = TMS_GBUFFER_STREAM_DRAW;

    verts2 = new tms::gbuffer(4*LINEBUFFER_MAX*sizeof(struct vert));
    verts2->usage = TMS_GBUFFER_STREAM_DRAW;

    indices = new tms::gbuffer(6*LINEBUFFER_MAX*sizeof(uint16_t));
    indices->usage = TMS_GBUFFER_STATIC_DRAW;

    va = new tms::varray(3);
    va->map_attribute("position", 3, GL_FLOAT, verts);
    va->map_attribute("uv", 2, GL_FLOAT, verts);
    va->map_attribute("vcolor", 4, GL_FLOAT, verts);

    va2 = new tms::varray(3);
    va2->map_attribute("position", 3, GL_FLOAT, verts2);
    va2->map_attribute("uv", 2, GL_FLOAT, verts2);
    va2->map_attribute("vcolor", 4, GL_FLOAT, verts2);

    uint16_t *i = (uint16_t*)indices->get_buffer();
    for (int x=0; x<LINEBUFFER_MAX; x++) {
        int o = x*6;
        int vo = x*4;

        i[o+0] = vo;
        i[o+1] = vo+1;
        i[o+2] = vo+2;
        i[o+3] = vo;
        i[o+4] = vo+2;
        i[o+5] = vo+3;
    }

    indices->upload();

    reset();
}

void linebuffer::add(
            float x1, float y1, float z1,
            float x2, float y2, float z2,
            float r1, float g1, float b1, float a1,
            float r2, float g2, float b2, float a2,
            float w1, float w2
            )
{
    if (n < LINEBUFFER_MAX) {
        struct vert *_b = (struct vert*)verts->get_buffer();

        b2Vec2 tangent = b2Vec2((y2-y1), -(x2-x1));
        tangent *= 1.f / tangent.Length();

        b2Vec2 pos[4] = {
            b2Vec2(x2 + tangent.x*w2, y2 + tangent.y*w2),
            b2Vec2(x2 - tangent.x*w2, y2 - tangent.y*w2),
            b2Vec2(x1 - tangent.x*w1, y1 - tangent.y*w1),
            b2Vec2(x1 + tangent.x*w1, y1 + tangent.y*w1)
        };

        b2Vec2 uv[4] = {
            b2Vec2(1.f, 1.f),
            b2Vec2(0.f, 1.f),
            b2Vec2(0.f, 0.f),
            b2Vec2(1.f, 0.f)
        };

        for (int ix=0; ix<2; ix++) {
            _b[n*4+ix].pos.x = pos[ix].x - cam_x;
            _b[n*4+ix].pos.y = pos[ix].y - cam_y;
            _b[n*4+ix].pos.z = z1;

            _b[n*4+ix].color.r = r1;
            _b[n*4+ix].color.g = g1;
            _b[n*4+ix].color.b = b1;
            _b[n*4+ix].color.a = a1;

            _b[n*4+ix].uv.x = uv[ix].x;
            _b[n*4+ix].uv.y = uv[ix].y;
        }

        for (int ix=2; ix<4; ix++) {
            _b[n*4+ix].pos.x = pos[ix].x - cam_x;
            _b[n*4+ix].pos.y = pos[ix].y - cam_y;
            _b[n*4+ix].pos.z = z2;

            _b[n*4+ix].color.r = r2;
            _b[n*4+ix].color.g = g2;
            _b[n*4+ix].color.b = b2;
            _b[n*4+ix].color.a = a2;

            _b[n*4+ix].uv.x = uv[ix].x;
            _b[n*4+ix].uv.y = uv[ix].y;
        }
        n++;
    }
}

void linebuffer::add2(
            float x1, float y1, float z1,
            float x2, float y2, float z2,
            float r1, float g1, float b1, float a1,
            float r2, float g2, float b2, float a2,
            float w1, float w2
            )
{
    if (n2 < LINEBUFFER_MAX) {
        struct vert *_b = (struct vert*)verts2->get_buffer();

        b2Vec2 tangent = b2Vec2((y2-y1), -(x2-x1));
        tangent *= 1.f / tangent.Length();

        b2Vec2 pos[4] = {
            b2Vec2(x2 + tangent.x*w2, y2 + tangent.y*w2),
            b2Vec2(x2 - tangent.x*w2, y2 - tangent.y*w2),
            b2Vec2(x1 - tangent.x*w1, y1 - tangent.y*w1),
            b2Vec2(x1 + tangent.x*w1, y1 + tangent.y*w1)
        };

        b2Vec2 uv[4] = {
            b2Vec2(1.f, 1.f),
            b2Vec2(0.f, 1.f),
            b2Vec2(0.f, 0.f),
            b2Vec2(1.f, 0.f)
        };

        for (int ix=0; ix<2; ix++) {
            _b[n2*4+ix].pos.x = pos[ix].x - cam_x;
            _b[n2*4+ix].pos.y = pos[ix].y - cam_y;
            _b[n2*4+ix].pos.z = z1;

            _b[n2*4+ix].color.r = r1;
            _b[n2*4+ix].color.g = g1;
            _b[n2*4+ix].color.b = b1;
            _b[n2*4+ix].color.a = a1;

            _b[n2*4+ix].uv.x = uv[ix].x;
            _b[n2*4+ix].uv.y = uv[ix].y;
        }

        for (int ix=2; ix<4; ix++) {
            _b[n2*4+ix].pos.x = pos[ix].x - cam_x;
            _b[n2*4+ix].pos.y = pos[ix].y - cam_y;
            _b[n2*4+ix].pos.z = z2;

            _b[n2*4+ix].color.r = r2;
            _b[n2*4+ix].color.g = g2;
            _b[n2*4+ix].color.b = b2;
            _b[n2*4+ix].color.a = a2;

            _b[n2*4+ix].uv.x = uv[ix].x;
            _b[n2*4+ix].uv.y = uv[ix].y;
        }
        n2++;
    }
}

void linebuffer::upload()
{
    if (mesh) {
        mesh->i_start = 0;
        mesh->i_count = n*6;
    }
    if (mesh2) {
        mesh2->i_start = 0;
        mesh2->i_count = n2*6;
    }
    if (n) {
        verts->upload_partial(n*4*sizeof(struct vert));
    }
    if (n2) {
        verts2->upload_partial(n2*4*sizeof(struct vert));
    }
}

