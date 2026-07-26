cmake_minimum_required(VERSION 3.14)

project(principia)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

message(STATUS "*** Building Principia! ***")

if (NOT CMAKE_BUILD_TYPE)
    message(STATUS "No build type selected, default to Release")
    set(CMAKE_BUILD_TYPE "Release")
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

include(DownloadLib)

add_custom_target(GenerateGitVersion
	COMMAND ${CMAKE_COMMAND}
	-D "GENERATE_VERSION_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
	-D "GENERATE_VERSION_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}"
	-P "${CMAKE_SOURCE_DIR}/cmake/Modules/GenerateGitVersion.cmake"
	WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")

option(UNITY_BUILD "Experimental unity build" FALSE)
mark_as_advanced(UNITY_BUILD)

option(USE_VENDORED_SDL3 "Build with vendored SDL3 library" FALSE)

# Find core dependencies
# ----------------------

if(USE_VENDORED_SDL3)
	include(cmake/SDL.cmake)
else()
	find_package(SDL3 REQUIRED)
endif()

if(ANDROID)
	cmake_minimum_required(VERSION 3.20)
	include(PrincipiaAndroidLibs)
elseif(NOT NINTENDO_SWITCH)
	set(OpenGL_GL_PREFERENCE GLVND)
	find_package(OpenGL REQUIRED)
endif()

if(NOT EMSCRIPTEN)
	find_package(Freetype REQUIRED)
	find_package(JPEG REQUIRED)
	find_package(PNG REQUIRED)
	find_package(ZLIB REQUIRED)
endif()


# Determine platform and backend
# ------------------------------

if(LINUX)
	option(SCREENSHOT_BUILD "Build screenshotter build (Linux only)" FALSE)
else()
	set(SCREENSHOT_BUILD FALSE)
endif()

if(ANDROID OR SCREENSHOT_BUILD)
	set(BACKEND_IMGUI FALSE)
else()
	set(BACKEND_IMGUI TRUE)
endif()

if(LINUX)
	option(USE_GLES "Use OpenGL ES on Linux (Experimental)" FALSE)
endif()

if(USE_GLES OR ANDROID OR EMSCRIPTEN OR NINTENDO_SWITCH)
	set(SHOULD_USE_GLES TRUE)
else()
	set(SHOULD_USE_GLES FALSE)
endif()


# Include dirs and main source files
# ----------------------------------

include_directories(
	lib/
	lib/GLAD/include/
	lib/imgui/
	lib/lua/
	lib/SDL_image/
	lib/SDL_mixer/
	src/
	${CMAKE_CURRENT_BINARY_DIR})

file(GLOB SRCS CONFIGURE_DEPENDS
	lib/GLAD/src/gl.c
	lib/lua/*.c
	lib/SDL_image/*.c
)

if(UNITY_BUILD)
	list(APPEND SRCS
		src/tms/_unity_chunk.c
		src/_unity_chunk.cc
		src/game.cc

		lib/Box2D/_unity_chunk.cc)
else()
	file(GLOB MAIN_SRCS CONFIGURE_DEPENDS
		src/tms/core/*.c
		src/tms/math/*.c
		src/tms/cpp.cc
		src/tms/modules/3ds.c

		src/*.cc
		src/luascript/*.cc

		lib/Box2D/Collision/*.cc
		lib/Box2D/Collision/Shapes/*.cc
		lib/Box2D/Common/*.cc
		lib/Box2D/Dynamics/*.cc
		lib/Box2D/Dynamics/Contacts/*.cc
		lib/Box2D/Dynamics/Joints/*.cc
		lib/Box2D/Particle/*.cc)
	list(APPEND SRCS ${MAIN_SRCS})
endif()

if(BACKEND_IMGUI)
	if(UNITY_BUILD)
		list(APPEND SRCS lib/imgui/_unity_chunk.cc)
	else()
		file(GLOB IMGUI_SRCS CONFIGURE_DEPENDS
			lib/imgui/*.cpp
			lib/imgui/misc/freetype/*.cpp
			src/ui/*.cc)
		list(APPEND SRCS ${IMGUI_SRCS})
	endif()
endif()

# Optional dependencies not found in the screenshot build
if(NOT SCREENSHOT_BUILD)
	if(NOT EMSCRIPTEN)
		find_package(CURL REQUIRED)
		include_directories(${CURL_INCLUDE_DIR})
	endif()

	file(GLOB SDL_mixer_SRCS CONFIGURE_DEPENDS lib/SDL_mixer/*.c)
	list(APPEND SRCS ${SDL_mixer_SRCS})
endif()


if(NOT UNITY_BUILD)
	if(SCREENSHOT_BUILD)
		set(BACKEND_SRC main_screenshotter.cc)
	else()
		set(BACKEND_SRC main.cc)
		list(APPEND SRCS src/tms/backend/pipe.cc)
	endif()

	list(APPEND SRCS src/tms/backend/${BACKEND_SRC})
endif()

if(WIN32)
	list(APPEND SRCS packaging/principia.rc)
endif()


# Add executable (or library for Android)
# ---------------------------------------

if(ANDROID)
	add_library(${PROJECT_NAME} SHARED ${SRCS})
else()
	add_executable(${PROJECT_NAME} ${SRCS})
endif()


# Link libraries against executable
# ---------------------------------

if(NOT EMSCRIPTEN)
	set(LIBS
	    Freetype::Freetype
	    JPEG::JPEG
	    PNG::PNG
	    ZLIB::ZLIB)

	if(NOT SCREENSHOT_BUILD AND NOT EMSCRIPTEN)
		list(APPEND LIBS ${CURL_LIBRARIES})
	endif()
endif()

list(APPEND LIBS SDL3::SDL3)

if(SHOULD_USE_GLES)
	list(APPEND LIBS GLESv2)
else()
	list(APPEND LIBS OpenGL::GL)
endif()

if(ANDROID)
	list(APPEND LIBS android dl log OpenSLES)
endif()

target_link_libraries(${PROJECT_NAME} ${LIBS})

add_dependencies(${PROJECT_NAME} GenerateGitVersion)

# Compiler flags
# --------------

if(SHOULD_USE_GLES)
	add_definitions(-DTMS_USE_GLES)
endif()

if(UNITY_BUILD)
	add_definitions(-DUNITY_BUILD)
endif()

if(WIN32)
	add_definitions(-D_WIN32_WINNT=0x0501)
elseif(SCREENSHOT_BUILD)
	add_definitions(-DNO_UI -DSCREENSHOT_BUILD)
endif()

if(BACKEND_IMGUI)
	add_definitions(-DPRINCIPIA_BACKEND_IMGUI -DIMGUI_DEFINE_MATH_OPERATORS)

	if(SHOULD_USE_GLES)
		add_definitions(-DIMGUI_IMPL_OPENGL_ES2)
	endif()
endif()

# Use a safe subset of fast math flags
set(COMMON_FLAGS "-fno-math-errno -fno-trapping-math -fno-signed-zeros")

if(EMSCRIPTEN)
	set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "--preload-file ../data/")
	set(LIBRARY_FLAGS "-sUSE_FREETYPE=1 -sUSE_LIBJPEG=1 -sUSE_LIBPNG=1 -sUSE_ZLIB=1 -pthread")
	string(APPEND COMMON_FLAGS " ${LIBRARY_FLAGS}")
	set(CMAKE_EXE_LINKER_FLAGS " ${LIBRARY_FLAGS} -pthread -sPTHREAD_POOL_SIZE=4 -sINITIAL_MEMORY=2013265920 -sALLOW_MEMORY_GROWTH=1 -sTOTAL_STACK=16Mb -sFETCH=1 -sEXPORTED_RUNTIME_METHODS=ccall,cwrap")
endif()

set(COMMON_FLAGS_DEBUG "${COMMON_FLAGS} -O0 -ggdb -DDEBUG=1")
set(COMMON_FLAGS_RELEASE "${COMMON_FLAGS} -DNDEBUG=1 -fomit-frame-pointer")

set(CMAKE_C_FLAGS_RELEASE "${COMMON_FLAGS_RELEASE} -O1")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELEASE} -g")
set(CMAKE_C_FLAGS_DEBUG "${COMMON_FLAGS_DEBUG}")

set(CMAKE_CXX_FLAGS_RELEASE "${COMMON_FLAGS_RELEASE} -O2 -fno-rtti")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELEASE} -g")
set(CMAKE_CXX_FLAGS_DEBUG "${COMMON_FLAGS_DEBUG}")

if(NOT DEFINED CMAKE_EXE_LINKER_FLAGS_RELEASE)
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "")
endif()

if(WIN32)
	string(APPEND CMAKE_EXE_LINKER_FLAGS_RELEASE "-mwindows ")
endif()

# macOS Clang's linker doesn't like these flags
if(NOT APPLE)
	string(APPEND CMAKE_EXE_LINKER_FLAGS_RELEASE "-Wl,-s ")
endif()

# Register protocol handler util on Windows
# -----------------------------------------

if(WIN32)
	add_executable(register-protocol-handler
		packaging/register-protocol-handler/main.c
		packaging/register-protocol-handler/windows.rc)
endif()

# Installation
# ------------

if(APPLE)
	set(BUNDLE_NAME ${PROJECT_NAME}.app)
	set(BUNDLE_PATH "${BUNDLE_NAME}")

	set(BINDIR ${BUNDLE_NAME}/Contents/MacOS)
	set(SHAREDIR ${BUNDLE_NAME}/Contents/Resources)

	install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/data" DESTINATION "${SHAREDIR}")

	install(FILES "packaging/principia.icns" DESTINATION "${SHAREDIR}")
	install(FILES "packaging/Info.plist" DESTINATION "${BUNDLE_PATH}/Contents")

elseif(EMSCRIPTEN)

	set(BINDIR .)
	install(FILES "packaging/index.html" DESTINATION .)
	install(FILES ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.wasm DESTINATION .)
	install(FILES ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.data DESTINATION .)

elseif(NINTENDO_SWITCH)

	nx_create_nro(${PROJECT_NAME})

else()
	include(GNUInstallDirs)
	set(SHAREDIR "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}")
	set(BINDIR "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}")

	install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/data" DESTINATION "${SHAREDIR}/principia")

	install(FILES "packaging/principia.desktop" DESTINATION "${SHAREDIR}/applications")
	install(FILES "packaging/principia-url-handler.desktop" DESTINATION "${SHAREDIR}/applications")

	install(FILES "packaging/principia.png" DESTINATION "${SHAREDIR}/icons/hicolor/128x128/apps")

	install(FILES "packaging/se.principia_web.principia.metainfo.xml" DESTINATION "${SHAREDIR}/metainfo")
endif()

install(TARGETS ${PROJECT_NAME}
	RUNTIME DESTINATION ${BINDIR}
	LIBRARY DESTINATION ${BINDIR}
	ARCHIVE DESTINATION ${BINDIR}
	BUNDLE DESTINATION .
)

if(APPLE)
	set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY 0)
	set(CPACK_GENERATOR ZIP)
	include(CPack)

	install(CODE "
		set(BU_CHMOD_BUNDLE_ITEMS ON)
		include(BundleUtilities)
		fixup_bundle(\"\${CMAKE_INSTALL_PREFIX}/${BUNDLE_PATH}\" \"\" \"\${CMAKE_INSTALL_PREFIX}/${BINDIR}\")
	" COMPONENT Runtime)
endif()
