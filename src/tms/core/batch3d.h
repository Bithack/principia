#ifndef _TMS_BATCH3D__H_
#define _TMS_BATCH3D__H_

/** @relates tms_batch3d @{ **/

#include "entity.h"

struct tms_mesh;
struct tms_texture;
struct tms_sprite;
struct tms_gbuffer;

struct tms_batch3d {
    struct tms_entity super;
    int modified;
    int num_indices;
    int num_vertices;
    int cap_indices;
    int cap_vertices;
    int color_loc;
    tvec4 color;

    struct tms_gbuffer *vertices;
    struct tms_gbuffer *indices;
};

struct tms_batch3d *tms_batch3d_alloc(void);
void tms_batch3d_init(struct tms_batch3d *b);
void tms_batch3d_set_texture(struct tms_batch3d *batch, struct tms_texture *texture);
void tms_batch3d_clear(struct tms_batch3d *batch);
int tms_batch3d_commit(struct tms_batch3d *b);
int tms_batch3d_render(struct tms_batch3d *batch);
int tms_batch3d_add_sprite(struct tms_batch3d *b, struct tms_sprite *sprite, float x, float y, float z);

#endif
