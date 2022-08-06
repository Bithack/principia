#ifndef _BACKEND__H_
#define _BACKEND__H_

#if !defined TMS_BACKEND_LINUX && !defined TMS_BACKEND_ANDROID && !defined TMS_BACKEND_WINDOWS && !defined TMS_BACKEND_IOS
#error undefined backend, modify your makefile
#endif

#include <tms/core/err.h>

#ifdef __cplusplus
extern "C" {
#endif

struct tms_context;

extern const char *T_B_strings[]; /* backends.c */

int tbackend_init_surface(void);

int tbackend_is_tablet();

const char *tbackend_get_storage_path();
#if defined TMS_BACKEND_IOS
const char *tbackend_get_user_path();
#else
#define tbackend_get_user_path tbackend_get_storage_path
#endif
const char *tbackend_get_device_info(void);
int tbackend_is_shitty(void);
void tbackend_toggle_fullscreen(void);

#ifdef __cplusplus
}
#endif

#endif
