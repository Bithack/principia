#if defined(TMS_BACKEND_ANDROID) || defined(TMS_BACKEND_IOS)
#include "curl_config_android32.h"
#elif defined(TMS_BACKEND_LINUX)
#include "curl_config_linux64.h"
#else
#error curl does not have a config for this TMS_BACKEND
#endif
