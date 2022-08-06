#if defined(TMS_BACKEND_ANDROID) || defined(TMS_BACKEND_IOS)
#include "curl_setup_android32.h"
#elif defined(TMS_BACKEND_LINUX)
#include "curl_setup_linux64.h"
#else
#error curl does not have a setup for this TMS_BACKEND
#endif
