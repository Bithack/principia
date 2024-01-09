cmake_minimum_required(VERSION 3.9)

project(principia)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

message(STATUS "*** Building Principia! ***")

if (NOT CMAKE_BUILD_TYPE)
    message(STATUS "No build type selected, default to Release")
    set(CMAKE_BUILD_TYPE "Release")
endif()

find_package(PkgConfig REQUIRED)
find_package(Freetype REQUIRED)
find_package(JPEG REQUIRED)
find_package(PNG REQUIRED)
find_package(SDL2 REQUIRED)
find_package(ZLIB REQUIRED)

pkg_check_modules(SDL2_image REQUIRED SDL2_image)
pkg_check_modules(SDL2_ttf REQUIRED SDL2_ttf)

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
	src/src/
	${FREETYPE_INCLUDE_DIRS}
	${JPEG_INCLUDE_DIRS}
	${OPENGL_INCLUDE_DIRS}
	${PNG_INCLUDE_DIRS}
	${SDL2_INCLUDE_DIRS}
	${SDL2_image_INCLUDE_DIRS}
	${SDL2_ttf_INCLUDE_DIRS}
	${ZLIB_INCLUDE_DIRS})

file(GLOB SRCS
	src/tms/core/*.c
	src/tms/math/*.c
	src/tms/util/*.c
	src/tms/bindings/cpp/cpp.cc
	src/tms/modules/3ds.c

	src/lua/*.c

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
	pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
	find_package(CURL REQUIRED)
	find_package(GLEW REQUIRED)

	pkg_check_modules(SDL2_mixer REQUIRED SDL2_mixer)

	include_directories(
		${CURL_INCLUDE_DIR}
		${GLEW_INCLUDE_DIRS}
		${GTK3_INCLUDE_DIRS}
		${SDL2_mixer_INCLUDE_DIRS})

	option(USE_LUASOCKET "Build with Luasocket support" TRUE)
else()
	set(USE_LUASOCKET false)
endif()

# Luasocket
if(USE_LUASOCKET)
	set(LUASOCKET_FLAGS "-DBUILD_LUASOCKET -DLUASOCKET_INET_PTON")

	if(WIN32)
		set(LUASOCKET_PLAT_SRC src/luasocket/wsocket.c)
	else()
		set(LUASOCKET_PLAT_SRC
			src/luasocket/usocket.c
			src/luasocket/unix.c)
	endif()
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
		${LUASOCKET_PLAT_SRC})
endif()

include_directories(src/tms/backends/${TMS_BACKEND}/)

set(SRCS ${SRCS}
	src/tms/backends/${TMS_BACKEND}/main.cc)

if(WIN32)
	set(WINRESOURCE_FILE "packaging/principia.rc")
	set(WINMANIFEST_FILE "packaging/principia.manifest")

	if(NOT CMAKE_RC_COMPILER)
		set(CMAKE_RC_COMPILER "windres.exe")
	endif()
	ADD_CUSTOM_COMMAND(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/winresource.o
		COMMAND ${CMAKE_RC_COMPILER} -I${CMAKE_CURRENT_SOURCE_DIR} -I${CMAKE_CURRENT_BINARY_DIR}
		-i${WINRESOURCE_FILE}
		-o ${CMAKE_CURRENT_BINARY_DIR}/winresource.o
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		DEPENDS ${WINRESOURCE_FILE} ${WINMANIFEST_FILE})
	SET(SRCS ${SRCS} ${CMAKE_CURRENT_BINARY_DIR}/winresource.o)
endif()

add_executable(${PROJECT_NAME} ${SRCS})
target_link_libraries(
	${PROJECT_NAME}
		${FREETYPE_LIBRARIES}
		${JPEG_LIBRARIES}
		${OPENGL_LIBRARIES}
		${PNG_LIBRARIES}
		${SDL2_LIBRARIES}
		${SDL2_image_LIBRARIES}
		${SDL2_ttf_LIBRARIES}
		${ZLIB_LIBRARIES})

if(NOT SCREENSHOT_BUILD)
	target_link_libraries(
		${PROJECT_NAME}
			${CURL_LIBRARIES}
			${GLEW_LIBRARIES}
			${GTK3_LIBRARIES}
			${SDL2_mixer_LIBRARIES})
endif()

set(COMMON_FLAGS "${LUASOCKET_FLAGS} -DGL_GLEXT_PROTOTYPES -DTMS_BACKEND_PC -DTMS_FAST_MATH -DLUA_COMPAT_MODULE -D__STDC_FORMAT_MACROS=1 -DUNICODE")

if(WIN32)
	target_link_libraries(${PROJECT_NAME} ws2_32.lib version.lib shlwapi.lib winmm.lib)

	set(COMMON_FLAGS "${COMMON_FLAGS} -DTMS_BACKEND_WINDOWS -D_WIN32_WINNT=0x0501 -Dsrandom=srand -Drandom=rand")
else()
	if(SCREENSHOT_BUILD)
		set(COMMON_FLAGS "${COMMON_FLAGS} -DNO_UI -DTMS_BACKEND_LINUX_SS")
	else()
		set(COMMON_FLAGS "${COMMON_FLAGS} -DTMS_BACKEND_LINUX")
	endif()
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	# Downgrade some errors to warnings when building with Clang
	set(COMMON_FLAGS "${COMMON_FLAGS} -Wno-error=incompatible-function-pointer-types -Wno-error=int-conversion")
endif()

set(COMMON_FLAGS_DEBUG "${COMMON_FLAGS} -O0 -ggdb -ffast-math -Werror=return-type -DDEBUG=1")
set(COMMON_FLAGS_RELEASE "${COMMON_FLAGS} -DNDEBUG=1 -fomit-frame-pointer -fvisibility=hidden -fdata-sections -ffunction-sections")

set(CMAKE_C_FLAGS_RELEASE "${COMMON_FLAGS_RELEASE} -O1")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELEASE} -g")
set(CMAKE_C_FLAGS_DEBUG "${COMMON_FLAGS_DEBUG}")

set(CMAKE_CXX_FLAGS_RELEASE "${COMMON_FLAGS_RELEASE} -O2 -fvisibility-inlines-hidden -fno-rtti")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELEASE} -g")
set(CMAKE_CXX_FLAGS_DEBUG "${COMMON_FLAGS_DEBUG} -O0")

set(CMAKE_EXE_LINKER_FLAGS_RELEASE "-Wl,-O,-s,--gc-sections")
set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "-Wl,-O,--gc-sections")
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "")

if(WIN32)
	set(CMAKE_EXE_LINKER_FLAGS_RELEASE "-mwindows")
endif()

# Installation

if(UNIX)
	include(GNUInstallDirs)
	set(SHAREDIR "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}")
	set(BINDIR "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}")
	set(APPS_DIR "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}/applications")
	set(ICONDIR "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}/icons")

	install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/data-pc" DESTINATION "${SHAREDIR}")
	install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/data-shared" DESTINATION "${SHAREDIR}")

	install(FILES "packaging/principia.desktop" DESTINATION "${APPS_DIR}")
	install(FILES "packaging/principia-url-handler.desktop" DESTINATION "${APPS_DIR}")

	install(FILES "packaging/principia.png" DESTINATION "${ICONDIR}/hicolor/128x128/apps")

	install(TARGETS ${PROJECT_NAME} DESTINATION ${BINDIR})
endif()
