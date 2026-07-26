
if(NINTENDO_SWITCH)
	set(DEP_SDL_VER "69c1e47162c0951f952ca420cf5eb8d51cce88f1")
	download_dep_tarball(
		"SDL-switch"
		"${DEP_SDL_VER}"
		"https://github.com/rollerozxa/SDL/archive/${DEP_SDL_VER}.tar.gz"
		"SKIP"
	)
else()
	set(DEP_SDL_VER "3.4.12")
	download_dep_tarball(
		"SDL"
		"${DEP_SDL_VER}"
		"https://github.com/libsdl-org/SDL/releases/download/release-${DEP_SDL_VER}/SDL3-${DEP_SDL_VER}.tar.gz"
		"f07b958a9ac5020fb7a44cadb957f658b2149c3c8abb4f63145fac9303249db7"
	)
endif()

set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)

set(DISABLED_FEATURES CAMERA GPU HAPTIC POWER RENDER SENSOR TESTS VULKAN)

foreach(feature ${DISABLED_FEATURES})
	set(SDL_${feature} OFF CACHE BOOL "" FORCE)
endforeach()

if(ANDROID OR HAIKU)
	enable_language(CXX)
endif()

if(HAIKU)
	add_definitions(-fPIC)
endif()

if(EMSCRIPTEN)
	set(SDL_PTHREADS ON CACHE BOOL "" FORCE)
	set(SDL_EMSCRIPTEN_PERSISTENT_PATH "/storage" CACHE STRING "" FORCE)
endif()

add_definitions(-DSDL_LEAN_AND_MEAN=1)

add_subdirectory(lib/SDL EXCLUDE_FROM_ALL)
