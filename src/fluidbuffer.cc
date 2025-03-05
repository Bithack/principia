#include "fluidbuffer.hh"
#include "world.hh"

static tms::gbuffer *verts;
static tms::gbuffer *indices;
static tms::varray *va;
static tms::mesh *mesh;
static tms::entity *e;
static tms::entity *e2;

static uint32_t n = 0;

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
    tms_infof("Initializing fluidbuffer...");

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
}

void fluidbuffer::add(
    float x, float y, float z,
    //float r, float g, float b, float a,
    float pressure,
    float w, float h
) {
    uint32_t particle_limit = ((W->level.version <= LEVEL_VERSION_1_5_1) ? FLUIDBUFFER_MAX_1_5_1 : FLUIDBUFFER_MAX);
    if (n >= particle_limit) return;

    struct vert *vert_buf = (struct vert*) verts->get_buffer();
    for (int ix=0; ix<4; ix++) {
        vert_buf[n*4+ix] = base[ix];

        vert_buf[n*4+ix].pos.x *= w;
        vert_buf[n*4+ix].pos.y *= h;

        vert_buf[n*4+ix].pos.x += x;
        vert_buf[n*4+ix].pos.y += y;
        vert_buf[n*4+ix].pos.z += z;

        vert_buf[n*4+ix].uv.x += .5f*(3%2);
        vert_buf[n*4+ix].uv.y -= .25f*(3/2);
        vert_buf[n*4+ix].uv.z = pressure;
    }
    n++;
}

void fluidbuffer::upload()
{
    if (mesh) {
        mesh->i_start = 0;
        mesh->i_count = n*6;
    }
    if (n) verts->upload_partial(n*4*sizeof(struct vert));
}
