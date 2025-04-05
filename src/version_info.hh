#pragma once

#ifdef DEBUG
	#define VER_EXTRA " [Debug]"
#else
	#define VER_EXTRA
#endif

#define PRINCIPIA_VERSION_CODE 39
#define PRINCIPIA_VERSION_STRING "2025.04.05" VER_EXTRA
