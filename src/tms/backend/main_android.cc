// Android backend

#include <unistd.h>
#include <sys/time.h>

#include <tms/core/project.h>
#include <tms/core/event.h>
#include <tms/core/tms.h>
#include <tms/backend/opengl.h>

#include "SDL.h"
#include <jni.h>
//#include "SDL_androidvideo.h"

SDL_Window *_window;

int keys[235];
int mouse_down[64];

extern "C" int tbackend_init_surface();
extern "C" const char *tbackend_get_storage_path(void);

static int T_intercept_input(SDL_Event ev);

int
SDL_main(int argc, char **argv)
{
    SDL_Event  ev;
    int        done = 0;
    int do_step = 1;

    tms_init();

    if (_tms.screen == 0)
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
                case SDL_WINDOWEVENT: {
                    switch (ev.window.event) {
                        case SDL_WINDOWEVENT_MINIMIZED:
                            tproject_soft_pause();
                            do_step = 0;
                            break;

                        case SDL_WINDOWEVENT_RESTORED:
                            tproject_soft_resume();
                            do_step = 1;
                            break;
                    }
                } break;

                case SDL_QUIT:
                    tproject_quit();
                    _tms.state = TMS_STATE_QUITTING;
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
                case SDL_TEXTINPUT:
                    T_intercept_input(ev);
                    break;

                default:
                    //tms_debugf("Unhandled event: %d", ev.type);
                    break;
            }
        }

        if (_tms.is_paused == 0) {
            tms_step();
            tms_begin_frame();
            tms_render();
            SDL_GL_SwapWindow(_window);
            tms_end_frame();
        } else {
            SDL_Delay(100);
        }
    } while (_tms.state != TMS_STATE_QUITTING);

    SDL_DestroyWindow(_window);

    SDL_Quit();

    return 0;
}

int
tbackend_init_surface()
{
    /* Set up SDL, create a window */
    tms_infof("Creating window...");

    SDL_Init(SDL_INIT_VIDEO);
    SDL_DisplayMode mode;
    SDL_GetCurrentDisplayMode(0, &mode);

    _window = SDL_CreateWindow("Principia", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
            0, 0,
            SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN);

    if (_window == NULL)
        tms_fatalf("Could not create SDL Window: %s", SDL_GetError());

    SDL_SetWindowFullscreen(_window, SDL_TRUE);
    SDL_GL_GetDrawableSize(_window, &_tms.window_width, &_tms.window_height);

    _tms._window = _window;
    float density_x, density_y;
    SDL_GetDisplayDPI(0, NULL, &density_x, &density_y);
    _tms.xppcm = density_x / 2.54f;
    _tms.yppcm = density_y / 2.54f;

    tms_infof("Device dimensions: %d %d", _tms.window_width, _tms.window_height);
    tms_infof("Device PPCM: %f %f", _tms.xppcm, _tms.yppcm);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);

    SDL_GL_CreateContext(_window);

    //SDL_GL_SetSwapInterval(0);

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
            spec.data.button.x = (int)(ev.tfinger.x*(float)_tms.window_width);
            spec.data.button.y = _tms.window_height-(int)(ev.tfinger.y*(float)_tms.window_height);
            break;

        case SDL_FINGERUP:
            spec.type = TMS_EV_POINTER_UP;
            spec.data.button.pointer_id = ev.tfinger.fingerId;
            spec.data.button.x = (int)(ev.tfinger.x*(float)_tms.window_width);
            spec.data.button.y = _tms.window_height-(int)(ev.tfinger.y*(float)_tms.window_height);
            break;

        case SDL_FINGERMOTION:
            spec.type = TMS_EV_POINTER_DRAG;
            spec.data.button.pointer_id = ev.tfinger.fingerId;
            spec.data.button.x = (int)(ev.tfinger.x*(float)_tms.window_width);
            spec.data.button.y = _tms.window_height-(int)(ev.tfinger.y*(float)_tms.window_height);
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
    return SDL_AndroidGetExternalStoragePath();
}
