#pragma once

#ifdef DEBUG
	#define VER_EXTRA " [Debug]"
#else
	#define VER_EXTRA
#endif

#define PRINCIPIA_VERSION_CODE 37
#define PRINCIPIA_VERSION_STRING "2024.07.12" VER_EXTRA
