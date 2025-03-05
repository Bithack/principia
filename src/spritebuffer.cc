#include "spritebuffer.hh"

static tms::gbuffer *verts;
static tms::gbuffer *verts2;
static tms::gbuffer *indices;
static tms::varray *va;
static tms::varray *va2;
static tms::mesh *mesh;
static tms::mesh *mesh2;
static tms::entity *e;
static tms::entity *e2;

static int n = 0;
static int n2 = 0;

struct vert {
    tvec3 pos;
    tvec2 uv;
    tvec4 color;
};

static struct vert base[4];

void spritebuffer::reset()
{
    n = 0;
    n2 = 0;
}

tms::entity *
spritebuffer::get_entity()
{
    if (e) return e;

    e = new tms::entity();
    mesh = new tms::mesh(va, indices);

    e->prio = 0;
    e->set_mesh(mesh);
    e->set_material(&m_spritebuf);

    mesh->i_start = 0;
    mesh->i_count = n*9*3;

    return e;
}

tms::entity *
spritebuffer::get_entity2()
{
    if (e2) return e2;

    e2 = new tms::entity();
    mesh2 = new tms::mesh(va2, indices);

    e2->prio = 0;
    e2->set_mesh(mesh2);
    e2->set_material(&m_spritebuf2);

    mesh2->i_start = 0;
    mesh2->i_count = n2*9*3;

    return e2;
}

void spritebuffer::_init()
{
    tms_infof("Initializing spritebuffer...");

    verts = new tms::gbuffer(4*SPRITEBUFFER_MAX*sizeof(struct vert));
    verts->usage = TMS_GBUFFER_STREAM_DRAW;

    verts2 = new tms::gbuffer(4*SPRITEBUFFER_MAX*sizeof(struct vert));
    verts2->usage = TMS_GBUFFER_STREAM_DRAW;

    indices = new tms::gbuffer(6*SPRITEBUFFER_MAX*sizeof(uint16_t));
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
    for (int x=0; x<SPRITEBUFFER_MAX; x++) {
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

    base[0] = (struct vert){
        (tvec3){.5f,.5f,0.f},
        (tvec2){.5f, 1.f},
        (tvec4){0.f, 0.f, 0.f, 0.f},
    };
    base[1] = (struct vert){
        (tvec3){-.5f,.5f,0.f},
        (tvec2){0.f, 1.f},
        (tvec4){0.f, 0.f, 0.f, 0.f},
    };
    base[2] = (struct vert){
        (tvec3){-.5f,-.5f,0.f},
        (tvec2){0.f, .75f},
        (tvec4){0.f, 0.f, 0.f, 0.f},
    };
    base[3] = (struct vert){
        (tvec3){.5f,-.5f,0.f},
        (tvec2){.5f, .75f},
        (tvec4){0.f, 0.f, 0.f, 0.f},
    };

    reset();
}

void
spritebuffer::add(float x, float y, float z,
        float r, float g, float b, float a,
        float w, float h, int sprite, float rot)
{
    float cs, sn;
    tmath_sincos(rot, &sn, &cs);

    if (n < SPRITEBUFFER_MAX) {
        struct vert *_b = (struct vert*)verts->get_buffer();
        for (int ix=0; ix<4; ix++) {
            _b[n*4+ix] = base[ix];

            _b[n*4+ix].pos.x *= w;
            _b[n*4+ix].pos.y *= h;

            float _x = _b[n*4+ix].pos.x * cs - _b[n*4+ix].pos.y * sn;
            float _y = _b[n*4+ix].pos.x * sn + _b[n*4+ix].pos.y * cs;
            _b[n*4+ix].pos.x = _x;
            _b[n*4+ix].pos.y = _y;

            _b[n*4+ix].pos.x += x;
            _b[n*4+ix].pos.y += y;
            _b[n*4+ix].pos.z += z;

            _b[n*4+ix].color.r = r;
            _b[n*4+ix].color.g = g;
            _b[n*4+ix].color.b = b;
            _b[n*4+ix].color.a = a;

            _b[n*4+ix].uv.x += .5f*(sprite%2);
            _b[n*4+ix].uv.y -= .25f*(sprite/2);
        }
        n++;
    }
}

void
spritebuffer::add(float x, float y, float z,
        float r, float g, float b, float a,
        float w, float h, int sprite)
{
    if (n < SPRITEBUFFER_MAX) {
        struct vert *_b = (struct vert*)verts->get_buffer();
        for (int ix=0; ix<4; ix++) {
            _b[n*4+ix] = base[ix];

            _b[n*4+ix].pos.x *= w;
            _b[n*4+ix].pos.y *= h;

            _b[n*4+ix].pos.x += x;
            _b[n*4+ix].pos.y += y;
            _b[n*4+ix].pos.z += z;

            _b[n*4+ix].color.r = r;
            _b[n*4+ix].color.g = g;
            _b[n*4+ix].color.b = b;
            _b[n*4+ix].color.a = a;

            _b[n*4+ix].uv.x += .5f*(sprite%2);
            _b[n*4+ix].uv.y -= .25f*(sprite/2);
        }
        n++;
    }
}

void
spritebuffer::add2(float x, float y, float z,
        float r, float g, float b, float a,
        float w, float h, int sprite, float rot)
{
    float cs, sn;
    tmath_sincos(rot, &sn, &cs);

    if (n2 < SPRITEBUFFER_MAX) {
        struct vert *_b = (struct vert*)verts2->get_buffer();
        for (int ix=0; ix<4; ix++) {
            _b[n2*4+ix] = base[ix];

            _b[n2*4+ix].pos.x *= w;
            _b[n2*4+ix].pos.y *= h;

            float _x = _b[n2*4+ix].pos.x * cs - _b[n2*4+ix].pos.y * sn;
            float _y = _b[n2*4+ix].pos.x * sn + _b[n2*4+ix].pos.y * cs;
            _b[n2*4+ix].pos.x = _x;
            _b[n2*4+ix].pos.y = _y;

            _b[n2*4+ix].pos.x += x;
            _b[n2*4+ix].pos.y += y;
            _b[n2*4+ix].pos.z += z;

            _b[n2*4+ix].color.r = r;
            _b[n2*4+ix].color.g = g;
            _b[n2*4+ix].color.b = b;
            _b[n2*4+ix].color.a = a;

            _b[n2*4+ix].uv.x += .5f*(sprite%2);
            _b[n2*4+ix].uv.y -= .25f*(sprite/2);
        }
        n2++;
    }
}

void
spritebuffer::add2(float x, float y, float z,
        float r, float g, float b, float a,
        float w, float h, int sprite)
{
    if (n2 < SPRITEBUFFER_MAX) {
        struct vert *_b = (struct vert*)verts2->get_buffer();
        for (int ix=0; ix<4; ix++) {
            _b[n2*4+ix] = base[ix];

            _b[n2*4+ix].pos.x *= w;
            _b[n2*4+ix].pos.y *= h;

            _b[n2*4+ix].pos.x += x;
            _b[n2*4+ix].pos.y += y;
            _b[n2*4+ix].pos.z += z;

            _b[n2*4+ix].color.r = r;
            _b[n2*4+ix].color.g = g;
            _b[n2*4+ix].color.b = b;
            _b[n2*4+ix].color.a = a;

            _b[n2*4+ix].uv.x += .5f*(sprite%2);
            _b[n2*4+ix].uv.y -= .25f*(sprite/2);
        }
        n2++;
    }
}

void spritebuffer::upload()
{
    if (mesh) {
        mesh->i_start = 0;
        mesh->i_count = n*6;
    }
    if (mesh2) {
        mesh2->i_start = 0;
        mesh2->i_count = n2*6;
    }
    if (n) verts->upload_partial(n*4*sizeof(struct vert));
    if (n2) verts2->upload_partial(n2*4*sizeof(struct vert));
}

