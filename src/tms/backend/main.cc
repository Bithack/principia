// Unified backend used for most platforms (Linux, Windows...)

#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <tms/core/project.h>
#include <tms/core/event.h>
#include <tms/core/tms.h>

#include <tms/backend/opengl.h>

#include "settings.hh"
#include "main.hh"
#include "version.hh"

#ifdef TMS_BACKEND_WINDOWS
    #include <windows.h>
    #include <windowsx.h>
    #include <clocale>
    #include "shlwapi.h"
    #include "SDL_syswm.h"
#else
    #include <pwd.h>
#endif

#include "pipe.hh"

FILE *_f_out = stdout;

static int _storage_type = 0;

SDL_Window *_window;

int keys[235];
int mouse_down;
static char *_storage_path = 0;

static int T_intercept_input(SDL_Event ev);

extern "C" int tbackend_init_surface();
extern "C" const char *tbackend_get_storage_path(void);

static void _catch_signal(int signal)
{
    tms_errorf("Segmentation fault!");

#ifdef TMS_BACKEND_WINDOWS
    if (_f_out != stdout) {
        fflush(_f_out);
        fclose(_f_out);
    }
#endif

	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Principia",
R"(An unrecoverable error has occurred and Principia will now close.

Please report this crash to the issue tracker with the relevant steps
to reproduce it, if possible.
)", 0);

    exit(1);
}

void print_log_header() {
    tms_printf( \
        "            _            _       _       \n"
        " _ __  _ __(_)_ __   ___(_)_ __ (_) __ _ \n"
        "| '_ \\| '__| | '_ \\ / __| | '_ \\| |/ _` |\n"
        "| |_) | |  | | | | | (__| | |_) | | (_| |\n"
        "| .__/|_|  |_|_| |_|\\___|_| .__/|_|\\__,_|\n"
        "|_|                       |_|            \n"
        "Version %s, commit %s\n", principia_version_string(), principia_version_hash());
}

int main(int argc, char **argv)
{
    SDL_Event  ev;
    int        done = 0;

    signal(SIGSEGV, _catch_signal);

#ifdef TMS_BACKEND_WINDOWS
    setlocale(LC_ALL, "C");
#endif

    setup_pipe(argc, argv);

    char* exedir = SDL_GetBasePath();
    tms_infof("chdirring to %s", exedir);
    chdir(exedir);

    // Switch to portable if ./portable.txt exists next to binary
    if (access("portable.txt", F_OK) == 0) {
        tms_infof("We're becoming portable!");
        _storage_type = 1;
    }

#ifdef TMS_BACKEND_WINDOWS
    mkdir(tbackend_get_storage_path());
#else
    mkdir(tbackend_get_storage_path(), S_IRWXU | S_IRWXG | S_IRWXO);
#endif

#ifndef DEBUG
    char logfile[1024];
    snprintf(logfile, 1023, "%s/run.log", tbackend_get_storage_path());

    tms_infof("Redirecting log output to %s", logfile);
    FILE *log = fopen(logfile, "w+");
    if (log) {
        _f_out = log;
    } else {
        tms_errorf("Could not open log file for writing! Nevermind.");
    }
#endif

    print_log_header();

    // Check if we're in the right place
    struct stat st{};
    if (stat("data", &st) != 0) {
        // We're in the build dir, go up
        tms_infof("chdirring to ../");
        chdir("../");

        // How about now?
        if (stat("data", &st) != 0) {
            // If that doesn't work we're assuming a system install.
            tms_infof("chdirring to ./share/principia/");
            chdir("./share/principia/");

            if (stat("data", &st) != 0) {
                // We're doomed, better just fail.
                tms_fatalf("Could not find data directories.");
            }
        }
    }

    SDL_version compiled;
    SDL_VERSION(&compiled);
    tms_infof("Compiled against SDL v%u.%u.%u",
        compiled.major, compiled.minor, compiled.patch);

    SDL_version linked;
    SDL_GetVersion(&linked);
    tms_infof("Linked against SDL v%u.%u.%u",
        linked.major, linked.minor, linked.patch);

    tms_infof("Initializing SDL...");
    SDL_Init(SDL_INIT_VIDEO);
    SDL_DisplayMode mode;
    SDL_GetCurrentDisplayMode(0, &mode);

    _tms.window_width = 1280;

    if (mode.w <= 1280)
        _tms.window_width = (int)((double)mode.w * .9);
    else if (mode.w >= 2100 && mode.h > 1100)
        _tms.window_width = 1920;

    _tms.window_height = (int)((double)_tms.window_width * .5625);

    tms_infof("set initial res to %dx%d", _tms.window_width, _tms.window_height);

    tproject_set_args(argc, argv);
    tms_init();

#ifdef TMS_BACKEND_WINDOWS
    SDL_EventState(SDL_SYSWMEVENT, settings["emulate_touch"]->is_true() ? SDL_ENABLE : SDL_DISABLE);
#endif

    if (_tms.screen == 0)
        tms_fatalf("Context has no initial screen!");

    do {
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

                case SDL_WINDOWEVENT:
                    switch (ev.window.event) {
                        case SDL_WINDOWEVENT_RESIZED: {
                            tms_infof("Window %d resized to %dx%d",
                                    ev.window.windowID, ev.window.data1,
                                    ev.window.data2);
                            int w = ev.window.data1;
                            int h = ev.window.data2;

                            _tms.window_width  = _tms.opengl_width  = w;
                            _tms.window_height = _tms.opengl_height = h;

                            tproject_window_size_changed();
                        } break;
                        case SDL_WINDOWEVENT_MAXIMIZED:
                            settings["window_maximized"]->v.b = true;
                            break;
                        case SDL_WINDOWEVENT_RESTORED:
                            settings["window_maximized"]->v.b = false;
                            break;
                        default:
                            break;
                    }
                    break;

                case SDL_FINGERDOWN:
                case SDL_FINGERUP:
                case SDL_FINGERMOTION:
                case SDL_MOUSEWHEEL:
                    T_intercept_input(ev);
                    break;
                case SDL_MOUSEBUTTONDOWN:
                case SDL_MOUSEBUTTONUP:
                case SDL_MOUSEMOTION: {
                    if (settings["emulate_touch"]->is_false()) {
                        //tms_infof("from sdl mouse thing");
                        T_intercept_input(ev);
                    }
                } break;

                // XXX: is this necessary
#ifdef TMS_BACKEND_WINDOWS
                case SDL_SYSWMEVENT: {
                    SDL_SysWMmsg *msg = (SDL_SysWMmsg*)ev.syswm.msg;

                    switch (msg->msg.win.msg) {
                        case WM_MOUSEMOVE: {
                            //tms_infof("MOUSE MOVE");

                            SDL_Event user_event;
                            user_event.type = SDL_MOUSEMOTION;
                            user_event.button.x = GET_X_LPARAM(msg->msg.win.lParam);
                            user_event.button.y = GET_Y_LPARAM(msg->msg.win.lParam);
                            user_event.button.button = SDL_BUTTON_LEFT;

                            T_intercept_input(user_event);
                        } break;

                        case WM_LBUTTONDOWN: {
                            //tms_infof("LBUTTON DOWN");
                            SDL_Event user_event;
                            user_event.type = SDL_MOUSEBUTTONDOWN;
                            user_event.button.x = GET_X_LPARAM(msg->msg.win.lParam);
                            user_event.button.y = GET_Y_LPARAM(msg->msg.win.lParam);
                            user_event.button.button = SDL_BUTTON_LEFT;

                            T_intercept_input(user_event);
                        } break;

                        case WM_LBUTTONUP: {
                            //tms_infof("LBUTTON UP");
                            SDL_Event user_event;
                            user_event.type = SDL_MOUSEBUTTONUP;
                            user_event.button.x = GET_X_LPARAM(msg->msg.win.lParam);
                            user_event.button.y = GET_Y_LPARAM(msg->msg.win.lParam);
                            user_event.button.button = SDL_BUTTON_LEFT;

                            T_intercept_input(user_event);
                        } break;

                        case WM_RBUTTONDOWN: {
                            //tms_infof("RBUTTON DOWN");
                            SDL_Event user_event;
                            user_event.type = SDL_MOUSEBUTTONDOWN;
                            user_event.button.x = GET_X_LPARAM(msg->msg.win.lParam);
                            user_event.button.y = GET_Y_LPARAM(msg->msg.win.lParam);
                            user_event.button.button = SDL_BUTTON_RIGHT;

                            T_intercept_input(user_event);
                        } break;

                        case WM_RBUTTONUP: {
                            //tms_infof("RBUTTON UP");
                            SDL_Event user_event;
                            user_event.type = SDL_MOUSEBUTTONUP;
                            user_event.button.x = GET_X_LPARAM(msg->msg.win.lParam);
                            user_event.button.y = GET_Y_LPARAM(msg->msg.win.lParam);
                            user_event.button.button = SDL_BUTTON_RIGHT;

                            T_intercept_input(user_event);
                        } break;
                    }
                } break;
#endif

                case SDL_TEXTINPUT:
                    T_intercept_input(ev);
                    break;
            }
        }

        tms_step();
        tms_begin_frame();
        tms_render();
        SDL_GL_SwapWindow(_window);
        tms_end_frame();

        if (settings["emulate_touch"]->is_true()) {
            int x, y;
            SDL_GetMouseState(&x, &y);

            SDL_Event user_event;
            user_event.type = SDL_MOUSEMOTION;
            user_event.button.x = x;
            user_event.button.y = y;
            user_event.button.button = SDL_BUTTON_LEFT;
        }
    } while (_tms.state != TMS_STATE_QUITTING);

    tproject_quit();

    SDL_Quit();

    return 0;
}

int
tbackend_init_surface()
{
    _tms.window_width = settings["window_width"]->v.i;
    _tms.window_height = settings["window_height"]->v.i;

    _tms.xppcm = 108.f/2.54f * 1.5f;
    _tms.yppcm = 107.f/2.54f * 1.5f;

    uint32_t flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN;

    if (settings["window_maximized"]->v.b)
        flags |= SDL_WINDOW_MAXIMIZED;

    if (settings["window_fullscreen"]->v.b)
        flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;

    if (settings["window_resizable"]->v.b)
        flags |= SDL_WINDOW_RESIZABLE;

    tms_infof("Creating window...");
    _window = SDL_CreateWindow("Principia", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		_tms.window_width, _tms.window_height, flags);

    if (_window == NULL) {
        tms_infof("ERROR: %s", SDL_GetError());
        exit(1);
    }

    _tms._window = _window;

#ifdef TMS_USE_GLES

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);

    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);

    SDL_GL_CreateContext(_window);

    SDL_GL_SetSwapInterval(0);

#else

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    SDL_GLContext gl_context = SDL_GL_CreateContext(_window);

    if (gl_context == NULL)
        tms_fatalf("Error creating GL Context: %s", SDL_GetError());

    tms_infof("Initializing GLEW...");
    GLenum err = glewInit();
    if (err != GLEW_OK && err != GLEW_ERROR_NO_GLX_DISPLAY) {
        tms_infof("ERROR: %s", glewGetErrorString(err));
        exit(1);
    }
    tms_infof("GLEW init OK (v%s)", glewGetString(GLEW_VERSION));

    tms_infof("GL Info: %s/%s/%s", glGetString(GL_VENDOR), glGetString(GL_RENDERER), glGetString(GL_VERSION));
    tms_infof("GLSL Version: %s", glGetString(GL_SHADING_LANGUAGE_VERSION));

    tms_printf("GL versions supported: ");
    if (GLEW_VERSION_4_6) tms_printf("4.6,");
    if (GLEW_VERSION_4_5) tms_printf("4.5,");
    if (GLEW_VERSION_4_4) tms_printf("4.4,");
    if (GLEW_VERSION_4_3) tms_printf("4.3,");
    if (GLEW_VERSION_4_2) tms_printf("4.2,");
    if (GLEW_VERSION_4_1) tms_printf("4.1,");
    if (GLEW_VERSION_3_3) tms_printf("3.3,");
    if (GLEW_VERSION_3_1) tms_printf("3.1,");
    if (GLEW_VERSION_3_0) tms_printf("3.0,");
    if (GLEW_VERSION_2_1) tms_printf("2.1,");
    if (GLEW_VERSION_2_0) tms_printf("2.0,");
    if (GLEW_VERSION_1_5) tms_printf("1.5,");
    if (GLEW_VERSION_1_4) tms_printf("1.4,");
    if (GLEW_VERSION_1_3) tms_printf("1.3,");
    if (GLEW_VERSION_1_2) tms_printf("1.2,");
    if (GLEW_VERSION_1_1) tms_printf("1.1");
	tms_printf("\n");

#ifdef TMS_BACKEND_WINDOWS

    if (!GLEW_VERSION_1_2) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Principia",
R"(Your graphics driver does not support OpenGL >1.1 and as such Principia will not start.
Most likely this is because you do not have any graphics drivers installed and are using
Windows' software rendering driver. Please install the necessary driver for your
graphics card.

If you are on a VM for testing purposes, then you can use Mesa's software renderer to
get Principia running. (place the Mesa opengl32.dll library next to principia.exe))", 0);
        exit(1);
    }

#endif

#endif

    return T_OK;
}

int
mouse_button_to_pointer_id(int button)
{
    switch (button) {
        case SDL_BUTTON_LEFT: return 0;
        case SDL_BUTTON_RIGHT: return 1;
        case SDL_BUTTON_MIDDLE: return 2;
        //case SDL_BUTTON_WHEELUP: return 3;
        default:/*case SDL_BUTTON_WHEELDOWN:*/ return 4;
    }
}

#define MAX_P 10

static int finger_ids[MAX_P];

static int finger_to_pointer(int finger, bool create)
{
    for (int x=0; x<MAX_P; x++) {
        if ((finger_ids[x] == 0 && create) || finger_ids[x] == finger) {
            tms_infof("found %u at %d", finger, x);
            finger_ids[x] = finger;
            return x;
        }
    }

    /* no slot found */
    if (create) {
        /* helo */
    }

    finger_ids[MAX_P-1] = finger;
    return MAX_P-1;
}

int
T_intercept_input(SDL_Event ev)
{
    struct tms_event spec;
    spec.type = -1;

    int motion_y = _tms.window_height-ev.motion.y;
    int button_y = _tms.window_height-ev.button.y;

    int f;

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

        case SDL_FINGERDOWN:
            spec.type = TMS_EV_POINTER_DOWN;

            f = finger_to_pointer(ev.tfinger.fingerId, true);
            spec.data.button.pointer_id = f;

            spec.data.button.x = (int)(ev.tfinger.x*(float)_tms.window_width);
            spec.data.button.y = _tms.window_height-(int)(ev.tfinger.y*(float)_tms.window_height);
            spec.data.button.button = 0;
            break;

        case SDL_FINGERUP:
            spec.type = TMS_EV_POINTER_UP;

            f = finger_to_pointer(ev.tfinger.fingerId, false);
            spec.data.button.pointer_id = f;
            spec.data.button.x = (int)(ev.tfinger.x*(float)_tms.window_width);
            spec.data.button.y = _tms.window_height-(int)(ev.tfinger.y*(float)_tms.window_height);
            spec.data.button.button = 0;

            finger_ids[f] = 0;
            break;

        case SDL_FINGERMOTION:
            spec.type = TMS_EV_POINTER_DRAG;

            f = finger_to_pointer(ev.tfinger.fingerId, false);
            spec.data.button.pointer_id = f;
            spec.data.button.x = (int)(ev.tfinger.x*(float)_tms.window_width);
            spec.data.button.y = _tms.window_height-(int)(ev.tfinger.y*(float)_tms.window_height);
            spec.data.button.button = 0;

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

        if (_storage_type == 0) { // System (Installed)
#ifdef TMS_BACKEND_WINDOWS
            strcpy(path, getenv("USERPROFILE"));
            strcat(path, "\\Principia");
#else
            struct passwd *pw = getpwuid(getuid());
            strcpy(path, pw->pw_dir);
            strcat(path, "/.principia");
#endif
        } else if (_storage_type == 1) { // Portable
            char* exedir = SDL_GetBasePath();
            strcpy(path, exedir);
            strcat(path, "userdata");
        }

        _storage_path = path;

        tms_infof("Storage path: %s", path);
    }
    return _storage_path;
}
