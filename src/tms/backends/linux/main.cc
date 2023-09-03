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
#include "ui.hh"
#include "game.hh"
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
            tms_infof("chdirring to /usr/share/principia/");
            chdir("/usr/share/principia/");

            if (stat("data-shared", &st) != 0) {
                // We're doomed, better just fail.
                tms_fatalf("Could not find data directories.");
            }
        }
    }

    char path[512];
    const char *storage = tbackend_get_storage_path();
    static const char *dirs[] = {
        "",
        "/lvl", "/lvl/db", "/lvl/local", "/lvl/main",
        "/pkg", "/pkg/db", "/pkg/local", "/pkg/main",
    };

    //tms_infof("Creating directories..");
    for (int x=0; x<sizeof(dirs)/sizeof(char*); x++) {
        /* XXX no bounds checking */
        sprintf(path, "%s%s", storage, dirs[x]);
        mkdir(path, S_IRWXU | S_IRWXG | S_IRWXO);
    }

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

    if (settings["fv"]->v.i == 1) {
        settings["fv"]->v.i = 2;
        settings["cam_speed_modifier"]->v.f = 1.f;
        settings["menu_speed"]->v.f = 1.f;
        settings["smooth_zoom"]->v.b = false;
        settings["smooth_cam"]->v.b = false;
        tms_infof("Modified cam settings.");
    }

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
                    //if (_tms.screen == &G->super) {
                    //    ui::open_dialog(DIALOG_CONFIRM_QUIT);
                    //} else {
                        _tms.state = TMS_STATE_QUITTING;
                    //}
                    break;

                case SDL_WINDOWEVENT:
                    switch (ev.window.event) {
                        case SDL_WINDOWEVENT_RESIZED:
                            {
                                RESIZE_WINDOW;
                            }
                            break;
                    }
                    break;

                case SDL_KEYDOWN:
                    T_intercept_input(ev);
                    keys[ev.key.keysym.scancode] = 1;
                    break;

                case SDL_KEYUP:
                    T_intercept_input(ev);
                    keys[ev.key.keysym.scancode] = 0;
                    break;

                case SDL_MOUSEBUTTONDOWN:
                case SDL_MOUSEBUTTONUP:
                case SDL_MOUSEWHEEL:
                case SDL_MOUSEMOTION:
                //case SDL_INPUTMOTION:
                    T_intercept_input(ev);
                    break;
            }
        }

        tms_step();
        tms_begin_frame();
        tms_render();
        SDL_GL_SwapWindow(_window);
        tms_end_frame();

    } while (_tms.state != TMS_STATE_QUITTING);

    tproject_quit();

    SDL_Quit();

    return 0;
}

#ifdef DEBUG
enum {
    DD_PC,
    DD_PC_27,
    DD_720,
    DD_1080,
    DD_GALAXY_ACE,
    DD_GALAXY_S3,
    DD_GALAXY_S4,
    DD_GALAXY_S5,
    DD_IPHONE_5S,
    DD_IPHONE_4,
    DD_IPAD_MINI,
    DD_IPAD_MINI_RETINA,
    DD_NEXUS_5,
};

static struct device {
    const char *name;
    float xppcm;
    float yppcm;
    int32_t width;
    int32_t height;
    float diagonal;
} devices[] = {
    {
        "PC",
        108.f/2.54f * 1.5f,
        107.f/2.54f * 1.5f,
        -1, -1,
        0.f,
    }, {
        "27\" PC",
        108.79f/2.54f,
        108.79f/2.54f,
        -1, -1,
        0.f,
    }, {
        "1280x720",
        108.f/2.54f * 1.5f,
        107.f/2.54f * 1.5f,
        1280, 720,
        0.f,
    }, {
        "1920x1080",
        108.f/2.54f * 1.5f,
        107.f/2.54f * 1.5f,
        1920, 1080,
        0.f,
    }, {
        "Galaxy ACE",
        165.f / 2.54f,
        165.f / 2.54f,
        480, 320,
        3.5f,
    }, {
        "Galaxy S3",
        306.f / 2.54f,
        306.f / 2.54f,
        1280, 720,
        4.8f,
    }, {
        "Galaxy S4",
        441.f / 2.54f,
        441.f / 2.54f,
        1920, 1080,
        5.f,
    }, {
        "Galaxy S5",
        332.f / 2.54f,
        332.f / 2.54f,
        1920, 1080,
        5.1f,
    }, {
        "iPhone 5s",
        125.98f,
        125.98f,
        1136, 640,
        4.f,
    }, {
        "iPhone 4",
        125.98f,
        125.98f,
        960, 640,
        3.5f,
    }, {
        "iPad mini / iPad 1/2",
        163.f / 2.54f,
        163.f / 2.54f,
        1024, 768,
        7.9f,
    }, {
        "iPad mini Retina",
        324.f / 2.54f,
        324.f / 2.54f,
        2048, 1536,
        7.9f,
    }, {
        "Google Nexus 5",
        445.f / 2.54f,
        445.f / 2.54f,
        1920, 1080,
        4.95f,
    }, {
        "iPad 3",
        264.f / 2.54f,
        264.f / 2.54f,
        2048, 1536,
        9.7f,
    }, {
        "Galaxy Tab 7.0 Plus",
        169.f / 2.54f,
        169.f / 2.54f,
        1024, 600,
        7.f,
    }, {
        "Galaxy Tab 8.9",
        169.f / 2.54f,
        169.f / 2.54f,
        1280, 800,
        8.9f,
    }, {
        "Galaxy Tab 10.1",
        149.f / 2.54f,
        149.f / 2.54f,
        1280, 800,
        10.1f,
    }, {
        "Google Nexus 7",
        216.f / 2.54f,
        216.f / 2.54f,
        1280, 800,
        7.f,
    }, {
        "Google Nexus 7 (2013)",
        323.f / 2.54f,
        323.f / 2.54f,
        1920, 1200,
        7.f,
    }, {
        "Google Nexus 4",
        318.f / 2.54f,
        318.f / 2.54f,
        1280, 720,
        4.7f,
    }, {
        "HTC Evo",
        217.f / 2.54f,
        217.f / 2.54f,
        800, 480,
        4.3f,
    }, {
        "HTC Sensation",
        256.f / 2.54f,
        256.f / 2.54f,
        960, 540,
        4.3f,
    }, {
        "HTC One",
        469.f / 2.54f,
        469.f / 2.54f,
        1920, 1080,
        4.7f,
    }, {
        "HTC Imagio",
        259.f / 2.54f,
        259.f / 2.54f,
        800, 480,
        3.6f,
    }, {
        "HTC One Max",
        373.f / 2.54f,
        373.f / 2.54f,
        1920, 1080,
        5.9f,
    }, {
        "HTC One V",
        252.f / 2.54f,
        252.f / 2.54f,
        800, 480,
        3.7f,
    }, {
        "HTC Pure",
        292.f / 2.54f,
        292.f / 2.54f,
        800, 480,
        3.2f,
    }, {
        "Motorola Moto G",
        326.f / 2.54f,
        326.f / 2.54f,
        1280, 720,
        4.5f,
    }, {
        "Motorola Moto X",
        312.f / 2.54f,
        312.f / 2.54f,
        1280, 720,
        4.7f,
    }, {
        "Nokia Lumia 920",
        332.f / 2.54f,
        332.f / 2.54f,
        1280, 768,
        4.5f,
    }, {
        "Nokia Lumia 820",
        217.f / 2.54f,
        217.f / 2.54f,
        800, 480,
        4.3f,
    }, {
        "Sony Xperia SP",
        282.f / 2.54f,
        282.f / 2.54f,
        1080, 720,
        4.6f,
    }, {
        "ZTE Open",
        165.f / 2.54f,
        165.f / 2.54f,
        480, 320,
        3.5f,
    },
};

static int NUM_DEBUG_DEVICES = sizeof(devices) / sizeof(devices[0]);
static int cur_dd = DD_PC;
#endif

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

    tms_progressf("Creating window... ");
    _window = SDL_CreateWindow("Principia", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, _tms.window_width, _tms.window_height, flags);

    if (_window == NULL) {
        tms_progressf("ERROR: %s\n", SDL_GetError());
        exit(1);
    } else {
        tms_progressf("OK\n");
    }

    _tms._window = _window;

    //SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    //SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
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

int
T_intercept_input(SDL_Event ev)
{
    struct tms_event spec;
    spec.type = -1;

    int motion_y = _tms.window_height-ev.motion.y;
    int button_y = _tms.window_height-ev.button.y;

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

        case SDL_MOUSEBUTTONDOWN:
            spec.type = TMS_EV_POINTER_DOWN;
            spec.data.button.pointer_id = mouse_button_to_pointer_id(ev.button.button);
            spec.data.button.x = ev.button.x;
            spec.data.button.y = button_y;
            spec.data.button.button = ev.button.button;

            if (mouse_down == 0)
                mouse_down = ev.button.button;
            break;

        case SDL_MOUSEBUTTONUP:
            spec.type = TMS_EV_POINTER_UP;
            spec.data.button.pointer_id = mouse_button_to_pointer_id(ev.button.button);
            spec.data.button.x = ev.button.x;
            spec.data.button.y = button_y;
            spec.data.button.button = ev.button.button;

            if (mouse_down == ev.button.button)
                mouse_down = 0;
            break;

        case SDL_MOUSEMOTION:
            //spec.data.button.pointer_id = 0;
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
