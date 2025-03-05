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

add_custom_target(GenerateGitVersion
	COMMAND ${CMAKE_COMMAND}
	-D "GENERATE_VERSION_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
	-D "GENERATE_VERSION_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}"
	-P "${CMAKE_SOURCE_DIR}/cmake/Modules/GenerateGitVersion.cmake"
	WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")


# Find core dependencies
# ----------------------

if(ANDROID)
	cmake_minimum_required(VERSION 3.20)
	include(PrincipiaAndroidLibs)

else()
	if(NOT EMSCRIPTEN)
		find_package(SDL2 REQUIRED)
	else()
		include(PrincipiaEmscriptenLibs)
	endif()

	set(OpenGL_GL_PREFERENCE GLVND)
	find_package(OpenGL REQUIRED)
endif()

find_package(Freetype REQUIRED)
find_package(JPEG REQUIRED)
find_package(PNG REQUIRED)
find_package(ZLIB REQUIRED)


# Determine platform and backend
# ------------------------------

if(LINUX)
	option(SCREENSHOT_BUILD "Build screenshotter build (Linux only)" FALSE)
else()
	set(SCREENSHOT_BUILD FALSE)
endif()

option(BACKEND_IMGUI "Enable incomplete Dear Imgui dialog backend (Experimental)" FALSE)

set(TMS_FORMFACTOR "PC")
if(WIN32)
	set(TMS_BACKEND "WINDOWS")
elseif(ANDROID)
	set(TMS_BACKEND "ANDROID")
	set(TMS_FORMFACTOR "MOBILE")
elseif(HAIKU)
	set(TMS_BACKEND "HAIKU")
elseif(APPLE)
	set(TMS_BACKEND "MACOS")
elseif(EMSCRIPTEN)
	set(TMS_BACKEND "EMSCRIPTEN")
else()
	set(TMS_BACKEND "LINUX")
endif()

if(LINUX)
	option(USE_GLES "Use OpenGL ES on Linux (Experimental)" FALSE)
endif()

if(USE_GLES OR ANDROID OR EMSCRIPTEN)
	set(SHOULD_USE_GLES TRUE)
else()
	set(SHOULD_USE_GLES FALSE)
endif()


# Include dirs and main source files
# ----------------------------------

include_directories(
	lib/
	lib/imgui/
	lib/lua/
	lib/SDL_image/
	lib/SDL_mixer/
	src/
	${CMAKE_CURRENT_BINARY_DIR})

file(GLOB SRCS
	src/tms/core/*.c
	src/tms/math/*.c
	src/tms/util/*.c
	src/tms/bindings/cpp/cpp.cc
	src/tms/modules/3ds.c

	src/*.cc
	src/*.c

	lib/lua/*.c
	lib/SDL_image/*.c

	lib/Box2D/Collision/*.cc
	lib/Box2D/Collision/Shapes/*.cc
	lib/Box2D/Common/*.cc
	lib/Box2D/Dynamics/*.cc
	lib/Box2D/Dynamics/Contacts/*.cc
	lib/Box2D/Dynamics/Joints/*.cc
	lib/Box2D/Particle/*.cc
)

if(BACKEND_IMGUI)
	file(GLOB IMGUI_SRCS
		lib/imgui/*.cpp)
	list(APPEND SRCS ${IMGUI_SRCS})
endif()

# Optional dependencies not found on Android or in the screenshot build
if(NOT SCREENSHOT_BUILD)
	if(NOT ANDROID AND NOT EMSCRIPTEN)
		if(NOT BACKEND_IMGUI)
			find_package(PkgConfig REQUIRED)
			pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
		endif()

		find_package(GLEW REQUIRED)

		if(NOT BACKEND_IMGUI)
			include_directories(${GTK3_INCLUDE_DIRS})
		endif()
	endif()

	find_package(CURL REQUIRED)
	include_directories(${CURL_INCLUDE_DIR})

	file(GLOB SDL_mixer_SRCS lib/SDL_mixer/*.c)
	list(APPEND SRCS ${SDL_mixer_SRCS})

	add_definitions(-DBUILD_CURL)
endif()


# Optional luasocket functionality

if(NOT SCREENSHOT_BUILD AND NOT ANDROID AND NOT EMSCRIPTEN)
	option(USE_LUASOCKET "Build with Luasocket support" TRUE)
else()
	set(USE_LUASOCKET FALSE)
endif()

if(USE_LUASOCKET)
	add_definitions(-DBUILD_LUASOCKET)

	file(GLOB LUASOCKET_SRCS
		lib/luasocket/*.c)

	list(APPEND SRCS ${LUASOCKET_SRCS})
endif()


if(ANDROID)
	set(BACKEND_SRC main_android.cc)
elseif(SCREENSHOT_BUILD)
	set(BACKEND_SRC main_screenshotter.cc)
elseif(EMSCRIPTEN)
	set(CMAKE_EXECUTABLE_SUFFIX ".html")
	set(BACKEND_SRC main_emscripten.cc)
else()
	set(BACKEND_SRC main.cc)
	list(APPEND SRCS src/tms/backend/pipe.cc)
endif()

list(APPEND SRCS src/tms/backend/${BACKEND_SRC})


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

set(LIBS
    Freetype::Freetype
    JPEG::JPEG
    PNG::PNG
    ZLIB::ZLIB
)

if(NOT EMSCRIPTEN)
	if(WIN32)
		list(APPEND LIBS SDL2::SDL2main)
	endif()
	list(APPEND LIBS SDL2::SDL2)
endif()

if(NOT SCREENSHOT_BUILD)
	list(APPEND LIBS ${CURL_LIBRARIES})

	if(NOT ANDROID)
		list(APPEND LIBS ${GTK3_LIBRARIES})
	endif()

	if(NOT SHOULD_USE_GLES)
		list(APPEND LIBS GLEW::GLEW)
	endif()
endif()

if(SHOULD_USE_GLES)
	list(APPEND LIBS GLESv2)
else()
	list(APPEND LIBS OpenGL::GL)
endif()

if(ANDROID)
	list(APPEND LIBS android dl log OpenSLES)
elseif(WIN32)
	list(APPEND LIBS ws2_32.lib version.lib shlwapi.lib winmm.lib)
endif()

target_link_libraries(${PROJECT_NAME} ${LIBS})

add_dependencies(${PROJECT_NAME} GenerateGitVersion)

# Compiler flags
# --------------

if(SHOULD_USE_GLES)
	add_definitions(-DTMS_USE_GLES)
elseif(NOT SCREENSHOT_BUILD)
	add_definitions(-DTMS_USE_GLEW)
endif()

if(ANDROID)
	add_definitions(-DHAVE_GCC_ATOMICS)
else()
	add_definitions(-DTMS_FAST_MATH)

	if(WIN32)
		add_definitions(-D_WIN32_WINNT=0x0501 -DUNICODE)
	elseif(SCREENSHOT_BUILD)
		add_definitions(-DNO_UI -DSCREENSHOT_BUILD)
	endif()
endif()

if(BACKEND_IMGUI)
	add_definitions(-DPRINCIPIA_BACKEND_IMGUI -DIMGUI_DEFINE_MATH_OPERATORS)
endif()

add_definitions(-DTMS_BACKEND_${TMS_FORMFACTOR} -DTMS_BACKEND_${TMS_BACKEND})

# Use a safe subset of fast math flags
set(COMMON_FLAGS "-fno-math-errno -fno-trapping-math -fno-signed-zeros")

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

# Installation
# ------------

if(EMSCRIPTEN)
	set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "--preload-file ../data/")
	target_link_libraries(${PROJECT_NAME} -lemsocket -lssl -lcrypto)
endif()

if(APPLE)
	set(BUNDLE_NAME ${PROJECT_NAME}.app)
	set(BUNDLE_PATH "${BUNDLE_NAME}")

	set(BINDIR ${BUNDLE_NAME}/Contents/MacOS)
	set(SHAREDIR ${BUNDLE_NAME}/Contents/Resources)

	install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/data" DESTINATION "${SHAREDIR}")

	install(FILES "packaging/principia.icns" DESTINATION "${SHAREDIR}")
	install(FILES "packaging/Info.plist" DESTINATION "${BUNDLE_PATH}/Contents")

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
