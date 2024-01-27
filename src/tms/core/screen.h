#ifndef _TMS_SCREEN__H_
#define _TMS_SCREEN__H_

#include <stdint.h>
#include "../math/vector.h"

/** @relates tms_screen @{ */

struct tms_screen_spec;
struct tms_event;
struct tms_scene;
struct tms_surface;

/**
 * Screen flags that will be chosen in the tms_screen_spec.
 **/
enum {
    /**
     * Should this screen allocate/free a scene automatically?
     * If set, the screen will allocate a scene in tms_screen->scene.
     * tms_entity's can be added/removed from this scene dynamically. When
     * the screen is destroyed, the scene is also destroyed.
     **/
    TMS_SCREEN_ALLOC_SCENE = 1,

    /**
     * Should this screen allocate/free a widget surface automatically
     **/
    TMS_SCREEN_ALLOC_SURFACE = 1<<1,
};

/**
 * A screen is a virtual render target where a scene is rendered.
 * The currently active screen will be responsible for rendering
 * to the framebuffer every frame, and handling input events.
 *
 * To create a screen, one must define a tms_screen_spec
 * that informs TMS of callback functions that any screen of this
 * "type" will use. One spec can be used to create several screens,
 * and these screens will share the same set of callback functions.
 *
 * To activate a screen, use tms_set_screen(). A screen can also be
 * activated by using a tms_transition.
 *
 * Typically, none of the screen functions except tms_screen_alloc()
 * and tms_screen_get_data() will be used by the dev. The others are used internally.
 **/
struct tms_screen {
    struct tms_screen_spec *spec;
    void  *data;  /**< this screen's private data,
                    *  allocated and returned by alloc_data in the spec. */

    tvec4 background_color;

    struct tms_surface *surface;
    struct tms_scene *scene;
};

/**
 * Screen specification.
 * Send a spec to tms_screen_alloc() to create a screen. All screens
 * created using the same spec will share the same set of callback functions.
 **/
struct tms_screen_spec {
    /**
     * Called the first time the screen is started. This function should return custom data
     * that will be stored in the tms_screen's data variable.
     **/
    void* (*alloc_data)(struct tms_screen *s);

    int (*pause)(struct tms_screen *s);
    int (*resume)(struct tms_screen *s);

    /**
     * Called once per frame.
     **/
    int (*render)(struct tms_screen *s);
    int (*post_render)(struct tms_screen *s);
    int (*begin_frame)(struct tms_screen *s);
    int (*end_frame)(struct tms_screen *s);

    /**
     * Called once per frame, but not if the screen is currently being transitioned.
     **/
    int (*step)(struct tms_screen *s, double dt);
    int (*input)(struct tms_screen *s, struct tms_event *ev, int action);
    int (*free_data)(struct tms_screen *s);

    uint32_t flags;
};

struct tms_screen *tms_screen_alloc(struct tms_screen_spec *spec);
void *tms_screen_get_data(struct tms_screen *s);
int tms_screen_handle_input(struct tms_screen *s, const struct tms_event *ev, int action);
int tms_screen_render(struct tms_screen *);
int tms_screen_begin_frame(struct tms_screen *);
int tms_screen_end_frame(struct tms_screen *);
int tms_screen_step(struct tms_screen *, double dt);
int tms_screen_set_scene(struct tms_screen *s, struct tms_scene *scene);
int tms_screen_set_surface(struct tms_screen *s, struct tms_surface *surf);
struct tms_surface *tms_screen_get_surface(struct tms_screen *s);
struct tms_scene *tms_screen_get_scene(struct tms_screen *s);

#endif
