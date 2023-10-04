
// This file contains common snippets shared across backends

#define CUTE_ASCII_ART \
	tms_progressf( \
		"            _            _       _       \n"    \
		" _ __  _ __(_)_ __   ___(_)_ __ (_) __ _ \n"    \
		"| '_ \\| '__| | '_ \\ / __| | '_ \\| |/ _` |\n" \
		"| |_) | |  | | | | | (__| | |_) | | (_| |\n"    \
		"| .__/|_|  |_|_| |_|\\___|_| .__/|_|\\__,_|\n"  \
		"|_|                       |_|            \n"    \
		"Version: %d. " __DATE__ "/" __TIME__ "\n", PRINCIPIA_VERSION_CODE);

#define RESIZE_WINDOW \
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
