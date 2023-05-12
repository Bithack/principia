#include "fluidbuffer.hh"

static tms::gbuffer *verts;
static tms::gbuffer *indices;
static tms::varray *va;
static tms::mesh *mesh;
static tms::entity *e;
static tms::entity *e2;

static int n = 0;

struct vert {
    tvec3 pos;
    tvec3 uv;
};

static struct vert base[4];

void fluidbuffer::reset()
{
    n = 0;
}

tms::entity *
fluidbuffer::get_entity()
{
    if (e) return e;

    mesh = new tms::mesh(va, indices);
    mesh->i32 = 1;
    
    e = new tms::entity();
    e->prio = 0;
    e->set_mesh(mesh);
    e->set_material(&m_fluidbuf);

    mesh->i_start = 0;
    mesh->i_count = n*9*3;

    return e;
}
void fluidbuffer::_init()
{
    tms_progressf("Initializing fluidbuffer... ");

    verts = new tms::gbuffer(4*(FLUIDBUFFER_MAX)*sizeof(struct vert));
    verts->usage = TMS_GBUFFER_STREAM_DRAW;

    indices = new tms::gbuffer(6*FLUIDBUFFER_MAX*sizeof(uint32_t));
    indices->usage = TMS_GBUFFER_STATIC_DRAW;
    
    va = new tms::varray(2);
    va->map_attribute("position", 3, GL_FLOAT, verts);
    va->map_attribute("uv", 3, GL_FLOAT, verts);

    uint32_t *i = (uint32_t*)indices->get_buffer();
    for (uint32_t x=0; x<FLUIDBUFFER_MAX; x++) {
        uint32_t o = x*6;
        uint32_t vo = x*4;

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
        (tvec3){.5f, 1.f, 0.f}
    };
    base[1] = (struct vert){
        (tvec3){-.5f,.5f,0.f},
        (tvec3){0.f, 1.f, 0.f},
    };
    base[2] = (struct vert){
        (tvec3){-.5f,-.5f,0.f},
        (tvec3){0.f, .75f, 0.f},
    };
    base[3] = (struct vert){
        (tvec3){.5f,-.5f,0.f},
        (tvec3){.5f, .75f, 0.f},
    };

    reset();

    tms_progressf("OK\n");
}

/*
void
fluidbuffer::add(float x, float y, float z,
        float r, float g, float b, float a,
        float w, float h)
        */
void
fluidbuffer::add(float x, float y, float z,
        //float r, float g, float b, float a,
        float pressure,
        float w, float h)
{
    if (n < FLUIDBUFFER_MAX) {
        struct vert *_b = (struct vert*)verts->get_buffer();
        for (int ix=0; ix<4; ix++) {
            _b[n*4+ix] = base[ix];

            _b[n*4+ix].pos.x *= w;
            _b[n*4+ix].pos.y *= h;

            _b[n*4+ix].pos.x += x;
            _b[n*4+ix].pos.y += y;
            _b[n*4+ix].pos.z += z;

            /*
            _b[n*4+ix].color.r = r;
            _b[n*4+ix].color.g = g;
            _b[n*4+ix].color.b = b;
            _b[n*4+ix].color.a = a;
            */

            _b[n*4+ix].uv.x += .5f*(3%2);
            _b[n*4+ix].uv.y -= .25f*(3/2);
            _b[n*4+ix].uv.z = pressure;
        }
        n++;
    }
}

void fluidbuffer::upload()
{
    if (mesh) {
        mesh->i_start = 0;
        mesh->i_count = n*6;
    }
    if (n) verts->upload_partial(n*4*sizeof(struct vert));
}
