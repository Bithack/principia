#pragma once

#ifdef DEBUG
	#define VER_EXTRA " [Debug]"
#else
	#define VER_EXTRA
#endif

#define PRINCIPIA_VERSION_CODE 34
#define PRINCIPIA_VERSION_STRING "1.5.2 Beta" VER_EXTRA
