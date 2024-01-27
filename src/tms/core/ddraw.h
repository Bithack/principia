#ifndef _TMS_DDRAW__H_
#define _TMS_DDRAW__H_

#include "tms/math/vector.h"

struct tms_texture;
struct tms_sprite;

/** @relates tms_ddraw @{ */

/**
 * A slow but very simple way of drawing primitives. Should only
 * be used for testing and debugging purposes.
 *
 * To draw primitives, first allocate a handle using tms_ddraw_alloc(). Next,
 * provide the ddraw handle with your camera's matrices using tms_ddraw_set_matrices().
 **/
struct tms_ddraw {
    float modelview[16];
    float projection[16];
    float tmp_modelview[16];
    float tmp_projection[16];
    int   primitive_type;
    tvec4 color;
};

/* XXX */
extern struct tms_mesh *tms_ddraw_circle_mesh;

struct tms_ddraw *tms_ddraw_alloc(void);
void tms_ddraw_init(struct tms_ddraw *d);
int tms_ddraw_sprite(struct tms_ddraw *d, struct tms_sprite *s, float x, float y, float width, float height);
int tms_ddraw_sprite_r(struct tms_ddraw *d, struct tms_sprite *s, float x, float y, float width, float height, float r);
int tms_ddraw_rsprite(struct tms_ddraw *d, struct tms_sprite *s, float x, float y, float width, float height, float border_outer);
int tms_ddraw_sprite1c(struct tms_ddraw *d, struct tms_sprite *s, float x, float y, float width, float height);
int        tms_ddraw_square(struct tms_ddraw *d, float x, float y, float width, float height);
int        tms_ddraw_square3d(struct tms_ddraw *d, float x, float y, float z, float width, float height);
int        tms_ddraw_circle(struct tms_ddraw *d, float x, float y, float width, float height);
int        tms_ddraw_line(struct tms_ddraw *d, float x1, float y1, float x2, float y2);
int        tms_ddraw_line3d(struct tms_ddraw *d, float x1, float y1, float z1, float x2, float y2, float z2);
void       tms_ddraw_set_matrices(struct tms_ddraw *d, float *mv, float *p);
void       tms_ddraw_set_color(struct tms_ddraw *d, float r, float g, float b, float a);
void tms_ddraw_set_rsprite_color(struct tms_ddraw *d, float r, float g, float b, float a);
void tms_ddraw_set_sprite1c_color(struct tms_ddraw *d, float r, float g, float b, float a);
int tms_ddraw_square_textured(struct tms_ddraw *d, float x, float y, float width, float height, struct tms_texture *texture);
int tms_ddraw_lcircle(struct tms_ddraw *d, float x, float y, float width, float height);
int tms_ddraw_lcircle3d(struct tms_ddraw *d, float x, float y, float z, float width, float height);
int tms_ddraw_lsquare(struct tms_ddraw *d, float x, float y, float width, float height);
int tms_ddraw_lsquare3d(struct tms_ddraw *d, float x, float y, float z, float width, float height);

#endif
