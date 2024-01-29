
// This file contains common snippets shared across desktop backends.


#define TOGGLE_FULLSCREEN \
	uint32_t flags = SDL_GetWindowFlags(_window); \
\
    if (flags & SDL_WINDOW_FULLSCREEN) { \
        SDL_SetWindowFullscreen(_window, SDL_FALSE); \
    } else { \
        SDL_SetWindowFullscreen(_window, SDL_TRUE); \
    }

// main/WinMain

#define CHDIR_EXE \
	char* exedir = SDL_GetBasePath(); \
    tms_infof("chdirring to %s", exedir); \
    chdir(exedir);

#define INIT_SDL \
	tms_progressf("Initializing SDL... "); \
    SDL_Init(SDL_INIT_VIDEO); \
    tms_progressf("OK\n"); \
    SDL_DisplayMode mode; \
    SDL_GetCurrentDisplayMode(0, &mode); \

#define RESIZE_WINDOW \
	_tms.window_width = 1280; \
\
    if (mode.w <= 1280) \
        _tms.window_width = (int)((double)mode.w * .9); \
    else if (mode.w >= 2100 && mode.h > 1100) \
        _tms.window_width = 1920; \
\
    _tms.window_height = (int)((double)_tms.window_width * .5625); \
\
    tms_infof("set initial res to %dx%d", _tms.window_width, _tms.window_height);

#define LOAD_SETTINGS \
	settings.init(); \
    tms_progressf("Loading settings... "); \
    if (!settings.load()) { \
        tms_progressf("ERROR\n"); \
    } else \
        tms_progressf("OK\n"); \
    P.loaded_correctly_last_run = settings["loaded_correctly"]->v.b; \
 \
    settings["is_very_shitty"]->v.b = (!settings["loaded_correctly"]->v.b || settings["is_very_shitty"]->v.b); \
    settings["loaded_correctly"]->v.b = false; \
    settings.save(); \
 \
    tms_infof("Texture quality: %d", settings["texture_quality"]->v.i8); \
    tms_infof("Shadow quality: %d (%dx%d)", \
            settings["shadow_quality"]->v.i8, \
            settings["shadow_map_resx"]->v.i, \
            settings["shadow_map_resy"]->v.i);


#define WINDOW_RESIZED \
	tms_infof("Window %d resized to %dx%d", \
			ev.window.windowID, ev.window.data1, \
			ev.window.data2); \
	int w = ev.window.data1; \
	int h = ev.window.data2; \
\
	_tms.window_width  = _tms.opengl_width  = w; \
	_tms.window_height = _tms.opengl_height = h; \
\
	tproject_window_size_changed();



// tbackend_init_surface()

#define CUTE_ASCII_ART \
	tms_progressf( \
		"            _            _       _       \n"    \
		" _ __  _ __(_)_ __   ___(_)_ __ (_) __ _ \n"    \
		"| '_ \\| '__| | '_ \\ / __| | '_ \\| |/ _` |\n" \
		"| |_) | |  | | | | | (__| | |_) | | (_| |\n"    \
		"| .__/|_|  |_|_| |_|\\___|_| .__/|_|\\__,_|\n"  \
		"|_|                       |_|            \n"    \
		"Version: %d. " __DATE__ "/" __TIME__ "\n", PRINCIPIA_VERSION_CODE);

#define CREATE_SDL_WINDOW \
	uint32_t flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE; \
 \
    if (settings["window_maximized"]->v.b) \
        flags |= SDL_WINDOW_MAXIMIZED; \
 \
    tms_progressf("Creating window... "); \
    _window = SDL_CreateWindow("Principia", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, \
		_tms.window_width, _tms.window_height, flags); \
    if (_window == NULL) { \
        tms_progressf("ERROR: %s\n", SDL_GetError()); \
        exit(1); \
    } else \
        tms_progressf("OK\n"); \
 \
    _tms._window = _window;


#define CREATE_GL_CONTEXT \
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1); \
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24); \
    SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1); \
\
    SDL_GLContext gl_context = SDL_GL_CreateContext(_window); \
\
    if (gl_context == NULL) \
        tms_fatalf("Error creating GL Context: %s", SDL_GetError()); \


#define INIT_GLEW \
	tms_progressf("Initializing GLEW... "); \
    glewExperimental = GL_TRUE; \
    GLenum err = glewInit(); \
    if (err != GLEW_OK) { \
        tms_progressf("ERROR: %s\n", glewGetErrorString(err)); \
        exit(1); \
    } \
    tms_progressf("OK (v%s)\n", glewGetString(GLEW_VERSION));

#define PRINT_GL_INFO \
	tms_infof("GL Info: %s/%s/%s", glGetString(GL_VENDOR), glGetString(GL_RENDERER), glGetString(GL_VERSION)); \
    tms_infof("GLSL Version: %s", glGetString(GL_SHADING_LANGUAGE_VERSION)); \
    tms_infof("Extensions: %s", glGetString(GL_EXTENSIONS)); \
\
    tms_progressf("GL versions supported: "); \
    if (GLEW_VERSION_4_6) tms_progressf("4.6,"); \
    if (GLEW_VERSION_4_5) tms_progressf("4.5,"); \
    if (GLEW_VERSION_4_4) tms_progressf("4.4,"); \
    if (GLEW_VERSION_4_3) tms_progressf("4.3,"); \
    if (GLEW_VERSION_4_2) tms_progressf("4.2,"); \
    if (GLEW_VERSION_4_1) tms_progressf("4.1,"); \
    if (GLEW_VERSION_3_3) tms_progressf("3.3,"); \
    if (GLEW_VERSION_3_1) tms_progressf("3.1,"); \
    if (GLEW_VERSION_3_0) tms_progressf("3.0,"); \
    if (GLEW_VERSION_2_1) tms_progressf("2.1,"); \
    if (GLEW_VERSION_2_0) tms_progressf("2.0,"); \
    if (GLEW_VERSION_1_5) tms_progressf("1.5,"); \
    if (GLEW_VERSION_1_4) tms_progressf("1.4,"); \
    if (GLEW_VERSION_1_3) tms_progressf("1.3,"); \
    if (GLEW_VERSION_1_2) tms_progressf("1.2,"); \
    if (GLEW_VERSION_1_1) tms_progressf("1.1"); \
	tms_progressf("\n");
