set(DEPS "${CMAKE_SOURCE_DIR}/build_web/deps/")

set(CURL_INCLUDE_DIR ${DEPS}/include/curl)
set(CURL_LIBRARY ${DEPS}/lib/libcurl.a)
set(FREETYPE_INCLUDE_DIR_ft2build ${DEPS}/include/freetype2)
set(FREETYPE_INCLUDE_DIR_freetype2 ${FREETYPE_INCLUDE_DIR_ft2build}/freetype)
set(FREETYPE_LIBRARY ${DEPS}/lib/libfreetype.a)
set(JPEG_INCLUDE_DIR ${DEPS}/include)
set(JPEG_LIBRARY ${DEPS}/lib/libjpeg.a)
set(PNG_PNG_INCLUDE_DIR ${DEPS}/include/libpng16) #what
set(PNG_LIBRARY ${DEPS}/lib/libpng16.a)
