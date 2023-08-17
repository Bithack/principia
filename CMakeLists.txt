cmake_minimum_required(VERSION 3.9)

project(principia)

message(STATUS "*** Building Principia! ***")

if (NOT CMAKE_BUILD_TYPE)
    message(STATUS "No build type selected, default to Release")
    set(CMAKE_BUILD_TYPE "Release")
endif()

find_package(Freetype REQUIRED)
find_package(JPEG REQUIRED)
find_package(PNG REQUIRED)
find_package(SDL2 REQUIRED)
find_package(SDL2_image REQUIRED)
find_package(SDL2_mixer REQUIRED)
find_package(SDL2_ttf REQUIRED)
find_package(ZLIB REQUIRED)

set(OpenGL_GL_PREFERENCE GLVND)
find_package(OpenGL REQUIRED)

option(SCREENSHOT_BUILD "Build screenshotter build (Linux only)" FALSE)

if(WIN32)
	set(TMS_BACKEND "windows")
else()
	if(SCREENSHOT_BUILD)
		set(TMS_BACKEND "screenshot-linux")
	else()
		set(TMS_BACKEND "linux")
	endif()
endif()

include_directories(
	src/
	src/lua/
	src/SDL_image/
	src/src/
	${FREETYPE_INCLUDE_DIRS}
	${JPEG_INCLUDE_DIRS}
	${OPENGL_INCLUDE_DIRS}
	${PNG_INCLUDE_DIRS}
	${SDL2_INCLUDE_DIRS}
	${SDL2_image_INCLUDE_DIRS}
	${SDL2_mixer_INCLUDE_DIRS}
	${SDL2_ttf_INCLUDE_DIRS}
	${ZLIB_INCLUDE_DIRS})

file(GLOB SRCS
	src/tms/core/*.c
	src/tms/math/*.c
	src/tms/util/*.c
	src/tms/bindings/cpp/cpp.cc
	src/tms/modules/3ds/3ds.c

	src/lua/*.c

	src/SDL_image/SDL2_rotozoom.c

	src/src/*.cc
	src/src/*.c

	src/src/Box2D/Collision/*.cc
	src/src/Box2D/Collision/Shapes/*.cc
	src/src/Box2D/Common/*.cc
	src/src/Box2D/Dynamics/*.cc
	src/src/Box2D/Dynamics/Contacts/*.cc
	src/src/Box2D/Dynamics/Joints/*.cc
	src/src/Box2D/Particle/*.cc
)

if(NOT SCREENSHOT_BUILD)
	find_package(PkgConfig REQUIRED)
	pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
	find_package(CURL REQUIRED)
	find_package(GLEW REQUIRED)

	# Luasocket
	SET(LUASOCKET_FLAGS "-DBUILD_LUASOCKET -DLUASOCKET_INET_PTON")

	include_directories(
		src/luasocket/
		${CURL_INCLUDE_DIR}
		${GLEW_INCLUDE_DIRS}
		${GTK3_INCLUDE_DIRS})

	set(SRCS ${SRCS}
		src/luasocket/auxiliar.c
		src/luasocket/buffer.c
		src/luasocket/except.c
		src/luasocket/inet.c
		src/luasocket/io.c
		src/luasocket/luasocket.c
		src/luasocket/mime.c
		src/luasocket/options.c
		src/luasocket/select.c
		src/luasocket/tcp.c
		src/luasocket/timeout.c
		src/luasocket/udp.c
		src/luasocket/usocket.c
		src/luasocket/unix.c)
endif()

include_directories(src/tms/backends/${TMS_BACKEND}/)

set(SRCS ${SRCS}
	src/tms/backends/${TMS_BACKEND}/main.cc)

add_executable(${PROJECT_NAME} ${SRCS})
target_link_libraries(
	${PROJECT_NAME}
		${CURL_LIBRARIES}
		${FREETYPE_LIBRARIES}
		${JPEG_LIBRARIES}
		${OPENGL_LIBRARIES}
		${PNG_LIBRARIES}
		${SDL2_LIBRARIES}
		SDL2_image::SDL2_image
		SDL2_mixer::SDL2_mixer
		SDL2_ttf::SDL2_ttf
		${ZLIB_LIBRARIES})

if(NOT SCREENSHOT_BUILD)
	target_link_libraries(
		${PROJECT_NAME}
			${GLEW_LIBRARIES}
			${GTK3_LIBRARIES})
endif()

set(COMMON_FLAGS "${LUASOCKET_FLAGS} -DGL_GLEXT_PROTOTYPES -DTMS_BACKEND_PC -DTMS_FAST_MATH -DLUA_COMPAT_MODULE -D__STDC_FORMAT_MACROS=1")

if(WIN32)
	set(COMMON_FLAGS "${COMMON_FLAGS} -DTMS_BACKEND_WINDOWS")
else()
	if(SCREENSHOT_BUILD)
		set(COMMON_FLAGS "${COMMON_FLAGS} -DNO_UI -DTMS_BACKEND_LINUX_SS")
	else()
		set(COMMON_FLAGS "${COMMON_FLAGS} -DTMS_BACKEND_LINUX")
	endif()
endif()

set(COMMON_FLAGS_DEBUG "${COMMON_FLAGS} -O0 -ggdb -ffast-math -Werror=return-type -DDEBUG=1 -rdynamic")
set(COMMON_FLAGS_RELEASE "${COMMON_FLAGS} -DNDEBUG=1 -s -fomit-frame-pointer -fvisibility=hidden -fdata-sections -ffunction-sections")

set(CMAKE_C_FLAGS_RELEASE "${COMMON_FLAGS_RELEASE} -O1")
set(CMAKE_C_FLAGS_DEBUG "${COMMON_FLAGS_DEBUG}")

set(CMAKE_CXX_FLAGS_RELEASE "${COMMON_FLAGS_RELEASE} -O2 -fvisibility-inlines-hidden -fno-rtti")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")

set(CMAKE_EXE_LINKER_FLAGS_RELEASE "-Wl,-O,-s,--gc-sections")
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "-rdynamic")
