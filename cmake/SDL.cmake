
set(DEP_SDL_VER "3.4.10")
download_dep_tarball(
	"SDL"
	"${DEP_SDL_VER}"
	"https://github.com/libsdl-org/SDL/releases/download/release-${DEP_SDL_VER}/SDL3-${DEP_SDL_VER}.tar.gz"
	"12b34280415ec8418c864408b93d008a20a6530687ee613d60bfbd20411f2785"
)
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
endif()

add_definitions(-DSDL_LEAN_AND_MEAN=1)

add_subdirectory(lib/SDL EXCLUDE_FROM_ALL)
