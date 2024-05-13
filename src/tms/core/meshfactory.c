#include "meshfactory.h"
#include "mesh.h"
#include "gbuffer.h"
#include "varray.h"

#include <tms/math/vector.h>
#include <tms/backend/opengl.h>

#define SPHERE_QUALITY 40

static float square_verts[] = {
.5f, .5f, 0.f,
0.f, 0.f, 1.f,
1.f, 1.f,
-.5f, .5f, 0.f,
0.f, 0.f, 1.f,
0.f, 1.f,
-.5f, -.5f, 0.f,
0.f, 0.f, 1.f,
0.f, 0.f,
.5f, -.5f, 0.f,
0.f, 0.f, 1.f,
1.f, 0.f,
};

/* cube with normals and uv mapping */
static float cube_verts[] = {
    /* front */
    0.5f, 0.5f, 0.5f,
    1.f, 1.f,
    0, 0, 1,

    -0.5f, 0.5f, 0.5f,
    0.f, 1.f,
    0, 0, 1,

    -0.5f, -0.5f, 0.5f,
    0.f, 0.f,
    0, 0, 1,

    0.5f, -0.5f, 0.5f,
    1.f, 0.f,
    0, 0, 1,

    /* left */
    -0.5f, 0.5f, 0.5f,
    1.f, 1.f,
    -1, 0, 0,

    -0.5f, 0.5f, -0.5f,
    0.f, 1.f,
    -1, 0, 0,

    -0.5f, -0.5f, -0.5f,
    0.f, 0.f,
    -1, 0, 0,

    -0.5f, -0.5f,0.5f,
    1.f, 0.f,
    -1, 0, 0,

    /* right */
    0.5f, 0.5f, -0.5f,
    1.f, 1.f,
    1, 0, 0,

    0.5f, 0.5f, 0.5f,
    0.f, 1.f,
    1, 0, 0,

    0.5f, -0.5f, 0.5f,
    0.f, 0.f,
    1, 0, 0,

    0.5f, -0.5f, -0.5f,
    1.f, 0.f,
    1, 0, 0,

    /* top */
    0.5f, 0.5f, -0.5f,
    1.f, 1.f,
    0, 1, 0,

    -0.5f, 0.5f, -0.5f,
    0.f, 1.f,
    0, 1, 0,

    -0.5f, 0.5f, 0.5f,
    0.f, 0.f,
    0, 1, 0,

    0.5f, 0.5f, 0.5f,
    1.f, 0.f,
    0, 1, 0,

    /* bottom */
    0.5f, -0.5f, 0.5f,
    1.f, 1.f,
    0, -1, 0,

    -0.5f, -0.5f, 0.5f,
    0.f, 1.f,
    0, -1, 0,

    -0.5f, -0.5f, -0.5f,
    0.f, 0.f,
    0, -1, 0,

    0.5f, -0.5f, -0.5f,
    1.f, 0.f,
    0, -1, 0,

    /* back */
    -0.5f, 0.5f, -0.5f,
    1.f, 1.f,
    0, 0, -1,

    0.5f, 0.5f, -0.5f,
    0.f, 1.f,
    0, 0, -1,

    0.5f, -0.5f, -0.5f,
    0.f, 0.f,
    0, 0, -1,

    -0.5f, -0.5f, -0.5f,
    1.f, 0.f,
    0, 0, -1,
};

short cube_indices[] = {
    0, 1, 2, 0, 2, 3,
    4+0, 4+1, 4+2, 4+0, 4+2, 4+3,
    8+0, 8+1, 8+2, 8+0, 8+2, 8+3,
    12+0, 12+1, 12+2, 12+0, 12+2, 12+3,
    16+0, 16+1, 16+2, 16+0, 16+2, 16+3,
    20+0, 20+1, 20+2, 20+0, 20+2, 20+3,
};

static struct tms_mesh *square;
static struct tms_mesh *cube;
static struct tms_mesh *cylinder;

#define CQ 12

const struct tms_mesh *
tms_meshfactory_get_cylinder(void)
{
    if (cylinder == 0) {
        float step = (M_PI*2)/(float)CQ;

        struct cvert {
            tvec3 pos;
            tvec3 nor;
            tvec2 uv;
        };

        struct tms_gbuffer *vbuf = tms_gbuffer_alloc(sizeof(struct cvert) * CQ*4);
        //struct tms_gbuffer *ibuf = tms_gbuffer_alloc(sizeof(short) * (CQ*4 + 12));
        struct tms_gbuffer *ibuf = tms_gbuffer_alloc(sizeof(short) * (CQ*6+CQ*3+CQ*3));

        struct cvert *vertices = tms_gbuffer_get_buffer(vbuf);;
        short *indices = tms_gbuffer_get_buffer(ibuf);

        for (int x=0; x<CQ; x++) {
            vertices[x*2].pos = (tvec3){cosf(step*x), sinf(step*x), 1.f};
            vertices[x*2].nor = (tvec3){cosf(step*x), sinf(step*x), 0};
            vertices[x*2].uv = (tvec2){0,0};

            vertices[x*2+1].pos = (tvec3){cosf(step*x), sinf(step*x), -1.f};
            vertices[x*2+1].nor = (tvec3){cosf(step*x), sinf(step*x), 0};
            vertices[x*2+1].uv = (tvec2){0,0};

            vertices[x+CQ*2].pos = (tvec3){cosf(step*x), sinf(step*x), 1.f};
            vertices[x+CQ*2].nor = (tvec3){0, 0, 1.f};
            vertices[x+CQ*2].uv = (tvec2){0,0};

            vertices[x+CQ*3].pos = (tvec3){cosf(step*x), sinf(step*x), -1.f};
            vertices[x+CQ*3].nor = (tvec3){0, 0, -1.f};
            vertices[x+CQ*3].uv = (tvec2){0,0};
        }

        for (int x=0; x<CQ; x++) {
            int o = x*6;
            int xx=x*2;
            indices[o+0] = xx;
            indices[o+1] = (xx+1)%(CQ*2);
            indices[o+2] = (xx+2)%(CQ*2);
            indices[o+3] = (xx+1)%(CQ*2);
            indices[o+4] = (xx+3)%(CQ*2);
            indices[o+5] = (xx+2)%(CQ*2);
        }
        /*front*/
        int o = CQ*6;
        for (int x=0; x<CQ; x++) {
            indices[o+x*3+0] = CQ*3;
            indices[o+x*3+1] = CQ*3+(x+1)%CQ;
            indices[o+x*3+2] = CQ*3+(x+2)%CQ;
        }
        /* back side */
        o = CQ*6 + CQ*3;
        for (int x=0; x<CQ; x++) {
            indices[o+x*3+0] = CQ*2;
            indices[o+x*3+1] = CQ*2+(x+1)%CQ;
            indices[o+x*3+2] = CQ*2+(x+2)%CQ;
        }

        struct tms_varray *va = tms_varray_alloc(3);
        tms_varray_map_attribute(va, "position", 3, GL_FLOAT, vbuf);
        tms_varray_map_attribute(va, "normal", 3, GL_FLOAT, vbuf);
        tms_varray_map_attribute(va, "texcoord", 2, GL_FLOAT, vbuf);

        tms_gbuffer_upload(ibuf);
        tms_gbuffer_upload(vbuf);

        cylinder = tms_mesh_alloc(va, ibuf);
        tms_mesh_set_primitive_type(cylinder, TMS_TRIANGLES);
    }

    return cylinder;
}

const struct tms_mesh *tms_meshfactory_get_cube(void)
{
    if (cube == 0) {
        struct tms_gbuffer *indices = tms_gbuffer_alloc_fill(cube_indices, sizeof(cube_indices));
        struct tms_gbuffer *vertices = tms_gbuffer_alloc_fill(cube_verts, sizeof(cube_verts));
        struct tms_varray *va = tms_varray_alloc(3);
        tms_varray_map_attribute(va, "position", 3, GL_FLOAT, vertices);
        tms_varray_map_attribute(va, "texcoord", 2, GL_FLOAT, vertices);
        tms_varray_map_attribute(va, "normal", 3, GL_FLOAT, vertices);

        tms_gbuffer_upload(indices);
        tms_gbuffer_upload(vertices);

        cube = tms_mesh_alloc(va, indices);
        tms_mesh_set_autofree_buffers(cube, 1);
    }

    return cube;
}

const struct tms_mesh *tms_meshfactory_get_square(void)
{
    if (square == 0) {

        struct tms_gbuffer *vertices = tms_gbuffer_alloc_fill(square_verts, sizeof(square_verts));
        tms_gbuffer_upload(vertices);

        struct tms_varray *va = tms_varray_alloc(3);
        tms_varray_map_attribute(va, "position", 3, GL_FLOAT, vertices);
        tms_varray_map_attribute(va, "normal", 3, GL_FLOAT, vertices);
        tms_varray_map_attribute(va, "texcoord", 2, GL_FLOAT, vertices);

        square = tms_mesh_alloc(va, 0);
        tms_mesh_set_autofree_buffers(square, 1);
        tms_mesh_set_primitive_type(square, TMS_TRIANGLE_FAN);
    }

    return square;
}

