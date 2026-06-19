#pragma once

#if !defined TMS_BACKEND_PC \
 && !defined TMS_BACKEND_MOBILE
	#error Either TMS_BACKEND_PC or TMS_BACKEND_MOBILE need to be defined for your platform.
#endif

#include <tms/core/err.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "storage.h"

#ifdef __cplusplus
}
#endif
