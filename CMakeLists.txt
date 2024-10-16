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

set(TMS_FORMFACTOR "PC")
if(WIN32)
	set(TMS_BACKEND "WINDOWS")
elseif(ANDROID)
	set(TMS_BACKEND "ANDROID")
	set(TMS_FORMFACTOR "MOBILE")
elseif(SCREENSHOT_BUILD)
	set(TMS_BACKEND "LINUX_SS")
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
	src/
	src/lua/
	src/SDL_image/
	src/src/
	${FREETYPE_INCLUDE_DIRS}
	${JPEG_INCLUDE_DIRS}
	${PNG_INCLUDE_DIRS}
	${ZLIB_INCLUDE_DIRS}
	${CMAKE_CURRENT_BINARY_DIR})

file(GLOB SRCS
	src/tms/core/*.c
	src/tms/math/*.c
	src/tms/util/*.c
	src/tms/bindings/cpp/cpp.cc
	src/tms/modules/3ds.c

	src/lua/*.c
	src/SDL_image/*.c

	src/src/*.cc
	src/src/*.c

	src/Box2D/Collision/*.cc
	src/Box2D/Collision/Shapes/*.cc
	src/Box2D/Common/*.cc
	src/Box2D/Dynamics/*.cc
	src/Box2D/Dynamics/Contacts/*.cc
	src/Box2D/Dynamics/Joints/*.cc
	src/Box2D/Particle/*.cc
)

# For non-Android, system SDL2 and OpenGL include dirs
include_directories(
	${SDL2_INCLUDE_DIRS}
	${OPENGL_INCLUDE_DIRS})

# Optional dependencies not found on Android or in the screenshot build
if(NOT SCREENSHOT_BUILD)
	if(NOT ANDROID AND NOT EMSCRIPTEN)
		find_package(PkgConfig REQUIRED)
		pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

		find_package(GLEW REQUIRED)

		include_directories(
			${GLEW_INCLUDE_DIRS}
			${GTK3_INCLUDE_DIRS})
	endif()

	find_package(CURL REQUIRED)

	file(GLOB SDL_mixer_SRCS src/SDL_mixer/*.c)
	set(SRCS ${SRCS} ${SDL_mixer_SRCS})

	include_directories(
		${CURL_INCLUDE_DIR}
		src/SDL_mixer/)

	add_definitions(-DBUILD_CURL)
endif()


# Optional luasocket functionality

if(NOT SCREENSHOT_BUILD AND NOT ANDROID AND NOT EMSCRIPTEN)
	option(USE_LUASOCKET "Build with Luasocket support" TRUE)
else()
	set(USE_LUASOCKET false)
endif()

if(USE_LUASOCKET)
	add_definitions(-DBUILD_LUASOCKET)

	file(GLOB LUASOCKET_SRCS
		src/luasocket/*.c)

	set(SRCS ${SRCS} ${LUASOCKET_SRCS})
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
	set(SRCS ${SRCS} src/tms/backend/pipe.cc)
endif()

set(SRCS ${SRCS} src/tms/backend/${BACKEND_SRC})


if(WIN32)
	# Windows manifest (resource and icon) for Windows builds
	set(WINRESOURCE_FILE "packaging/principia.rc")
	set(WINMANIFEST_FILE "packaging/principia.manifest")

	if(NOT CMAKE_RC_COMPILER)
		set(CMAKE_RC_COMPILER "windres.exe")
	endif()
	add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/principia.rc.o
		COMMAND ${CMAKE_RC_COMPILER} -I${CMAKE_CURRENT_SOURCE_DIR} -I${CMAKE_CURRENT_BINARY_DIR}
		-i${WINRESOURCE_FILE}
		-o ${CMAKE_CURRENT_BINARY_DIR}/principia.rc.o
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		DEPENDS ${WINRESOURCE_FILE} ${WINMANIFEST_FILE})
	set(SRCS ${SRCS} ${CMAKE_CURRENT_BINARY_DIR}/principia.rc.o)
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

target_link_libraries(
	${PROJECT_NAME}
		${FREETYPE_LIBRARIES}
		${JPEG_LIBRARIES}
		${PNG_LIBRARIES}
		${ZLIB_LIBRARIES})

if(NOT SCREENSHOT_BUILD)
	target_link_libraries(${PROJECT_NAME} ${CURL_LIBRARIES})

	if(NOT ANDROID)
		target_link_libraries(${PROJECT_NAME} ${GTK3_LIBRARIES})
	endif()

	if(NOT SHOULD_USE_GLES)
		target_link_libraries(${PROJECT_NAME} ${GLEW_LIBRARIES})
	endif()
endif()

if(NOT EMSCRIPTEN)
	target_link_libraries(${PROJECT_NAME} ${SDL2_LIBRARIES})
endif()

if(SHOULD_USE_GLES)
	target_link_libraries(${PROJECT_NAME} -lGLESv2)
else()
	target_link_libraries(${PROJECT_NAME} ${OPENGL_LIBRARIES})
endif()

add_dependencies(${PROJECT_NAME} GenerateGitVersion)

# Compiler flags
# --------------

if(SHOULD_USE_GLES)
	add_definitions(-DTMS_USE_GLES)
elseif(NOT SCREENSHOT_BUILD)
	add_definitions(-DTMS_USE_GLEW)
endif()

if(ANDROID)
	target_link_libraries(${PROJECT_NAME} -landroid -ldl -llog -lOpenSLES)

	add_definitions(-DHAVE_GCC_ATOMICS)
else()
	add_definitions(-DTMS_FAST_MATH)

	if(WIN32)
		target_link_libraries(${PROJECT_NAME} ws2_32.lib version.lib shlwapi.lib winmm.lib)

		add_definitions(-D_WIN32_WINNT=0x0501 -DUNICODE)
	elseif(SCREENSHOT_BUILD)
		add_definitions(-DNO_UI)
	endif()
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

# macOS Clang's linker doesn't like these flags
if(NOT APPLE)
	if(WIN32)
		set(CMAKE_EXE_LINKER_FLAGS_RELEASE "-mwindows -Wl,-s")
	else()
		set(CMAKE_EXE_LINKER_FLAGS_RELEASE "-Wl,-s")
	endif()
endif()


# Installation
# ------------

if(EMSCRIPTEN)
	set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "--preload-file ../data-pc/ --preload-file ../data-shared/")
	target_link_libraries(${PROJECT_NAME} -lemsocket -lssl -lcrypto)
endif()

if(APPLE)
	set(BUNDLE_NAME ${PROJECT_NAME}.app)
	set(BUNDLE_PATH "${BUNDLE_NAME}")

	set(BINDIR ${BUNDLE_NAME}/Contents/MacOS)
	set(SHAREDIR ${BUNDLE_NAME}/Contents/Resources)

	install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/data-pc" DESTINATION "${SHAREDIR}")
	install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/data-shared" DESTINATION "${SHAREDIR}")

	install(FILES "packaging/principia.icns" DESTINATION "${SHAREDIR}")
	install(FILES "packaging/Info.plist" DESTINATION "${BUNDLE_PATH}/Contents")

else()
	include(GNUInstallDirs)
	set(SHAREDIR "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}")
	set(BINDIR "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}")

	install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/data-pc" DESTINATION "${SHAREDIR}/principia")
	install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/data-shared" DESTINATION "${SHAREDIR}/principia")

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
