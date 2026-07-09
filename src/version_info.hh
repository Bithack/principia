#pragma once

#ifdef DEBUG
	#define VER_EXTRA " [Debug]"
#else
	#define VER_EXTRA
#endif

#define PRINCIPIA_VERSION_CODE 42
#define PRINCIPIA_VERSION_STRING "2026.07.09" VER_EXTRA
