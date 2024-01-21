#include <stdlib.h>

#include "../math/matrix.h"
#include "camera.h"
#include "tms.h"

/**
 * Allocate a camera struct.
 **/
struct tms_camera* tms_camera_alloc(void)
{
    struct tms_camera *c = malloc(sizeof(struct tms_camera));
    if (c) tms_camera_init(c);
    return c;
}

void
tms_camera_init(struct tms_camera *c)
{
    tmat4_load_identity(c->view);
    tmat4_load_identity(c->projection);
    tmat4_load_identity(c->combined);

    c->_velocity = (tvec3){0,0,0};
    c->_position = (tvec3){0,0,0};
    c->_direction = (tvec3){0,0,0};
    c->_lookat = (tvec3){0,0,0};
    c->_flags = 0;

    c->fov = 55.f;
    c->near = 1.f;
    c->far = -100.f;
    c->velocity_damping = 0;
    c->up = (tvec3){0,1,0};

    c->width = tms.window_width;
    c->height = tms.window_height;

    c->_direction.z = -1;
}

tvec3
tms_camera_project(struct tms_camera *c, float x, float y, float z)
{
    tvec4 v = (tvec4){x,y,z,1.f};
    tvec4_mul_mat4(&v, c->combined);

    v.x /= v.w;
    v.y /= v.w;
    v.z /= v.w;

    float w = c->_flags & TMS_CAMERA_PERSPECTIVE ? c->width : c->owidth;
    float h = c->_flags & TMS_CAMERA_PERSPECTIVE ? c->height : c->oheight;

    v.x = w * (v.x + 1.f) / 2.f;
    v.y = h * (v.y + 1.f) / 2.f;
    v.z = (v.z + 1) / 2;

    tvec3 r;
    r.x = v.x;
    r.y = v.y;
    r.z = v.z;
    return r;
}

tvec3
tms_camera_unproject(struct tms_camera *c, float x, float y, float z)
{
    tvec4 v = (tvec4){0,0,0,0};
    float mat[16];
    tmat4_copy(mat, c->combined);
    tmat4_invert(mat);

    if (c->_flags & TMS_CAMERA_PERSPECTIVE) {
        v.x = (x / (float)c->width) * 2.f - 1.f;
        v.y = (y / (float)c->height) * 2.f - 1.f;
        v.z = 2.f*z-1.f;
        v.w = 1.f;
    } else {
        v.x = (x / (float)c->owidth) * 2.f - 1.f;
        v.y = (y / (float)c->oheight) * 2.f - 1.f;
        v.z = 2.f*z-1.f;
        v.w = 1.f;
    }

    tvec4_mul_mat4(&v, mat);

    v.x /= v.w;
    v.y /= v.w;
    v.z /= v.w;
    v.w = 1.f;

    return (tvec3){v.x, v.y, v.z};
}

void
tms_camera_disable(struct tms_camera *c, uint32_t flag)
{
    c->_flags &= ~flag;
}

void
tms_camera_enable(struct tms_camera *c, uint32_t flag)
{
    c->_flags |= flag;
}

int
tms_camera_enabled(struct tms_camera *c, uint32_t flag)
{
    return c->_flags & flag;
}

void
tms_camera_set_direction(struct tms_camera *c, float x, float y, float z)
{
    c->_direction.x = x;
    c->_direction.y = y;
    c->_direction.z = z;
    tvec3_normalize(&c->_direction);
}

/**
 * Set the position of the camera.
 * Use TVEC3_INLINE() to expand a tvec3. The position is relative
 * to the world origin.
 **/
void
tms_camera_set_position(struct tms_camera *c, float x, float y, float z)
{
    c->_position.x = x;
    c->_position.y = y;
    c->_position.z = z;
}

/**
 * Move the camera relative to its current position.
 * Also see tms_camera_set_position
 **/
void
tms_camera_translate(struct tms_camera *c, float x, float y, float z)
{
    c->_position.x += x;
    c->_position.y += y;
    c->_position.z += z;
}

void
tms_camera_step(struct tms_camera *c, float dt)
{
    if (c->_flags & TMS_CAMERA_VELOCITY) {
        c->_position.x += c->_velocity.x*dt;
        c->_position.y += c->_velocity.y*dt;
        c->_position.z += c->_velocity.z*dt;
    }
}

/**
 * Move the camera towards a point.
 * Smoothly interpolates the the camera's position towards
 * the given point x, y, z, by a factory specified using factor_*
 **/
void
tms_camera_confine(struct tms_camera *c,
        float x, float y, float z,
        float factor_x, float factor_y, float factor_z)
{
    tvec3 norm = (tvec3){x - c->_position.x, y - c->_position.y, z-c->_position.z};
    float len = tvec3_magnitude(&norm);
    tvec3_normalize(&norm);

    len*=len;

    norm.x *= len*factor_x;
    norm.y *= len*factor_y;
    norm.z *= len*factor_z;

    tvec3_add(&c->_position, &norm);
}

void
tms_camera_set_lookat(struct tms_camera *c, float x, float y, float z)
{
    c->_lookat = (tvec3){x,y,z};
}

/**
 * Update the camera's matrices.
 *
 * Call this function to update the combined, view and projection
 * matrices.
 **/
void
tms_camera_calculate(struct tms_camera *c)
{
    tvec3 rdir;

    if (c->_flags & TMS_CAMERA_PERSPECTIVE) {
        tmat4_perspective(c->projection, c->fov, c->width/c->height,
                               c->near, c->far);
    } else {
        /* orthographic camera */
        tmat4_set_ortho(c->projection,
                        -c->width/2.f, c->width/2.f,
                        -c->height/2.f, c->height/2.f,
                        c->near, c->far);
    }

    if (c->_flags & TMS_CAMERA_LOOKAT) {
        rdir = c->_lookat;
    } else {
        rdir = (tvec3) {
            c->_position.x+c->_direction.x,
            c->_position.y+c->_direction.y,
            c->_position.z+c->_direction.z
        };
    }

    tmat4_lookat(c->view,
            TVEC3_INLINE(c->_position),
            TVEC3_INLINE(rdir),
            TVEC3_INLINE(c->up)
            );

    tmat4_copy(c->combined, c->projection);

    tmat4_multiply(c->combined, c->view);
}

