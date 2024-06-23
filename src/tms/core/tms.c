#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <stdarg.h>

#include "tms.h"
#include "screen.h"
#include "event.h"
#include "material.h"
#include "backend.h"
#include "shader.h"
#include "project.h"
#include "hash.h"

struct tms_singleton _tms = {
    .in_frame = 0,
    .window_width = 0,
    .window_height = 0,
    .gamma = 2.2,
    .gl_extensions = "",
    .delta_cap = 0,
    .emulating_portrait = 0,
};

int
tms_init(void)
{
    tms.framebuffer = 0;
    tms.is_paused = 0;
    tms.state = TMS_STATE_DEFAULT;

    tproject_preinit();

    tbackend_init_surface();

    tms.gl_extensions = (const char*)glGetString(GL_EXTENSIONS);

    tmat4_set_ortho(tms.window_projection, 0, tms.window_width, 0, tms.window_height, 1, -1);

#ifndef TMS_USE_GLES
    tms_shader_global_define_vs("lowp", "");
    tms_shader_global_define_fs("lowp", "");
    tms_shader_global_define_vs("mediump", "");
    tms_shader_global_define_fs("mediump", "");
    tms_shader_global_define_vs("highp", "");
    tms_shader_global_define_fs("highp", "");
#endif

    tproject_init();
    tproject_init_pipelines();
    tproject_initialize();

    return T_OK;
}

int
tms_set_screen(struct tms_screen *screen)
{
    struct tms_screen *cur_screen = tms.screen;

    if (!tms.in_frame) {
        if (tms.screen != 0) {
            if (tms.screen->spec->pause != 0) {
                tms.screen->spec->pause(tms.screen);
            }
        }

        if (screen != 0) {
            if (screen->spec->resume != 0) {
                screen->spec->resume(screen);
            }
        }
    }

    if (tms.screen != cur_screen) {
        tms_infof("current screen changed, skipping set");
    } else
        tms.screen = screen;

    return T_OK;
}

#ifdef TMS_BACKEND_IOS
uint64_t tms_IOS_get_time();
#endif

static inline void
init_frame_time(void)
{
#ifndef TMS_BACKEND_IOS
    struct timeval t;
#endif
    uint64_t curr_time, delta;

#ifdef TMS_BACKEND_IOS
    curr_time = tms_IOS_get_time();
#else
    gettimeofday(&t, 0);
    curr_time = t.tv_usec + t.tv_sec * 1000000ull;
#endif

    if (tms.last_time == 0)
        tms.last_time = curr_time;

    delta = curr_time - tms.last_time;

    if (tms.delta_cap != 0 && delta < tms.delta_cap) {
        /* XXX */
        SDL_Delay((tms.delta_cap - delta)/1000);
        delta = tms.delta_cap;

#ifdef TMS_BACKEND_IOS
        curr_time = tms_IOS_get_time();
#else
        gettimeofday(&t, 0);
        curr_time = t.tv_usec + t.tv_sec * 1000000ull;
#endif
    }
    tms.last_time = curr_time;

    /* TODO: handle too large delta number */

    tms.time_accum += delta;

    if (tms.last_time != 0ull) {
        tms.dt = (double)(delta) / 1000000.0;
    } else {
        tms.dt = 0.f;
    }

    if (tms.dt_accum >= TMS_FPS_MEAN_LIMIT) {
        tms.fps_mean = 1.0 / (tms.dt_accum / tms.dt_count);
        tms.dt_count = 0;
        tms.dt_accum = 0.0;
    } else {
        tms.dt_count ++;
        tms.dt_accum += tms.dt;
    }

    /* direct fps estimation */
    tms.fps = 1.0 / tms.dt;
}

int
tms_begin_frame(void)
{
    tms.active_screen = tms.screen;
    tms.in_frame = 1;

#ifdef TMS_BACKEND_IOS
    glBindFramebuffer(GL_FRAMEBUFFER, viewFramebuffer);
#endif

    tms_screen_begin_frame(tms.active_screen);
    return T_OK;
}

int
tms_end_frame(void)
{
    tms_screen_end_frame(tms.active_screen);

    if (tms.active_screen != tms.screen) {
        /* the screen was changed */
        if (tms.active_screen != 0) {
            if (tms.active_screen->spec->pause != 0) {
                tms.active_screen->spec->pause(tms.active_screen);
            }
        }

        if (tms.screen != 0) {
            if (tms.screen->spec->resume != 0) {
                tms.screen->spec->resume(tms.screen);
            }
        }
    }

    tms.in_frame = 0;
    return T_OK;
}

void
tms_step(void)
{
    tproject_step();
}

int
tms_render(void)
{
    init_frame_time();

    struct tms_screen *s = tms.active_screen;

    tms_event_process_all(s);

    tms_screen_step(s, tms.dt);
    tms_screen_render(s);

    return T_OK;
}

