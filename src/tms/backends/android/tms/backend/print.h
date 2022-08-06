#ifndef _BACKEND_PRINT__H_
#define _BACKEND_PRINT__H_

#include <stdlib.h>
#include <android/log.h>

#define tms_infof(...) (__android_log_print(ANDROID_LOG_INFO, "tms", __VA_ARGS__))
#define tms_warnf(...) (__android_log_print(ANDROID_LOG_WARN, "tms", __VA_ARGS__))
#define tms_fatalf(...) ((__android_log_print(ANDROID_LOG_ERROR, "tms", __VA_ARGS__)), _fatal_exit())
#define tms_errorf(...) __android_log_print(ANDROID_LOG_ERROR, "tms", __VA_ARGS__)

#if DEBUG
#define tms_progressf(...) (__android_log_print(ANDROID_LOG_INFO, "tms", __VA_ARGS__))
#else
#define tms_progressf(...)
#endif

static inline int _fatal_exit() {
    exit(1);
    return 0;
}

static void
tms_trace(void)
{
}

#if DEBUG
#define tms_debugf(...) (void)__android_log_print(ANDROID_LOG_INFO, "tms", __VA_ARGS__);
#else
#define tms_debugf(...) {}
#endif

#define tms_raise()

#ifdef DEBUG
#define tms_assertf(expr, f, ...) {if (expr);else tms_fatalf(f, ##__VA_ARGS__);}
#else
#define tms_assertf(...)
#endif

#define tms_fatal tfatalf

#endif
