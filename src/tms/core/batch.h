#ifndef _TMS_BATCH__H_
#define _TMS_BATCH__H_

/** @relates tms_batch @{ **/

#include "entity.h"

struct tms_mesh;
struct tms_texture;
struct tms_sprite;
struct tms_gbuffer;

/** 
 * Batched quad rendering.
 *
 * This very simple batch object can be used to render many quads on the screen.
 * Each quad will be textured by a sprite created from a texture atlas. Only
 * one atlas can be used per render call.
 *
 * Internally, a single mesh is used and each quad is rendered as two triangles.
 * By default, the batch will use the projection matrix from tms.window_projection,
 * which orthographic 2d rendering.
 *
 * The batch will stay intact until tms_batch_clear is called. This means you can
 * create the data once and then render it many times. Alternatively, call
 * tms_batch_clear every frame if your data changes often.
 *
 * Since tms_batch extends entity, you can add it to a scene. If added to a scene,
 * the scene will manage the batch's matrices.
 **/
struct tms_batch {
    struct tms_entity super;
    int modified;
    int num_indices;
    int num_vertices;
    int color_loc;
    tvec4 color;

    struct tms_gbuffer *vertices;
    struct tms_gbuffer *indices;
};

struct tms_batch *tms_batch_alloc(void);
void tms_batch_set_texture(struct tms_batch *batch, struct tms_texture *texture);
void tms_batch_clear(struct tms_batch *batch);
int tms_batch_commit(struct tms_batch *b);
int tms_batch_render(struct tms_batch *batch);
int tms_batch_add_sprite(struct tms_batch *b, struct tms_sprite *sprite, float x, float y);

static inline void tms_batch_set_color(struct tms_batch *ba, float r, float g, float b, float a)
{
    ba->color = tvec4f(r,g,b,a);
}

#endif
