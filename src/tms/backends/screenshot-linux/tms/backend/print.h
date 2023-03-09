#ifndef _BACKEND_PRINT__H_
#define _BACKEND_PRINT__H_

#include <stdlib.h>
#include <stdio.h>

#define tms_infof(f, ...) fprintf(stdout, "III: " f "\n", ##__VA_ARGS__);

#define tms_warnf(f, ...) fprintf(stderr, "warning: " f "\n", ##__VA_ARGS__);

#define tms_fatalf(f, ...) (fprintf(stderr, "fatal error: " f "\n", ##__VA_ARGS__), _fatal_exit());

#define tms_errorf(f, ...) fprintf(stderr, "error: " f "\n", ##__VA_ARGS__);

#define tms_progressf(f, ...) fprintf(stdout, f, ##__VA_ARGS__); fflush(stdout);

#if DEBUG
#define tms_debugf(f, ...) fprintf(stderr, "(debug) %s(): " f "\n",  __func__, ##__VA_ARGS__);
#else
#define tms_debugf(...)
#endif

#define tms_raise()

static inline int _fatal_exit() {
    exit(1);
    return 0;
}

#ifdef __cplusplus
extern "C" {
#endif

void tms_trace(void);

#ifdef __cplusplus
}
#endif

#ifdef DEBUG
#define tms_assertf(expr, f, ...) {if (expr);else tms_fatalf(f, ##__VA_ARGS__);}
#else
#define tms_assertf(...)
#endif

#define tfatal tfatalf

#endif
