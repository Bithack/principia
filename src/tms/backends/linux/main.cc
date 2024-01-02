#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <cxxabi.h>
#include <unistd.h>
#include <libgen.h>

#include <tms/core/project.h>
#include <tms/core/event.h>
#include <tms/core/tms.h>

#include <tms/backend/opengl.h>
#include <tms/backends/common.h>

#ifdef DEBUG
#include <fenv.h>
#endif

#include "settings.hh"
#include "main.hh"
#include "version.hh"

SDL_Window *_window;

int keys[235];
int mouse_down;
static char *_storage_path = 0;
static int pipe_h;

static int T_intercept_input(SDL_Event ev);

static char *_args[2] = {0,0};
static char buf[1024];

void tgen_init(void){};
extern "C" int tbackend_init_surface();
extern "C" const char *tbackend_get_storage_path(void);

int _pipe_listener(void *p)
{
    ssize_t sz;

    while (1) {
        tms_infof("attempting to open /tmp/principia.run O_RDONLY");
        while ((pipe_h = open("/tmp/principia.run", O_RDONLY)) == -1) {
            if (errno != EINTR)
                return 1;
        }

        while ((sz = read(pipe_h, buf, 1023)) > 0) {
            //tms_infof("read %d bytes", sz);

            if (sz > 0) {
                buf[sz] = '\0';
                _args[1] = buf;
                tproject_set_args(2, _args);
            }
        }

        close(pipe_h);
    }

    tms_infof("Pipe listener EXITING");
}

int
main(int argc, char **argv)
{
#ifdef DEBUG
    //feenableexcept(FE_INVALID | FE_OVERFLOW);
#endif

    SDL_Event  ev;
    int        done = 0;

    int status = mkfifo("/tmp/principia.run", S_IWUSR | S_IRUSR);
    int skip_pipe = 0;

    if (status == 0) {
        tms_infof("Created fifo");
    } else {
        if (errno != EEXIST) {
            tms_errorf("could not create fifo pipe!");
            skip_pipe = 1;
        }
    }

    if (!skip_pipe) {
        if ((pipe_h = open("/tmp/principia.run", O_WRONLY | O_NONBLOCK)) == -1) {
            if (errno != ENXIO) {
                skip_pipe = 1;
                tms_infof("error: %s", strerror(errno));
            }
        } else {
            if (argc > 1) {
                /* open the fifo for writing instead */
                tms_infof("sending arg: %s", argv[1]);
                write(pipe_h, argv[1], strlen(argv[1]));

            } else {
                tms_infof("principia already running");
            }

            close(pipe_h);
            exit(0);
        }
    }

    if (!skip_pipe) {
        tms_infof("Starting fifo listener thread");
        SDL_CreateThread(_pipe_listener, "_pipe_listener", 0);
    }

    char buf[512];
    readlink("/proc/self/exe", buf, 511);
    dirname(buf);
    tms_infof("chdirring to %s", buf);
    chdir(buf);

    // Check if we're in the right place
    struct stat st{};
    if (stat("data-shared", &st) != 0) {
        // We're in the build dir, go up
        tms_infof("chdirring to ../");
        chdir("../");

        // How about now?
        if (stat("data-shared", &st) != 0) {
            // If that doesn't work we're assuming a system install.
            tms_infof("chdirring to ./share/principia/");
            chdir("./share/principia/");

            if (stat("data-shared", &st) != 0) {
                // We're doomed, better just fail.
                tms_fatalf("Could not find data directories.");
            }
        }
    }

    mkdir(tbackend_get_storage_path(), S_IRWXU | S_IRWXG | S_IRWXO);

    tms_progressf("Initializing SDL... ");
    SDL_Init(SDL_INIT_VIDEO);
    tms_progressf("OK\n");
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
    settings.load();
    P.loaded_correctly_last_run = settings["loaded_correctly"]->v.b;

    settings["is_very_shitty"]->v.b = (!settings["loaded_correctly"]->v.b || settings["is_very_shitty"]->v.b);
    settings["loaded_correctly"]->v.b = false;
    settings.save();

    tms_infof("Texture quality: %d", settings["texture_quality"]->v.i8);
    tms_infof("Shadow quality: %d (%dx%d)",
            settings["shadow_quality"]->v.i8,
            settings["shadow_map_resx"]->v.i,
            settings["shadow_map_resy"]->v.i);

    tproject_set_args(argc, argv);
    tms_init();

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
                            RESIZE_WINDOW;
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
                case SDL_MOUSEMOTION:
                    {
                        if (settings["emulate_touch"]->is_false()) {
                            //tms_infof("from sdl mouse thing");
                            T_intercept_input(ev);
                        }
                    }
                    break;

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
    CUTE_ASCII_ART;

    _tms.window_width = settings["window_width"]->v.i;
    _tms.window_height = settings["window_height"]->v.i;

    /* default */
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
    } else {
        tms_progressf("OK\n");
    }

    _tms._window = _window;

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
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

    if (GLEW_VERSION_4_4)
        tms_progressf("4.4, ");
    if (GLEW_VERSION_4_3)
        tms_progressf("4.3, ");
    if (GLEW_VERSION_4_2)
        tms_progressf("4.2, ");
    if (GLEW_VERSION_4_1)
        tms_progressf("4.1, ");
    if (GLEW_VERSION_3_3)
        tms_progressf("3.3, ");
    if (GLEW_VERSION_3_1)
        tms_progressf("3.1, ");
    if (GLEW_VERSION_3_0)
        tms_progressf("3.0, ");
    if (GLEW_VERSION_2_1)
        tms_progressf("2.1, ");
    if (GLEW_VERSION_2_0)
        tms_progressf("2.0, ");
    if (GLEW_VERSION_1_5)
        tms_progressf("1.5, ");
    if (GLEW_VERSION_1_4)
        tms_progressf("1.4, ");
    if (GLEW_VERSION_1_3)
        tms_progressf("1.3, ");
    if (GLEW_VERSION_1_2)
        tms_progressf("1.2, ");
    if (GLEW_VERSION_1_1)
        tms_progressf("1.1");

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
                spec.data.motion.x = ev.motion.x;
                spec.data.motion.y = motion_y;
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
        struct passwd *pw = getpwuid(getuid());

        strcpy(path, pw->pw_dir);
        strcat(path, "/.principia");

        _storage_path = path;
    }
    return _storage_path;
}

void
tbackend_toggle_fullscreen(void)
{
    uint32_t flags = SDL_GetWindowFlags(_window);

    if (flags & SDL_WINDOW_FULLSCREEN) {
        SDL_SetWindowFullscreen(_window, SDL_FALSE);
    } else {
        SDL_SetWindowFullscreen(_window, SDL_TRUE);
    }
}

void
tms_trace(void)
{
#ifdef DEBUG
    void *array[10];
    size_t size;
    char **strings;
    size_t i;

    size = backtrace(array, 10);
    strings = backtrace_symbols(array, size);

    size_t funcnamesize = 256;
    char *funcname = (char*)malloc(funcnamesize);

    for (i=1; i<size; ++i) {
        char *begin_name = 0, *begin_offset = 0, *end_offset = 0;

        // find parentheses and +address offset surrounding the mangled name:
        // ./module(function+0x15c) [0x8048a6d]
        for (char *p = strings[i]; *p; ++p) {
            if (*p == '(') {
                begin_name = p;
            } else if (*p == '+') {
                begin_offset = p;
            } else if (*p == ')' && begin_offset) {
                end_offset = p;
                break;
            }
        }

        if (begin_name && begin_offset && end_offset && begin_name < begin_offset) {
            *begin_name++ = '\0';
            *begin_offset++ = '\0';
            *end_offset = '\0';

            // mangled name is now in [begin_name, begin_offset) and caller
            // offset in [begin_offset, end_offset). now apply
            // __cxa_demangle():

            int status;
            char* ret = abi::__cxa_demangle(begin_name,
                    funcname, &funcnamesize, &status);

            if (status == 0) {
                funcname = ret; // use possibly realloc()-ed string
                fprintf(stderr, "  %s : %s+%s\n",
                        strings[i], funcname, begin_offset);
            } else {
                // demangling failed. Output function name as a C function with
                // no arguments.
                fprintf(stderr, "  %s : %s()+%s\n",
                        strings[i], begin_name, begin_offset);
            }
        } else {
            // couldn't parse the line? print the whole line.
            fprintf(stderr, "  %s\n", strings[i]);
        }
    }

    free(funcname);
    free(strings);
#endif
}
