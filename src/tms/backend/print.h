#ifndef _BACKEND_PRINT__H_
#define _BACKEND_PRINT__H_

#include <stdlib.h>
#include <stdio.h>

#if defined(TMS_BACKEND_LINUX) && defined(DEBUG)
    #include <signal.h>
#endif

static void tms_trace(void) {}

static inline int _fatal_exit() {
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
		#define tms_progressf(...) (__android_log_print(ANDROID_LOG_INFO, "tms", __VA_ARGS__))
	#endif

	#if DEBUG
		#define tms_debugf(...) (void)__android_log_print(ANDROID_LOG_INFO, "tms", __VA_ARGS__);
	#endif

#elif defined(TMS_BACKEND_IOS)

	// todo
	#error "NYI"

#elif defined(TMS_BACKEND_WINDOWS)

	#include <tms/util/glob.h>

	#undef near
	#undef far

	extern FILE *_f_out;
	extern FILE *_f_err;

	#define tms_infof(f, ...) fprintf(_f_out, "III: " f "\n", ##__VA_ARGS__), fflush(_f_out);
	#define tms_warnf(f, ...) fprintf(_f_err, "warning: " f "\n", ##__VA_ARGS__), fflush(_f_err);
	#define tms_fatalf(f, ...) (fprintf(_f_err, "fatal error: " f "\n", ##__VA_ARGS__), fflush(_f_err), _fatal_exit())
	#define tms_errorf(f, ...) fprintf(_f_err, "error: " f "\n", ##__VA_ARGS__), fflush(_f_err);

	static inline void
	tms_set_log_file(FILE *out, FILE *err)
	{
		tms_infof("Log file changing!");
		_f_out = out;
		_f_err = err;
	};

	#define tms_progressf(f, ...) fprintf(_f_out, f, ##__VA_ARGS__); fflush(_f_out);

	#if DEBUG
		#define tms_debugf(f, ...) fprintf(_f_err, "(debug) %s(): " f "\n",  __func__, ##__VA_ARGS__), fflush(_f_err);
	#endif

#else

	// Every other platform (Linux, screenshotter...)

	#define tms_infof(f, ...) fprintf(stdout, "III: " f "\n", ##__VA_ARGS__);
	#define tms_warnf(f, ...) fprintf(stderr, "warning: " f "\n", ##__VA_ARGS__);
	#define tms_fatalf(f, ...) (fprintf(stderr, "fatal error: " f "\n", ##__VA_ARGS__), _fatal_exit());
	#define tms_errorf(f, ...) fprintf(stderr, "error: " f "\n", ##__VA_ARGS__);
	#define tms_progressf(f, ...) fprintf(stdout, f, ##__VA_ARGS__); fflush(stdout);

	#ifdef DEBUG
		#define tms_debugf(f, ...) fprintf(stderr, "(debug) %s(): " f "\n",  __func__, ##__VA_ARGS__);
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

#ifndef tms_progressf
	#define tms_progressf(...)
#endif

#endif
