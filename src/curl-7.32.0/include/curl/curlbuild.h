#if defined(TMS_BACKEND_ANDROID) || defined(TMS_BACKEND_IOS)
#include "curlbuild_android32.h"
#elif defined(TMS_BACKEND_LINUX)
#ifdef __i386__
#include "curlbuild_linux32.h"
#else
#include "curlbuild_linux64.h"
#endif
#else
#error curl does not have a curlbuild.h for this TMS_BACKEND
#endif
