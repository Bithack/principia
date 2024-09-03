#include "ledbuffer.hh"
#include "material.hh"

#define RADIUS .04f

static tms::gbuffer *verts;
static tms::gbuffer *indices;
static tms::varray *va;
static tms::mesh *mesh;
static tms::entity *e;

static int n = 0;

static tvec4 base[9];

void ledbuffer::reset()
{
    n = 0;
}

tms::entity *
ledbuffer::get_entity()
{
    if (e) return e;

    e = new tms::entity();
    mesh = new tms::mesh(va, indices);

    e->prio = 0;
    e->set_mesh(mesh);
    e->set_material(&m_ledbuf);

    mesh->i_start = 0;
    mesh->i_count = n*7*3;

    return e;
}

void ledbuffer::_init()
{
    verts = new tms::gbuffer(9*LEDBUFFER_MAX*sizeof(tvec4));
    verts->usage = TMS_GBUFFER_STREAM_DRAW;

    indices = new tms::gbuffer(7*3*LEDBUFFER_MAX*sizeof(uint16_t));
    indices->usage = TMS_GBUFFER_STATIC_DRAW;

    va = new tms::varray(1);
    va->map_attribute("position", 4, GL_FLOAT, verts);

    uint16_t *i = (uint16_t*)indices->get_buffer();
    for (int x=0; x<LEDBUFFER_MAX; x++) {
        int o = x*7*3;
        int vo = x*9;

        for (int y=0; y<7; y++) {
            i[o+y*3] = vo+1+y;
            i[o+y*3+1] = vo+2+y;
            i[o+y*3+2] = vo+0;
        }
    }

    indices->upload();

    float step = (M_PI*2.f)/9.f;
    for (int x=0; x<9; x++) {
        float sx, sy;
        tmath_sincos((x)*step, &sy, &sx);
        sx *= RADIUS;
        sy *= RADIUS;
        base[x] = (tvec4){sx,sy,.25f, 0.f};
    }

    reset();
}

void
ledbuffer::add(float x, float y, float z, float col)
{
    if (n < LEDBUFFER_MAX) {
        col*=.8f;
        tvec4 *b = (tvec4*)verts->get_buffer();
        for (int ix=0; ix<9; ix++) {
            b[n*9+ix] = base[ix];

            b[n*9+ix].x += x;
            b[n*9+ix].y += y;
            b[n*9+ix].z += z;
            b[n*9+ix].w += col;
        }
        n++;
    }
}

void ledbuffer::upload()
{
    if (mesh) {
        mesh->i_start = 0;
        mesh->i_count = n*7*3;
    }
    if (n)
        verts->upload_partial(n*9*sizeof(tvec4));
}

