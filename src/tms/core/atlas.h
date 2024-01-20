#ifndef _TMS_TEXTUREATLAS__H_
#define _TMS_TEXTUREATLAS__H_

#include "texture.h"
#include <tms/math/vector.h>

/** @relates tms_atlas @{ */

struct tms_atlas {
    struct tms_texture texture;
    int padding_x;
    int padding_y;
    int cell_width; /**< optional fixed width */
    int cell_height; /**< optional fixed height */

    int current_x;
    int current_y;
    int current_height;
};

struct tms_sprite {
    tvec2 bl, tr;
    float width, height;
};

struct tms_atlas *tms_atlas_alloc(int width, int height, int num_channels);
struct tms_sprite * tms_atlas_add_bitmap(struct tms_atlas *t, int width, int height, int num_channels, unsigned char *data);
struct tms_sprite * tms_atlas_add_file(struct tms_atlas *t, const char *filename, int invert_y);
void tms_atlas_free(struct tms_atlas* a);

static inline void tms_atlas_reset(struct tms_atlas *t)
{
    t->current_x = 0;
    t->current_y = 0;
    t->current_height = 0;
}

#endif
