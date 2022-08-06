
#include "glob.h"

struct vertex3d {
    tvec3 pos;
    tvec3 nor;
    tvec2 tex;
};

struct tms_batch3d * tms_batch3d_alloc(void)
{
    struct tms_batch3d *b = calloc(1, sizeof(struct tms_batch3d));

    tms_batch3d_init(b);

    return b;
}

void
tms_batch3d_init(struct tms_batch3d *b)
{
    /*
    tms_entity_init(b);

    struct tms_varray *va = tms_varray_alloc(3);

    b->vertices = tms_gbuffer_alloc(1024*sizeof(struct vertex3d));
    b->indices = tms_gbuffer_alloc(1024*sizeof(short));

    b->cap_indices = 1024;
    b->cap_vertices = 1024;

    tms_varray_map_attribute(va, "position", 3, GL_FLOAT, b->vertices);
    tms_varray_map_attribute(va, "normal", 3, GL_FLOAT, b->vertices);
    tms_varray_map_attribute(va, "texcoord", 2, GL_FLOAT, b->vertices);

    b->super.mesh = tms_mesh_alloc(va, b->indices);

    struct tms_material *mat = tms_material_alloc();
    mat->shader = tms_get_shader("texture-light-shadow-alphatest");
    mat->flags |= TMS_MATERIAL_BLEND;
    mat->flags |= TMS_MATERIAL_FLAT;

    tms_entity_set_material(b, mat);
    */
}

int tms_batch3d_add_sprite(struct tms_batch3d *b, struct tms_sprite *sprite,
                           float x, float y, float z/*,
                           float *transform*/)
{
    if (b->num_indices+6 >= b->cap_indices) {
        tms_gbuffer_realloc(b->indices, b->cap_indices * 2 * sizeof(short));
        b->cap_indices *= 2;
    }
    if (b->num_vertices+4 >= b->cap_vertices) {
        tms_gbuffer_realloc(b->vertices, (b->cap_vertices * 2) * sizeof(struct vertex3d));
        b->cap_vertices *= 2;
    }

    struct vertex3d *vertices = tms_gbuffer_get_buffer(b->vertices);
    short *indices = tms_gbuffer_get_buffer(b->indices);

    vertices[b->num_vertices].pos = (tvec3){sprite->width/2 + x, sprite->height/2 + y, z};
    vertices[b->num_vertices].nor = (tvec3){0, 1, 0};
    vertices[b->num_vertices].tex = sprite->tr;

    vertices[b->num_vertices+1].pos = (tvec3){-sprite->width/2 + x, sprite->height/2 + y, z};
    vertices[b->num_vertices+1].nor = (tvec3){0, 1, 0};
    vertices[b->num_vertices+1].tex = (tvec2){sprite->bl.x, sprite->tr.y};

    vertices[b->num_vertices+2].pos = (tvec3){ -sprite->width/2 + x, -sprite->height/2 + y, z};
    vertices[b->num_vertices+2].nor = (tvec3){0, 1, 0};
    vertices[b->num_vertices+2].tex = sprite->bl;

    vertices[b->num_vertices+3].pos = (tvec3){ sprite->width/2 + x, -sprite->height/2 + y, z};
    vertices[b->num_vertices+3].nor = (tvec3){0, 1, 0};
    vertices[b->num_vertices+3].tex = (tvec2){sprite->tr.x, sprite->bl.y};

    indices[b->num_indices+0] = b->num_vertices;
    indices[b->num_indices+1] = b->num_vertices+1;
    indices[b->num_indices+2] = b->num_vertices+2;
    indices[b->num_indices+3] = b->num_vertices;
    indices[b->num_indices+4] = b->num_vertices+2;
    indices[b->num_indices+5] = b->num_vertices+3;

    b->num_vertices += 4;
    b->num_indices += 6;

    return T_OK;
}

void
tms_batch3d_set_texture(struct tms_batch3d *b, struct tms_texture *texture)
{
    //b->super.material->texture[0] = texture;
}

void
tms_batch3d_clear(struct tms_batch3d *b)
{
    b->num_vertices = 0;
    b->num_indices = 0;
    b->modified = 1;
}

int
tms_batch3d_commit(struct tms_batch3d *b)
{
    tms_gbuffer_upload_partial(b->vertices, b->num_vertices*sizeof(struct vertex3d));
    tms_gbuffer_upload_partial(b->indices, b->num_indices*sizeof(short));
    b->modified = 0;

    return T_OK;
}

int
tms_batch3d_render(struct tms_batch3d *b)
{
    static float mv[] = TMAT4_IDENTITY;

    if (b->num_vertices == 0)
        return T_NO_DATA;

    if (b->modified)
        tms_batch_commit(b);

    /*
    struct tms_shader *shader = b->super.material->shader;
    tms_shader_bind(shader);
    glUniform4f(b->color_loc, TVEC4_INLINE(b->color));
    tms_shader_set_matrices(shader, mv, tms.window_projection);
    tms_texture_bind(b->super.material->texture[0]);
    tms_mesh_render(b->super.mesh, shader);
    */

    return T_OK;
}


