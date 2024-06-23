#include <stdlib.h>

#include <tms/math/intersect.h>
#include <tms/backend/opengl.h>

#include "tms.h"
#include "screen.h"
#include "event.h"
#include "scene.h"
#include "surface.h"

struct tms_screen*
tms_screen_alloc(struct tms_screen_spec *spec)
{
    struct tms_screen *r;

    if (!(r = calloc(1, sizeof(struct tms_screen))))
        return 0;

    r->spec = spec;

    if (spec->flags & TMS_SCREEN_ALLOC_SCENE)
        r->scene = tms_scene_alloc();

    if (spec->flags & TMS_SCREEN_ALLOC_SURFACE)
        r->surface = tms_surface_alloc();

    if (spec->alloc_data)
        r->data = spec->alloc_data(r);

    return r;
}

    /*
void
tms_sceen_free(struct tms_screen *s)
{
    if (s->spec->flags & TMS_SCREEN_ALLOC_SCENE)
        tms_scene_free(s->scene);

    if (s->spec->flags & TMS_SCREEN_ALLOC_SURFACE)
        tms_surface_free(s->scene);
}
    */

/**
 * Get the active surface of this screen.
 *
 * @relates tms_screen
 **/
struct tms_surface *tms_screen_get_surface(struct tms_screen *s)
{
    return s->surface;
}

/**
 * Get the active scene of this screen.
 *
 * @relates tms_screen
 **/
struct tms_scene *tms_screen_get_scene(struct tms_screen *s)
{
    return s->scene;
}

/**
 * Get the data that was allocated by the `tms_screen_spec`s `alloc_data` callback.
 *
 * @relates tms_screen
 **/
void *
tms_screen_get_data(struct tms_screen *s)
{
    return s->data;
}

/**
 * Set the screen' scene..
 * Only valid if the screen was not created with TMS_SCREEN_ALLOC_SCENE.
 *
 * @returns T_ERR on error, T_OK otherwise.
 *
 * @relates tms_screen
 **/
int
tms_screen_set_scene(struct tms_screen *s, struct tms_scene *scene)
{
    if (s->spec->flags & TMS_SCREEN_ALLOC_SCENE)
        return T_ERR;

    s->scene = scene;

    return T_OK;
}

/**
 * Set the screen's surface.
 * Can only be called if the screen did not allocate its own surface
 * according to the tms_screen_spec.
 *
 * @returns T_ERR if the spec's flags include TMS_SCREEN_ALLOC_SURFACE. T_OK otherwise.
 *
 * @relates tms_screen
 **/
int
tms_screen_set_surface(struct tms_screen *s, struct tms_surface *surf)
{
    if (s->spec->flags & TMS_SCREEN_ALLOC_SURFACE)
        return T_ERR;

    s->surface = surf;

    return T_OK;
}

/**
 * Intermediate function between the specs input callback
 * and the internal input system.
 *
 * @relates tms_screen
 **/
int
tms_screen_handle_input(struct tms_screen *s,
                        const struct tms_event *ev,
                        int action)
{
    if (s->surface != 0 && ev->type & TMS_EV_MASK_POINTER)
        if (tms_surface_handle_input(s->surface, ev, action) == T_OK)
            return T_OK;

    return T_CONT;
}

/**
 * @relates tms_screen
 **/
int
tms_screen_end_frame(struct tms_screen *s)
{
    if (s->spec->end_frame)
        return s->spec->end_frame(s);

    return T_OK;
}

/**
 * @relates tms_screen
 **/
int
tms_screen_begin_frame(struct tms_screen *s)
{
    if (s->spec->begin_frame)
        return s->spec->begin_frame(s);

    return T_OK;
}

/**
 * Called by tms_render each frame.
 * Will forward the rendering to the function defined by the screen's spec.
 *
 * @relates tms_screen
 **/
int
tms_screen_render(struct tms_screen *s)
{
    if (s->spec->render)
        s->spec->render(s);

    if (s->surface != 0)
        tms_surface_render(s->surface);

    if (s->spec->post_render)
        s->spec->post_render(s);

    return T_OK;
}

/**
 * Called by tms_render each frame
 *
 * @relates tms_screen
 **/
int
tms_screen_step(struct tms_screen *s, double dt)
{
    if (s->spec->step)
        s->spec->step(s, dt);

    return T_OK;
}

