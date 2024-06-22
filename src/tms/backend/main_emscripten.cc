
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <tms/core/project.h>
#include <tms/core/event.h>
#include <tms/core/tms.h>

#include <tms/backend/opengl.h>

#include <pwd.h>

#include <emscripten.h>

FILE *_f_out = stdout;

static int _storage_type = 0;

SDL_Window *_window;

int keys[235];
int mouse_down;
static char *_storage_path = 0;

static int T_intercept_input(SDL_Event ev);

extern "C" int tbackend_init_surface();
extern "C" const char *tbackend_get_storage_path(void);

static void mainloop(void)
{
    SDL_Event  ev;
    int i;

    for (i = 0; i < 235; ++i) {
        if (keys[i] == 1) {
            struct tms_event spec;
            spec.type = TMS_EV_KEY_DOWN;
            spec.data.key.keycode = i;

            tms_event_push(spec);
        }
    }

    while (SDL_PollEvent(&ev)) {
        switch (ev.type) {
            case SDL_QUIT:
                _tms.state = TMS_STATE_QUITTING;
                break;

            case SDL_KEYDOWN:
                T_intercept_input(ev);
                keys[ev.key.keysym.scancode] = 1;
                break;

            case SDL_KEYUP:
                T_intercept_input(ev);
                keys[ev.key.keysym.scancode] = 0;
                break;

            case SDL_MOUSEWHEEL:
            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
            case SDL_MOUSEMOTION:
            case SDL_TEXTINPUT:
                T_intercept_input(ev);
                break;

            default:
                break;
        }
    }

    tms_step();
    tms_begin_frame();
    tms_render();
    SDL_GL_SwapWindow(_window);
    tms_end_frame();
}

int main(int argc, char **argv)
{
    int        done = 0;

    tms_infof("Initializing SDL...");
    SDL_Init(SDL_INIT_VIDEO);
    SDL_DisplayMode mode;
    SDL_GetCurrentDisplayMode(0, &mode);

    _tms.window_width = 1280;
    _tms.window_height = 720;

    tproject_set_args(argc, argv);
    tms_init();

    if (_tms.screen == 0)
        tms_fatalf("Context has no initial screen!");

    emscripten_set_main_loop(mainloop, 0, 1);

    tproject_quit();

    SDL_Quit();

    return 0;
}

int
tbackend_init_surface()
{
    SDL_Init(SDL_INIT_VIDEO);

    _window = SDL_CreateWindow("Principia", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
            1280, 720,
            SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);

    if (_window == NULL)
        tms_fatalf("Could not create SDL Window: %s", SDL_GetError());

    _tms._window = _window;
    _tms.xppcm = 108.f/2.54f * 1.5f;
    _tms.yppcm = 107.f/2.54f * 1.5f;

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);

    SDL_GL_CreateContext(_window);

    SDL_GL_SetSwapInterval(0);

    return T_OK;
}

int
mouse_button_to_pointer_id(int button)
{
    switch (button) {
        case SDL_BUTTON_LEFT: return 0;
        case SDL_BUTTON_RIGHT: return 1;
        case SDL_BUTTON_MIDDLE: return 2;
        default: return 4;
    }
}

int
T_intercept_input(SDL_Event ev)
{
    struct tms_event spec;
    spec.type = -1;

    int motion_y = _tms.window_height-ev.motion.y;
    int button_y = _tms.window_height-ev.button.y;

    switch (ev.type) {
        case SDL_KEYDOWN:
            if (ev.key.repeat)
                spec.type = TMS_EV_KEY_REPEAT;
            else
                spec.type = TMS_EV_KEY_PRESS;

            spec.data.key.keycode = ev.key.keysym.scancode;

            spec.data.key.mod = ev.key.keysym.mod;
            switch (spec.data.key.keycode) {
                case TMS_KEY_LEFT_CTRL: spec.data.key.mod |= TMS_MOD_LCTRL; break;
                case TMS_KEY_RIGHT_CTRL: spec.data.key.mod |= TMS_MOD_RCTRL; break;
                case TMS_KEY_LEFT_SHIFT: spec.data.key.mod |= TMS_MOD_LSHIFT; break;
                case TMS_KEY_RIGHT_SHIFT: spec.data.key.mod |= TMS_MOD_RSHIFT; break;
            }
            break;

        case SDL_KEYUP:
            spec.type = TMS_EV_KEY_UP;
            spec.data.key.keycode = ev.key.keysym.scancode;

            spec.data.key.mod = ev.key.keysym.mod;
            break;

        case SDL_MOUSEBUTTONDOWN:
            if (ev.button.which == SDL_TOUCH_MOUSEID)
                return T_OK;

            spec.type = TMS_EV_POINTER_DOWN;
            spec.data.button.pointer_id = mouse_button_to_pointer_id(ev.button.button);
            spec.data.button.x = ev.button.x;
            spec.data.button.y = button_y;
            spec.data.button.button = ev.button.button;

            if (mouse_down == 0)
                mouse_down = ev.button.button;

            break;

        case SDL_MOUSEBUTTONUP:
            if (ev.button.which == SDL_TOUCH_MOUSEID)
                return T_OK;

            spec.type = TMS_EV_POINTER_UP;
            spec.data.button.pointer_id = mouse_button_to_pointer_id(ev.button.button);
            spec.data.button.x = ev.button.x;
            spec.data.button.y = button_y;
            spec.data.button.button = ev.button.button;

            if (mouse_down == ev.button.button)
                mouse_down = 0;

            break;

        case SDL_MOUSEMOTION:
            if (ev.button.which == SDL_TOUCH_MOUSEID)
                return T_OK;

            spec.data.button.pointer_id = mouse_button_to_pointer_id(ev.button.button);

            if (mouse_down) {
                spec.type = TMS_EV_POINTER_DRAG;
                spec.data.button.x = ev.motion.x;
                spec.data.button.y = button_y;
                spec.data.button.button = mouse_down;
            } else {
                spec.type = TMS_EV_POINTER_MOVE;
                spec.data.button.x = ev.motion.x;
                spec.data.button.y = motion_y;
            }

            break;

        case SDL_MOUSEWHEEL:
            spec.type = TMS_EV_POINTER_SCROLL;
            spec.data.scroll.x = ev.wheel.x;
            spec.data.scroll.y = ev.wheel.y;
            SDL_GetMouseState(&spec.data.scroll.mouse_x, &spec.data.scroll.mouse_y);
            break;

        case SDL_TEXTINPUT:
            spec.type = TMS_EV_TEXT_INPUT;
            std::copy(ev.text.text, ev.text.text + 32, spec.data.text.text);
            break;
    }

    tms_event_push(spec);

    return T_OK;
}

const char *tbackend_get_storage_path(void)
{
    if (!_storage_path) {
        char *path = (char*)malloc(512);

        char* exedir = SDL_GetPrefPath("Bithack", "Principia");
        strcpy(path, exedir);
        strcat(path, "userdata");

        _storage_path = path;

        tms_infof("Storage path: %s", path);
    }
    return _storage_path;
}
