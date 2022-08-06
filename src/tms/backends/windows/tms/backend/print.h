#ifndef _BACKEND_PRINT__H_
#define _BACKEND_PRINT__H_

#include <stdlib.h>
#include <stdio.h>

#include <tms/util/glob.h>

#undef near
#undef far

extern FILE *_f_out;
extern FILE *_f_err;

#define tms_infof(f, ...) fprintf(_f_out, "III: " f "\n", ##__VA_ARGS__), fflush(_f_out);
#define tms_warnf(f, ...) fprintf(_f_err, "warning: " f "\n", ##__VA_ARGS__), fflush(_f_err);
#define tms_fatalf(f, ...) (fprintf(_f_err, "fatal error: " f "\n", ##__VA_ARGS__), fflush(_f_err), _fatal_exit())
#define tms_errorf(f, ...) fprintf(_f_err, "error: " f "\n", ##__VA_ARGS__), fflush(_f_err);

TMS_STATIC_INLINE void
tms_set_log_file(FILE *out, FILE *err)
{
    tms_infof("Log file changing!");
    _f_out = out;
    _f_err = err;
};

#define tms_progressf(f, ...) fprintf(_f_out, f, ##__VA_ARGS__); fflush(_f_out);

#if DEBUG
#define tms_debugf(f, ...) fprintf(_f_err, "(debug) %s(): " f "\n",  __func__, ##__VA_ARGS__), fflush(_f_err);
#else
#define tms_debugf(...)
#endif

#define tms_raise()

TMS_STATIC_INLINE int
_fatal_exit() {
    exit(1);
    return 0;
};

static void
tms_trace(void)
{
};

#ifdef DEBUG
#define tms_assertf(expr, f, ...) {if (expr);else tms_fatalf(f, ##__VA_ARGS__);}
#else
#define tms_assertf(...)
#endif

#define tfatal tfatalf

#endif
