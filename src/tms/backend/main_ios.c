// iOS backend
// (we have absolutely no idea if this still works)

#include <unistd.h>
#include <sys/time.h>

#include <tms/core/project.h>
#include <tms/core/event.h>
#include <tms/core/tms.h>

#include "opengl.h"
#include <SDL.h>

SDL_Window *_window;

int keys[235];
int mouse_down[64];

static int T_intercept_input(SDL_Event ev);

int
SDL_main(int argc, char **argv)
{
    SDL_Event  ev;
    int        done = 0;
    int do_step = 1;

    tms_init();

    if (tms.screen == 0)
        tms_fatalf("context has no initial screen, bailing out");
    do {
        int i;

        if (do_step) {
            for (i = 0; i < 235; ++i) {
                if (keys[i] == 1) {
                    struct tms_event spec;
                    spec.type = TMS_EV_KEY_DOWN;
                    spec.data.key.keycode = i;

                    tms_event_push(spec);
                }
            }
        }

        while (SDL_PollEvent(&ev)) {
            switch (ev.type) {

                case SDL_WINDOWEVENT:
                    {
                        switch (ev.window.event) {
                            //case SDL_WINDOWEVENT_FOCUS_LOST:
                            case SDL_WINDOWEVENT_MINIMIZED:
                                tproject_soft_pause();
                                tms_infof("FOCUS LOST ---------------");
                                do_step = 0;
                                break;

                            //case SDL_WINDOWEVENT_FOCUS_GAINED:
                            case SDL_WINDOWEVENT_RESTORED:
                                tms_infof("FOCUS GAINED");
                                do_step = 1;
                                tproject_soft_resume();
                                break;
                        }
                    }
                    break;

                case SDL_QUIT:
                    tms.state = TMS_STATE_QUITTING;
                    //done = 1;
                    break;

                case SDL_KEYDOWN:
                    T_intercept_input(ev);
                    keys[ev.key.keysym.scancode] = 1;
                    break;

                case SDL_KEYUP:
                    T_intercept_input(ev);
                    keys[ev.key.keysym.scancode] = 0;
                    break;

                case SDL_FINGERMOTION:
                case SDL_FINGERDOWN:
                case SDL_FINGERUP:
                    T_intercept_input(ev);
                    break;

                case SDL_TEXTINPUT:
                    T_intercept_input(ev);
                    break;
            }
        }

        if (do_step) {
            tms_step();
            tms_begin_frame();
            tms_render();
            SDL_GL_SwapWindow(_window);
            tms_end_frame();
        } else {
            SDL_Delay(100);
        }

//        usleep(15000);
    } while (tms.state != TMS_STATE_QUITTING);

    return 0;
}

int
tbackend_init_surface()
{
    /* Set up SDL, create a window */
    tms_infof("Creating window...");
    SDL_Init(SDL_INIT_VIDEO);
	SDL_SetHint("SDL_HINT_ORIENTATIONS", "LandscapeRight");
	SDL_SetHint("SDL_IOS_ORIENTATIONS", "LandscapeRight");
    SDL_DisplayMode mode;

    int num_modes = SDL_GetNumDisplayModes(0);

    int min_displ = 2000000000;
    int max_displ = 0;

    int min_displ_w, min_displ_h, max_displ_w, max_displ_h;

    for (int x=0; x<num_modes; x++) {
        SDL_GetDisplayMode(0, x, &mode);
        tms_infof("display mode %d %d", mode.w, mode.h);

        int displ = mode.w*mode.h;
        if (displ < min_displ) {
            min_displ = displ;
            min_displ_w = mode.w;
            min_displ_h = mode.h;
        }
        if (displ > max_displ) {
            max_displ = displ;
            max_displ_w = mode.w;
            max_displ_h = mode.h;
        }
    }

    if (max_displ_w < max_displ_h) {
        int tmp = max_displ_w;
        max_displ_w = max_displ_h;
        max_displ_h = tmp;
    }

    if (min_displ_w < min_displ_h) {
        int tmp = min_displ_w;
        min_displ_w = min_displ_h;
        min_displ_h = tmp;
    }
    SDL_GetDesktopDisplayMode(0, &mode);

    tms_infof("max display mode: %d %d", max_displ_w, max_displ_h);
    tms_infof("min display mode: %d %d", min_displ_w, min_displ_h);


    _window = SDL_CreateWindow("Principia", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
            //mode.w, mode.h,
			min_displ_w, min_displ_h,
            SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN /*| SDL_WINDOW_FULLSCREEN*/ | SDL_WINDOW_BORDERLESS);


    //SDL_GetWindowSize(_window, &_tms.window_width, &_tms.window_height);
#if 0
    tms.opengl_width = min_displ_w;
    tms.opengl_height = min_displ_h;
#else
    tms.opengl_width = max_displ_w;
    tms.opengl_height = max_displ_h;

#endif
    tms.window_width = max_displ_w;
    tms.window_height = max_displ_h;

    tms._window = _window;
    tms.xppcm = IOS_density_x / 2.54;
    tms.yppcm = IOS_density_y / 2.54;

    tms_infof("OpenGL dimensions: %d %d", tms.opengl_width, tms.opengl_height);
    tms_infof("Window dimensions: %d %d", _tms.window_width, _tms.window_height);
    tms_infof("Device PPCM: %f %f", tms.xppcm, tms.yppcm);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);

    SDL_GL_CreateContext(_window);

    SDL_GL_SetSwapInterval(0);

//#include "glhacks/definc.h"

    return T_OK;
}

int
T_intercept_input(SDL_Event ev)
{
    struct tms_event spec;
    spec.type = -1;

    switch (ev.type) {
        case SDL_KEYDOWN:
            if (keys[ev.key.keysym.scancode] == 1) {
                return T_OK;
            }

            spec.type = TMS_EV_KEY_PRESS;
            spec.data.key.keycode = ev.key.keysym.scancode;
            spec.data.key.mod = ev.key.keysym.mod;
            break;

        case SDL_KEYUP:
            spec.type = TMS_EV_KEY_UP;
            spec.data.key.keycode = ev.key.keysym.scancode;
            spec.data.key.mod = ev.key.keysym.mod;
            break;

        case SDL_FINGERDOWN:
            spec.type = TMS_EV_POINTER_DOWN;
            spec.data.button.pointer_id = ev.tfinger.fingerId;
            tms_infof("event: %d %d", ev.tfinger.x, ev.tfinger.y);
            spec.data.button.x = ((float)ev.tfinger.x / 65536.f)*(float)_tms.window_width;
            spec.data.button.y = tms.window_height - ((float)ev.tfinger.y / 65536.f) * (float)_tms.window_height;
            //tms_infof("event: %f %f", spec.data.button.x, spec.data.button.y);
            break;

        case SDL_FINGERUP:
            spec.type = TMS_EV_POINTER_UP;
            spec.data.button.pointer_id = ev.tfinger.fingerId;
            spec.data.button.x = ((float)ev.tfinger.x / 65536.f)*(float)_tms.window_width;
            spec.data.button.y = tms.window_height - ((float)ev.tfinger.y / 65536.f) * (float)_tms.window_height;
            break;

        case SDL_FINGERMOTION:
            spec.type = TMS_EV_POINTER_DRAG;
            spec.data.button.pointer_id = ev.tfinger.fingerId;
            spec.data.motion.x = ((float)ev.tfinger.x / 65536.f)*(float)_tms.window_width;
            spec.data.motion.y = tms.window_height - ((float)ev.tfinger.y / 65536.f) * (float)_tms.window_height;
            break;

        case SDL_TEXTINPUT:
            spec.type = TMS_EV_TEXT_INPUT;
            memcpy(spec.data.text.text, ev.text.text, 32);
            break;
    }

    tms_event_push(spec);

    return T_OK;
}

const char *tbackend_get_storage_path(void)
{
    return "../Documents";
}
