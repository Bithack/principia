set(DEPS "${CMAKE_SOURCE_DIR}/android/deps/${ANDROID_ABI}")

set(CURL_INCLUDE_DIR ${DEPS}/curl/include)
set(CURL_LIBRARY ${DEPS}/curl/libcurl.a;${DEPS}/curl/libmbedcrypto.a;${DEPS}/curl/libmbedtls.a;${DEPS}/curl/libmbedx509.a)
set(FREETYPE_INCLUDE_DIR_ft2build ${DEPS}/freetype/include/freetype2)
set(FREETYPE_INCLUDE_DIR_freetype2 ${FREETYPE_INCLUDE_DIR_ft2build}/freetype)
set(FREETYPE_LIBRARY ${DEPS}/freetype/libfreetype.a)
set(JPEG_INCLUDE_DIR ${DEPS}/libjpeg/include)
set(JPEG_LIBRARY ${DEPS}/libjpeg/libjpeg.a)
set(PNG_PNG_INCLUDE_DIR ${DEPS}/libpng/include) #what
set(PNG_LIBRARY ${DEPS}/libpng/libpng.a)

add_library(SDL2::SDL2 STATIC IMPORTED)

set_target_properties(SDL2::SDL2 PROPERTIES
	INTERFACE_INCLUDE_DIRECTORIES ${DEPS}/SDL2/include/SDL2
	IMPORTED_LOCATION ${DEPS}/SDL2/libSDL2.a)
