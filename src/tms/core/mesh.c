#include <stdlib.h>

#include "glob.h"

/**
 * Allocate space for a mesh.
 *
 * @relates tms_mesh
 **/
struct tms_mesh*
tms_mesh_alloc(struct tms_varray *va,
               struct tms_gbuffer *indices)
{
    int     x;
    struct tms_mesh *r;

    if (!(r = malloc(sizeof(struct tms_mesh))))
        return 0;

    tms_mesh_init(r, va, indices);

    return r;
}

void
tms_mesh_init(struct tms_mesh *r, struct tms_varray *va,
              struct tms_gbuffer *indices)
{
    r->vertex_array = va;
    r->indices = indices;
    r->primitive_type = GL_TRIANGLES;
    r->autofree_bufs = 0;
    r->i32 = 0;
    r->i_start = 0;
    r->i_count = -1;
    r->v_base = 0;
    r->cw = 0;
}

/**
 * Bind the shader program, all textures, and render the mesh.
 * Preferably, you should bind textures and shader programs manually
 * and call tms_mesh_draw() instead.
 *
 * @relates tms_mesh
 **/
int
tms_mesh_render(struct tms_mesh *m, struct tms_program *program)
{
    tms_program_bind(program);

    int *locations = tms_program_get_locations(program, m->vertex_array);
    tms_varray_bind_attributes(m->vertex_array, locations);

    tms_mesh_draw(m);

    tms_varray_unbind_attributes(m->vertex_array, locations);

    return T_OK;
}

/**
 * Draw the mesh, assume all necessary textures and the shader is bound.
 *
 * @relates tms_mesh
 **/
int
tms_mesh_draw(struct tms_mesh *m)
{
    if (m->indices) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m->indices->vbo);

        int start = m->i_start;
        int count = m->i_count;

        //tms_infof("start %d, count %d", start, count);

        if (count == -1) {
            count = m->indices->usize / (m->i32?sizeof(int):sizeof(short));
        }

        if (count) glDrawElements(
            m->primitive_type,
            count,
            (m->i32?GL_UNSIGNED_INT:GL_UNSIGNED_SHORT),
            (char*)(start * (m->i32?sizeof(int):sizeof(short)))
        );

        //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    } else
        glDrawArrays(m->primitive_type, 0, m->vertex_array->gbufs[0].gbuf->usize / m->vertex_array->gbufs[0].vsize);

    return T_OK;
}
