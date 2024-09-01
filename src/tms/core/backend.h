#ifndef _BACKEND__H_
#define _BACKEND__H_

#if !defined TMS_BACKEND_PC \
 && !defined TMS_BACKEND_MOBILE
	#error Either TMS_BACKEND_PC or TMS_BACKEND_MOBILE need to be defined for your platform.
#endif

#include <tms/core/err.h>

#ifdef __cplusplus
extern "C" {
#endif

int tbackend_init_surface(void);

const char *tbackend_get_storage_path();

#ifdef __cplusplus
}
#endif

#endif
