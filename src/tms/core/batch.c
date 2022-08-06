#include "glob.h"

struct vertex2d {
    tvec2 pos;
    tvec2 tex;
};

/** 
 * Allocate a quad batch optimized for 2d rendering.
 *
 * @relates tms_batch
 **/
struct tms_batch *tms_batch_alloc(void)
{
    struct tms_batch *b = calloc(1, sizeof(struct tms_batch));

    tms_entity_init(b);

    struct tms_varray *va = tms_varray_alloc(2);

    b->vertices = tms_gbuffer_alloc(2048*4*sizeof(float));
    b->indices = tms_gbuffer_alloc(2048*sizeof(short));

    tms_varray_map_attribute(va, "position", 2, GL_FLOAT, b->vertices);
    tms_varray_map_attribute(va, "texcoord", 2, GL_FLOAT, b->vertices);

    b->super.mesh = tms_mesh_alloc(va, b->indices);

    /*
    struct tms_material *mat = tms_material_alloc();
    mat->shader = tms_get_shader("2d-texture-tint");
    mat->flags |= TMS_MATERIAL_BLEND;

    tms_entity_set_material(b, mat);
    */

    //b->color_loc = glGetUniformLocation(mat->shader->program, "color");
    //tms_batch_set_color(b, 1.f, 1.f, 1.f, 1.f);

    return b;
}

int tms_batch_add_sprite(struct tms_batch *b, struct tms_sprite *sprite, float x, float y)
{
    struct vertex2d *vertices = tms_gbuffer_get_buffer(b->vertices);
    short *indices = tms_gbuffer_get_buffer(b->indices);

    vertices[b->num_vertices].pos = (tvec2){sprite->width + x, sprite->height + y};
    vertices[b->num_vertices].tex = sprite->tr;

    vertices[b->num_vertices+1].pos = (tvec2){x, sprite->height + y};
    vertices[b->num_vertices+1].tex = (tvec2){sprite->bl.x, sprite->tr.y};

    vertices[b->num_vertices+2].pos = (tvec2){x, y};
    vertices[b->num_vertices+2].tex = sprite->bl;

    vertices[b->num_vertices+3].pos = (tvec2){ sprite->width + x, y};
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
tms_batch_set_texture(struct tms_batch *b, struct tms_texture *texture)
{
    //b->super.material->texture[0] = texture;
}

void
tms_batch_clear(struct tms_batch *b)
{
    b->num_vertices = 0;
    b->num_indices = 0;
    b->modified = 1;
}

/** 
 * If the tms_batch is being used as an entity in a scene,
 * commit must be called after modification is done.
 *
 * @relates tms_batch
 **/
int
tms_batch_commit(struct tms_batch *b)
{
    tms_gbuffer_upload_partial(b->vertices, b->num_vertices*sizeof(struct vertex2d));
    tms_gbuffer_upload_partial(b->indices, b->num_indices*sizeof(short));
    b->modified = 0;

    return T_OK;
}

int
tms_batch_render(struct tms_batch *b)
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

