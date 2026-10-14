#!/bin/bash
set -eu

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
PREFIX="${ROOT}/build-ios-deps"
WORK="${ROOT}/build-ios-deps-src"

SDK="${IOS_SDK:-iphoneos}"
ARCH="${IOS_ARCH:-arm64}"
DEPLOYMENT_TARGET="${IOS_DEPLOYMENT_TARGET:-15.0}"

COMMON_CMAKE_ARGS=(
    -G Ninja
    -DCMAKE_SYSTEM_NAME=iOS
    -DCMAKE_SYSTEM_PROCESSOR="${ARCH}"
    -DCMAKE_OSX_SYSROOT="${SDK}"
    -DCMAKE_OSX_ARCHITECTURES="${ARCH}"
    -DCMAKE_OSX_DEPLOYMENT_TARGET="${DEPLOYMENT_TARGET}"
    -DCMAKE_BUILD_TYPE=Release
    -DCMAKE_INSTALL_PREFIX="${PREFIX}"
)

rm -rf "${PREFIX}" "${WORK}"
mkdir -p "${PREFIX}" "${WORK}"

cd "${WORK}"

build_dep() {
    local name="$1"
    local repo="$2"
    local tag="$3"
    shift 3

    echo "===== ${name} ====="

    git clone --depth 1 --branch "$tag" "$repo" "$name"

    cmake -S "$name" -B "${name}-build" "${COMMON_CMAKE_ARGS[@]}" "$@"
    cmake --build "${name}-build"
    cmake --install "${name}-build"
}

build_dep \
    zlib \
    https://github.com/madler/zlib.git \
    v1.3.2 \
    -DZLIB_BUILD_TESTING=OFF

build_dep \
    libpng \
    https://github.com/pnggroup/libpng.git \
    v1.6.58 \
    -DCMAKE_PREFIX_PATH="$PREFIX" \
    -DPNG_STATIC=ON -DPNG_{SHARED,TESTS,TOOLS}=OFF

build_dep \
    libjpeg-turbo \
    https://github.com/libjpeg-turbo/libjpeg-turbo.git \
    3.2.0 \
    -DENABLE_SHARED=OFF -DENABLE_STATIC=ON \
    -DWITH_{TURBOJPEG,TOOLS,TESTS}=OFF

build_dep \
    FreeType \
    https://github.com/freetype/freetype.git \
    VER-2-14-3 \
    -DCMAKE_PREFIX_PATH="$PREFIX" \
    -DFT_DISABLE_{BZIP2,BROTLI,HARFBUZZ}=ON

# TODO: secure transport was removed in curl 8.15, need to switch to mbedtls to update to latest curl.
build_dep \
    curl \
    https://github.com/curl/curl.git \
    curl-8_14_1 \
    -DCMAKE_PREFIX_PATH="$PREFIX" \
    -DBUILD_SHARED_LIBS=OFF -DBUILD_STATIC_LIBS=ON \
    -DBUILD_{CURL_EXE,LIBCURL_DOCS,MISC_DOCS}=OFF \
    -DCURL_DISABLE_{ALTSVC,AWS,BASIC_AUTH,BEARER_AUTH,BINDLOCAL,DICT,DIGEST_AUTH,DOH,FTP,GOPHER,HSTS,IMAP,IPFS,KERBEROS_AUTH,LDAP,LDAPS,MQTT,NEGOTIATE_AUTH,PARSEDATE,POP3,RTSP,SMTP,SOCKETPAIR,SRP,TELNET,TFTP,WEBSOCKETS}=ON \
    -DCURL_USE_{LIBPSL,LIBSSH2}=OFF \
    -DCURL_USE_SECTRANSP=ON \
    -DCURL_ZLIB=ON -DCURL_ZSTD=OFF
