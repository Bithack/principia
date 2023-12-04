#include <stdlib.h>
#ifdef _MSC_VER
#else
#include <sys/time.h>
#endif
#include <windows.h>
#include <windowsx.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <clocale>

#include <tms/core/project.h>
#include <tms/core/event.h>
#include <tms/core/tms.h>

#include <tms/backend/opengl.h>
#include <tms/backends/common.h>

#include "settings.hh"
#include "menu_main.hh"

#include "shlwapi.h"

#include "main.hh"
#include "version.hh"

#define SDL_WINDOW_FULLSCREEN_DESKTOP (SDL_WINDOW_FULLSCREEN | 0x00001000)

#include "SDL_syswm.h"

SDL_Window *_window;

FILE *_f_out = stdout;
FILE *_f_err = stderr;

int keys[235];
int mouse_down;
static int _storage_type = 0;
static char *_storage_path = 0;

static int T_intercept_input(SDL_Event ev);

void tgen_init(void){};
extern "C" int tbackend_init_surface();
extern "C" const char *tbackend_get_storage_path(void);

char *_tmp[]={0,0};
static HANDLE pipe_h;
static uint8_t buf[512];

int _pipe_listener(void *p)
{
    DWORD num_read;

    while (ConnectNamedPipe(pipe_h, 0) || GetLastError() == ERROR_PIPE_CONNECTED) {
        tms_infof("Client connected, reading...");

        if (ReadFile(pipe_h, buf, 511, &num_read, 0)) {
            tms_infof("read %u bytes:", num_read);
            buf[num_read] = '\0';
            tms_infof("%s", buf);

            _tmp[1] = (char*)buf;

            tproject_set_args(2, _tmp);
        } else
            tms_infof("error reading from pipe: %d", GetLastError());

        FlushFileBuffers(pipe_h);
        DisconnectNamedPipe(pipe_h);
    }

    tms_infof("ConnectNamedPipe returned false, quitting");

    CloseHandle(pipe_h);

    return T_OK;
}

static void
_catch_signal(int signal)
{
    tms_infof("SIGSEGV");
    if (_f_out != stdout) {
        fflush(_f_out);
        fclose(_f_out);
    }
    exit(1);
}

int CALLBACK
WinMain(HINSTANCE hi, HINSTANCE hp, LPSTR cl, int cs)
{
    SDL_Event  ev;
    int        done = 0;

    setlocale(LC_ALL, "C");
    signal(SIGSEGV, _catch_signal);

    pipe_h = CreateNamedPipe(
            L"\\\\.\\pipe\\principia-process",
            PIPE_ACCESS_INBOUND,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
            1,
            128,
            128,
            0,
            0
            );

    if (pipe_h == INVALID_HANDLE_VALUE) {
        /* could not create named pipe */
        tms_infof("Forwarding arguments through pipe...");

        while (1) {
            pipe_h = CreateFile(
                    L"\\\\.\\pipe\\principia-process",
                    GENERIC_WRITE,
                    0,
                    0,
                    OPEN_EXISTING,
                    0,
                    0
                );

            if (pipe_h != INVALID_HANDLE_VALUE)
                break;

            if (GetLastError() != ERROR_PIPE_BUSY) {
                tms_errorf("error opening pipe");
                exit(1);
            }

            tms_infof("Waiting for pipe...");
            if (!WaitNamedPipe((LPCWSTR)pipe_h, 3000))
                tms_errorf("Failed, waited too long.");
        }

        DWORD dwMode = PIPE_READMODE_MESSAGE;
        SetNamedPipeHandleState(
                pipe_h,
                &dwMode,
                0,0);

        int len = strlen(cl);
        DWORD written;
        if (!(WriteFile(pipe_h, cl, len, &written, 0))) {
            tms_errorf("error writing to pipe");
        }

        tms_infof("done");
        CloseHandle(pipe_h);

        /* bring the window to the front */
        HWND h = FindWindow(NULL, L"Principia");
        SetForegroundWindow(h);
        exit(0);
    } else {
        /* we've created the named pipe */
        tms_infof("Created named pipe, starting listener thread.");
        SDL_CreateThread(_pipe_listener, "_pipe_listener", 0);
    }

    /* Only chdir .. if it's a debug build */
#ifdef DEBUG
    tms_infof("chdirring back!");
    chdir("..");
#else
    wchar_t current_dir[MAX_PATH];
    char current_dir_2[MAX_PATH];
    GetModuleFileName(NULL, current_dir, MAX_PATH);
    PathRemoveFileSpec(current_dir);
    wcstombs(current_dir_2, current_dir,MAX_PATH);
    tms_infof("chdirring to %s", current_dir_2);
    chdir(current_dir_2);
#endif

    // Switch to portable if ./portable.txt exists next to exe
    if (access("portable.txt", F_OK) == 0) {
        _storage_type = 1;
    }

    mkdir(tbackend_get_storage_path());

    /* set temporary width and height for the settings loader */
    SDL_Init(SDL_INIT_VIDEO);

    SDL_DisplayMode mode;
    SDL_GetCurrentDisplayMode(0, &mode);

    _tms.window_width = 1280;

    if (mode.w <= 1280) {
        _tms.window_width = (int)((double)mode.w * .9);
    } else if (mode.w >= 2100 && mode.h > 1100) {
        _tms.window_width = 1920;
    }
    _tms.window_height = (int)((double)_tms.window_width * .5625);

    tms_infof("set initial res to %dx%d", _tms.window_width, _tms.window_height);

    settings.init();
    tms_progressf("Loading settings... ");
    if (!settings.load()) {
        tms_progressf("ERROR\n");
    } else {
        tms_progressf("OK\n");
    }
    P.loaded_correctly_last_run = settings["loaded_correctly"]->v.b;

    settings["is_very_shitty"]->v.b = (!settings["loaded_correctly"]->v.b || settings["is_very_shitty"]->v.b);
    settings["loaded_correctly"]->v.b = false;
    settings.save();

    tms_infof("Texture quality: %d", settings["texture_quality"]->v.i8);
    tms_infof("Shadow quality: %d (%dx%d)",
            settings["shadow_quality"]->v.i8,
            settings["shadow_map_resx"]->v.i,
            settings["shadow_map_resy"]->v.i);

    _tmp[1] = cl;
    tproject_set_args(2, _tmp);
    tms_init();

    SDL_EventState(SDL_SYSWMEVENT, settings["emulate_touch"]->is_true() ? SDL_ENABLE : SDL_DISABLE);

    if (_tms.screen == 0) {
        tms_fatalf("context has no initial screen, bailing out");
    }

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
                    //if (_tms.screen == &P.s_game->super) {
                    //    ui::open_dialog(DIALOG_CONFIRM_QUIT);
                    //} else {
                        _tms.state = TMS_STATE_QUITTING;
                    //}
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
                        case SDL_WINDOWEVENT_RESIZED:
                            {
                                RESIZE_WINDOW;
                            }
                            break;
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
                case SDL_MOUSEMOTION:
                    {
                        if (settings["emulate_touch"]->is_false()) {
                            //tms_infof("from sdl mouse thing");
                            T_intercept_input(ev);
                        }
                    }
                    break;

                case SDL_SYSWMEVENT:
                    {
                        SDL_SysWMmsg *msg = (SDL_SysWMmsg*)ev.syswm.msg;

                        switch (msg->msg.win.msg) {
                            case WM_MOUSEMOVE:
                                {
                                    //tms_infof("MOUSE MOVE");

                                    SDL_Event user_event;
                                    user_event.type = SDL_MOUSEMOTION;
                                    user_event.button.x = GET_X_LPARAM(msg->msg.win.lParam);
                                    user_event.button.y = GET_Y_LPARAM(msg->msg.win.lParam);
                                    user_event.button.button = SDL_BUTTON_LEFT;

                                    T_intercept_input(user_event);
                                }
                                break;

                            case WM_LBUTTONDOWN:
                                {
                                    //tms_infof("LBUTTON DOWN");
                                    SDL_Event user_event;
                                    user_event.type = SDL_MOUSEBUTTONDOWN;
                                    user_event.button.x = GET_X_LPARAM(msg->msg.win.lParam);
                                    user_event.button.y = GET_Y_LPARAM(msg->msg.win.lParam);
                                    user_event.button.button = SDL_BUTTON_LEFT;

                                    T_intercept_input(user_event);
                                }
                                break;

                            case WM_LBUTTONUP:
                                {
                                    //tms_infof("LBUTTON UP");
                                    SDL_Event user_event;
                                    user_event.type = SDL_MOUSEBUTTONUP;
                                    user_event.button.x = GET_X_LPARAM(msg->msg.win.lParam);
                                    user_event.button.y = GET_Y_LPARAM(msg->msg.win.lParam);
                                    user_event.button.button = SDL_BUTTON_LEFT;

                                    T_intercept_input(user_event);
                                }
                                break;

                            case WM_RBUTTONDOWN:
                                {
                                    //tms_infof("RBUTTON DOWN");
                                    SDL_Event user_event;
                                    user_event.type = SDL_MOUSEBUTTONDOWN;
                                    user_event.button.x = GET_X_LPARAM(msg->msg.win.lParam);
                                    user_event.button.y = GET_Y_LPARAM(msg->msg.win.lParam);
                                    user_event.button.button = SDL_BUTTON_RIGHT;

                                    T_intercept_input(user_event);
                                }
                                break;

                            case WM_RBUTTONUP:
                                {
                                    //tms_infof("RBUTTON UP");
                                    SDL_Event user_event;
                                    user_event.type = SDL_MOUSEBUTTONUP;
                                    user_event.button.x = GET_X_LPARAM(msg->msg.win.lParam);
                                    user_event.button.y = GET_Y_LPARAM(msg->msg.win.lParam);
                                    user_event.button.button = SDL_BUTTON_RIGHT;

                                    T_intercept_input(user_event);
                                }
                                break;
                        }
                    }
                    break;

                case SDL_TEXTINPUT:
                    T_intercept_input(ev);
                    break;
                
                default:
                    tms_debugf("Unhandled input: %d", ev.type);
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

            //SDL_PushEvent(&user_event);
        }
    } while (_tms.state != TMS_STATE_QUITTING);

    tproject_quit();

    return 0;
}

int
tbackend_init_surface()
{
    CUTE_ASCII_ART;

    /* Set up SDL, create a window */
    tms_progressf("Initializing SDL... ");
    tms_progressf("OK\n");

    _tms.window_width = settings["window_width"]->v.i;
    _tms.window_height = settings["window_height"]->v.i;

    _tms.xppcm = 108.f/2.54f * 1.5f;
    _tms.yppcm = 107.f/2.54f * 1.5f;

    uint32_t flags = 0;

    flags |= SDL_WINDOW_OPENGL;
    flags |= SDL_WINDOW_SHOWN;
    flags |= SDL_WINDOW_RESIZABLE;

    if (settings["window_maximized"]->v.b)
        flags |= SDL_WINDOW_MAXIMIZED;

    tms_progressf("Creating window... ");
    _window = SDL_CreateWindow("Principia", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, _tms.window_width, _tms.window_height, flags);
    if (_window == NULL) {
        tms_progressf("ERROR: %s\n", SDL_GetError());
        exit(1);
    } else
        tms_progressf("OK\n");

    _tms._window = _window;

    //SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    //SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 0);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);

    SDL_GLContext gl_context = SDL_GL_CreateContext(_window);

    if (gl_context == NULL) {
        tms_fatalf("Error creating GL Context: %s", SDL_GetError());
    }

    if (settings["vsync"]->v.b) {
        if (SDL_GL_SetSwapInterval(-1) == -1)
            SDL_GL_SetSwapInterval(1);
    } else
        SDL_GL_SetSwapInterval(0);

    tms_progressf("Initializing GLEW... ");
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        tms_progressf("ERROR: %s\n", glewGetErrorString(err));
        exit(1);
    }
    tms_progressf("OK (v%s)\n", glewGetString(GLEW_VERSION));

    tms_infof("GL Info: %s/%s/%s", glGetString(GL_VENDOR), glGetString(GL_RENDERER), glGetString(GL_VERSION));
    tms_infof("GLSL Version: %s", glGetString(GL_SHADING_LANGUAGE_VERSION));
    tms_infof("Extensions: %s", glGetString(GL_EXTENSIONS));

    tms_progressf("GL versions supported: ");

    if (GLEW_VERSION_4_4) {
        tms_progressf("4.4, ");
    } if (GLEW_VERSION_4_3) {
        tms_progressf("4.3, ");
    } if (GLEW_VERSION_4_2) {
        tms_progressf("4.2, ");
    } if (GLEW_VERSION_4_1) {
        tms_progressf("4.1, ");
    } if (GLEW_VERSION_3_3) {
        tms_progressf("3.3, ");
    } if (GLEW_VERSION_3_1) {
        tms_progressf("3.1, ");
    } if (GLEW_VERSION_3_0) {
        tms_progressf("3.0, ");
    } if (GLEW_VERSION_2_1) {
        tms_progressf("2.1, ");
    } if (GLEW_VERSION_2_0) {
        tms_progressf("2.0, ");
    } if (GLEW_VERSION_1_5) {
        tms_progressf("1.5, ");
    } if (GLEW_VERSION_1_4) {
        tms_progressf("1.4, ");
    } if (GLEW_VERSION_1_3) {
        tms_progressf("1.3, ");
    } if (GLEW_VERSION_1_2) {
        tms_progressf("1.2, ");
    } if (GLEW_VERSION_1_1) {
        tms_progressf("1.1");
    }

    tms_progressf("\n");

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

static DWORD finger_ids[MAX_P];

static int finger_to_pointer(DWORD finger, bool create)
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
            if (ev.key.repeat) {
                spec.type = TMS_EV_KEY_REPEAT;
            } else {
                spec.type = TMS_EV_KEY_PRESS;
            }

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
            //tms_infof("FINGER DOWN %d->%d", ev.tfinger.fingerId, f);
            spec.data.button.pointer_id = f;

            spec.data.button.x = (int)(ev.tfinger.x*(float)_tms.window_width);
            spec.data.button.y = _tms.window_height-(int)(ev.tfinger.y*(float)_tms.window_height);
            spec.data.button.button = 0;
            break;

        case SDL_FINGERUP:
            spec.type = TMS_EV_POINTER_UP;

            f = finger_to_pointer(ev.tfinger.fingerId, false);
            //tms_infof("FINGER UP %d->%d", ev.tfinger.fingerId, f);
            spec.data.button.pointer_id = f;
            spec.data.button.x = (int)(ev.tfinger.x*(float)_tms.window_width);
            spec.data.button.y = _tms.window_height-(int)(ev.tfinger.y*(float)_tms.window_height);
            spec.data.button.button = 0;

            finger_ids[f] = 0;
            break;

        case SDL_FINGERMOTION:
            spec.type = TMS_EV_POINTER_DRAG;

            f = finger_to_pointer(ev.tfinger.fingerId, false);
            //tms_infof("FINGER MOTION %d->%d", ev.tfinger.fingerId, f);
            spec.data.button.pointer_id = f;
            spec.data.button.x = (int)(ev.tfinger.x*(float)_tms.window_width);
            spec.data.button.y = _tms.window_height-(int)(ev.tfinger.y*(float)_tms.window_height);
            spec.data.button.button = 0;

            break;

        case SDL_MOUSEBUTTONDOWN:
            if (ev.button.which == SDL_TOUCH_MOUSEID) {
                return T_OK;
            }

            spec.type = TMS_EV_POINTER_DOWN;
            spec.data.button.pointer_id = mouse_button_to_pointer_id(ev.button.button);
            spec.data.button.x = ev.button.x;
            spec.data.button.y = button_y;
            spec.data.button.button = ev.button.button;

            /*
            tms_infof("MOUSE BUTTON DOWN %d %d %d %d",
                    ev.button.which,
                    spec.data.button.pointer_id,
                    spec.data.button.button,
                    ev.button.button);
                    */

            if (mouse_down == 0)
                mouse_down = ev.button.button;
            break;

        case SDL_MOUSEBUTTONUP:
            if (ev.button.which == SDL_TOUCH_MOUSEID) {
                return T_OK;
            }

            spec.type = TMS_EV_POINTER_UP;
            spec.data.button.pointer_id = mouse_button_to_pointer_id(ev.button.button);
            spec.data.button.x = ev.button.x;
            spec.data.button.y = button_y;
            spec.data.button.button = ev.button.button;

            /*
            tms_infof("MOUSE BUTTON UP %d %d %d %d",
                    ev.button.which,
                    spec.data.button.pointer_id,
                    spec.data.button.button,
                    ev.button.button);
                    */

            if (mouse_down == ev.button.button)
                mouse_down = 0;
            break;

        case SDL_MOUSEMOTION:
            if (ev.button.which == SDL_TOUCH_MOUSEID)
                return T_OK;

            //tms_infof("MOUSE MOTION");

            //spec.data.button.pointer_id = 0;
            spec.data.button.pointer_id = mouse_button_to_pointer_id(ev.button.button);

            if (mouse_down) {
                spec.type = TMS_EV_POINTER_DRAG;
                spec.data.button.x = ev.motion.x;
                spec.data.button.y = button_y;
                spec.data.button.button = mouse_down;

                /*
                tms_infof("MOUSE MOTION %.2f %.2f %d",
                        spec.data.button.x,
                        spec.data.button.y,
                        spec.data.button.button
                        );
                        */
            } else {
                spec.type = TMS_EV_POINTER_MOVE;
                spec.data.button.x = ev.motion.x;
                spec.data.button.y = motion_y;

                /*
                tms_infof("MOUSE MOTION %.2f %.2f",
                        spec.data.button.x,
                        spec.data.button.y
                        );
                        */
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
            strcpy(path, getenv("USERPROFILE"));
            strcat(path, "\\Principia");
        } else if (_storage_type == 1) { // Portable
            strcpy(path, ".\\userdata\\");
        }

        _storage_path = path;
    }
    return _storage_path;
}

void
tbackend_toggle_fullscreen(void)
{
    uint32_t flags = SDL_GetWindowFlags(_window);

    if (flags & SDL_WINDOW_FULLSCREEN)
        SDL_SetWindowFullscreen(_window, SDL_FALSE);
    else
        SDL_SetWindowFullscreen(_window, SDL_TRUE);
}
