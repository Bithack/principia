#ifndef _BACKEND_PRINT__H_
#define _BACKEND_PRINT__H_

#include <stdlib.h>
#include <stdio.h>

static inline int _fatal_exit(void) {
    exit(1);
    return 0;
}

#if defined(TMS_BACKEND_ANDROID)

	#include <android/log.h>

	#define tms_infof(...) (__android_log_print(ANDROID_LOG_INFO, "tms", __VA_ARGS__))
	#define tms_warnf(...) (__android_log_print(ANDROID_LOG_WARN, "tms", __VA_ARGS__))
	#define tms_fatalf(...) ((__android_log_print(ANDROID_LOG_ERROR, "tms", __VA_ARGS__)), _fatal_exit())
	#define tms_errorf(...) __android_log_print(ANDROID_LOG_ERROR, "tms", __VA_ARGS__)

	#if DEBUG
		#define tms_printf(...) (__android_log_print(ANDROID_LOG_INFO, "tms", __VA_ARGS__))
		#define tms_debugf(...) (__android_log_print(ANDROID_LOG_DEBUG, "tms", __VA_ARGS__));
	#endif

#elif defined(TMS_BACKEND_IOS)

	// todo
	#error "NYI"

#else

	extern FILE *_f_out;

	#define tms_infof(f, ...) fprintf(_f_out, "I: " f "\n", ##__VA_ARGS__), fflush(_f_out);
	#define tms_warnf(f, ...) fprintf(_f_out, "W: " f "\n", ##__VA_ARGS__), fflush(_f_out);
	#define tms_fatalf(f, ...) (fprintf(_f_out, "F: " f "\n", ##__VA_ARGS__), fflush(_f_out), _fatal_exit())
	#define tms_errorf(f, ...) fprintf(_f_out, "E: " f "\n", ##__VA_ARGS__), fflush(_f_out);

	#define tms_printf(f, ...) fprintf(_f_out, f, ##__VA_ARGS__); fflush(_f_out);

	#if DEBUG
		#define tms_debugf(f, ...) fprintf(_f_out, "D: " f "\n", ##__VA_ARGS__), fflush(_f_out);
	#endif

#endif

// Common logging functions, and fallbacks

#ifdef DEBUG
	#define tms_assertf(expr, f, ...) { if (expr); else tms_fatalf(f, ##__VA_ARGS__); }
#else
	#define tms_assertf(...)
#endif

#ifndef tms_debugf
	#define tms_debugf(...)
#endif

#ifndef tms_printf
	#define tms_printf(...)
#endif

#endif
