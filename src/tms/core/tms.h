#ifndef _TMS__H_
#define _TMS__H_

#include <stdint.h>
#include <stdio.h>

/**
 * @mainpage
 *
 * \section Basics
 *
 * \section Reference
 *
 * \subsection Screens
 * \li tms_screen - Container of tms_scene and surfaces of tms_widget 's
 * \li tms_widget - Widgets for the 2D widget surface
 * \li tms_fb - For off-screen rendering and wrapping tms_screen
 *
 * \subsection Rendering
 * \li tms_scene - The scene
 * \li tms_entity - Entity, anything in the scene that will be rendered
 * \li tms_material - An entity's material
 * \li tms_mesh - An entity's mesh
 * \li tms_texture - An entity's texture
 *
 * \subsection Math
 * \li tmatN - 3x3 and 4x4 matrices
 * \li tvec2 - 2D vector
 * \li tvec3 - 3D vector
 * \li tvec4 - 4D vector / Plane / Homogenous 3D vector
 **/

/** @relates tms @{ **/

#include "SDL.h"
#include <tms/core/err.h>
#include <tms/math/vector.h>
#include <tms/core/settings.h>
#include <tms/math/matrix.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Internal state
 **/
enum TMS_STATE {
    TMS_STATE_DEFAULT,
    TMS_STATE_QUITTING,
};

struct tms_screen;
struct tms_fb;
struct tms_model;
struct tms_texture;
struct tms_mesh;

#define TMS_FPS_MEAN_LIMIT .25

#ifndef __cplusplus
#define tms _tms
#endif

#ifndef TMS_BACKEND_IOS
#define opengl_width window_width
#define opengl_height window_height
#endif

/**
 * Global singleton object that the project work against.
 * tms is a global singleton that can be accessed from anywhere,
 * it contains data for the current "context", including window
 * width and height.
 **/
extern struct tms_singleton {
    int window_width;
    int window_height;
#ifdef TMS_BACKEND_IOS
    int opengl_width;
    int opengl_height;
#endif

    float xppcm;
    float yppcm;

    int gamma_correct;
    int emulating_portrait;
    int in_frame;

    float window_projection[16];
    struct tms_screen      *screen; /**< current screen */
    struct tms_screen      *active_screen;
    struct tms_screen      *next; /**< next screen (used if transitioning) */
    struct tms_fb *framebuffer; /**< pointer to the currently bound framebuffer */
    int                     state; /**< current state, see TMS_STATE */
    const char *gl_extensions;

    double gamma;

    int is_paused;

    uint64_t time_accum;
    uint64_t delta_cap;
    double dt; /**< delta time since last frame, in milliseconds */
    double fps; /**< FPS calculated from the current dt, also see fps_mean */
    double fps_mean; /**< average frames per second for an internal amount of frames */
    double dt_accum;
    int dt_count;
    uint64_t last_time;

    SDL_Window *_window;
} _tms;

int tms_init(void);
int tms_set_screen(struct tms_screen *screen);
void tms_step(void);
int tms_render(void);
int tms_begin_frame(void);
int tms_end_frame(void);

static inline void tms_convert_to_portrait(int *x, int *y)
{
    int tmp_y = *y;

    if ((*x) < _tms.window_width/2) {
        (*y) = (*x);
    } else {
        (*y) = _tms.window_height - (_tms.window_width-(*x));
    }

    (*x) = _tms.window_width - tmp_y;
}

#ifdef __cplusplus
}
#endif

#endif
