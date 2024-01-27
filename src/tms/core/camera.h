#ifndef _TMS_CAMERA__H_
#define _TMS_CAMERA__H_

/** @relates tms_camera @{ */

#include <stdint.h>
#include <tms/math/vector.h>

#define TMS_CAMERA_VELOCITY    1 /**< Set to enable camera velocity. lol jk this isnt even implemented yet */
#define TMS_CAMERA_PERSPECTIVE 2 /**< Enable perspective camera. If unset, the camera is orthographic. */
#define TMS_CAMERA_LOOKAT      4 /**< If set, the camera's direction is calculated from a point given to tms_camera_lookat, otherwise the camera's direction is set using tms_camera_set_direction */

/**
 * Perspective/ortho2d camera convenience stuff.
 *
 * tms_camera will generate projection and view matrices
 * for you.
 **/
struct tms_camera {
    /* call tms_camera_calculate() before fetching these */
    float             view[16];
    float             projection[16];
    float             combined[16];

    tvec3              _velocity;
    tvec3              _position;
    tvec3              _direction;
    tvec3              _lookat;
    tvec3              up;
    uint32_t          _flags;
    float             fov;
    float             aspect;
#ifdef TMS_BACKEND_WINDOWS
#undef near
#undef far
#endif
    float             near;
    float             far;
    float             velocity_damping;

    float width;
    float height;

    float owidth;
    float oheight;
};

struct tms_camera* tms_camera_alloc(void);
void tms_camera_init(struct tms_camera *c);
void tms_camera_set_position(struct tms_camera *cam, float x, float y, float z);
void tms_camera_translate(struct tms_camera *c, float x, float y, float z);
void tms_camera_confine(struct tms_camera *c, float x, float y, float z, float factor_x, float factor_y, float factor_z);
void tms_camera_set_lookat(struct tms_camera *c, float x, float y, float z);
void tms_camera_enable(struct tms_camera *cam, uint32_t flag);
int tms_camera_enabled(struct tms_camera *cam, uint32_t flag);
void tms_camera_disable(struct tms_camera *cam, uint32_t flag);
void tms_camera_free(struct tms_camera *cam);
void tms_camera_set_direction(struct tms_camera *c, float x, float y, float z);
void tms_camera_calculate(struct tms_camera *c);

tvec3 tms_camera_unproject(struct tms_camera *c, float x, float y, float z);
tvec3 tms_camera_project(struct tms_camera *c, float x, float y, float z);

#endif
