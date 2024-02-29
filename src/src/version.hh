#pragma once

#ifdef DEBUG
	#define VER_EXTRA " [Debug]"
#else
	#define VER_EXTRA
#endif

#define PRINCIPIA_VERSION_CODE 35
#define PRINCIPIA_VERSION_STRING "2024.02.29" VER_EXTRA
