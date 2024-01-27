#ifndef _TMS_MESH__H_
#define _TMS_MESH__H_

/** @relates tms_mesh @{ */

#include <tms/core/settings.h>
#include <tms/backend/opengl.h>

#define TMS_TRIANGLE_FAN   GL_TRIANGLE_FAN
#define TMS_TRIANGLES      GL_TRIANGLES
#define TMS_TRIANGLE_STRIP GL_TRIANGLE_STRIP
#define TMS_LINES          GL_LINES
#define TMS_LINE_LOOP      GL_LINE_LOOP

struct tms_shader;
struct tms_gbuffer;
struct tms_varray;
struct tms_program;

struct tms_mesh {
    struct tms_varray *vertex_array;
    struct tms_gbuffer *indices;
    int primitive_type;
    int autofree_bufs;
    int i32;
    int cw;
    int i_start; /* in elements */
    int i_count;
    int v_base;
    int id; /* Unique identifier of mesh, optional */

    /* if loaded to a mesh */
    struct tms_model *owner;
    int v_start; /* in bytes */
    int v_count; /* in sizeof(struct vertex) */
};

struct tms_mesh* tms_mesh_alloc(struct tms_varray *vertex_array, struct tms_gbuffer *indices);
void tms_mesh_init(struct tms_mesh *r, struct tms_varray *va, struct tms_gbuffer *indices);
void      tms_mesh_free(struct tms_mesh *m);
int       tms_mesh_render(struct tms_mesh *m, struct tms_program *program);
int       tms_mesh_draw(struct tms_mesh *m);

static inline void tms_mesh_set_primitive_type(struct tms_mesh *m, int type)
{
    m->primitive_type = type;
}

static inline void tms_mesh_set_autofree_buffers(struct tms_mesh *m, int f)
{
    m->autofree_bufs = f;
}

#endif
