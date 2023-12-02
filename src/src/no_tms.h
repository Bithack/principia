#ifndef _NO_TMS__H_
#define _NO_TMS__H_

#include "local_config.h"
#include <stdlib.h>
#include <stdio.h>

/*
#define tms_infof(f, ...) fprintf(stdout, "III: " f "\n", ##__VA_ARGS__);
#define tms_warnf(f, ...) fprintf(stderr, "warning: " f "\n", ##__VA_ARGS__);
#define tms_fatalf(f, ...) (fprintf(stderr, "fatal error: " f "\n", ##__VA_ARGS__), _fatal_exit());
#define tms_errorf(f, ...) fprintf(stderr, "error: " f "\n", ##__VA_ARGS__);
*/
#define tms_infof(...)
#define tms_warnf(...)
#define tms_fatalf(...)
#define tms_errorf(...)
#define tms_progressf(f, ...) fprintf(stdout, f, ##__VA_ARGS__); fflush(stdout);
#define tms_debugf(...)
#define tms_raise()
#define tms_assertf(expr, f, ...) {if (expr);else tms_fatalf(f, ##__VA_ARGS__);}

#ifdef PROGRESSTEST
static const char *tbackend_get_storage_path(){return ".";};
#else
static const char *tbackend_get_storage_path(){return STATIC_STORAGE_PATH;};
#endif

static inline int
_fatal_exit() {
    exit(1);
    return 0;
}

#endif
