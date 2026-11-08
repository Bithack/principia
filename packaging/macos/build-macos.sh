#!/bin/bash
set -eu

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
PREFIX="${ROOT}/build-macos-deps"
WORK="${ROOT}/build-macos-deps-src"

# todo: make builds for intel macs too
COMMON_CMAKE_ARGS=(
    -G Ninja
    "-DCMAKE_OSX_ARCHITECTURES=arm64"
    -DCMAKE_OSX_DEPLOYMENT_TARGET=11
    -DCMAKE_BUILD_TYPE=Release
)

COMMON_CMAKE_DEP_ARGS=(
    "${COMMON_CMAKE_ARGS[@]}"
    -DCMAKE_INSTALL_PREFIX="${PREFIX}"
)

rm -rf "${PREFIX}" "${WORK}"
mkdir -p "${PREFIX}" "${WORK}"

build_dep() {
    local name="$1"
    local repo="$2"
    local tag="$3"
    shift 3

    echo "===== ${name} ====="

    git clone --depth 1 --branch "$tag" "$repo" "$name"

    cmake -S "$name" -B "${name}-build" "${COMMON_CMAKE_DEP_ARGS[@]}" "$@"
    cmake --build "${name}-build"
    cmake --install "${name}-build"
}

build_deps() {
    pushd "${WORK}"

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
        freetype \
        https://github.com/freetype/freetype.git \
        VER-2-14-3 \
        -DCMAKE_PREFIX_PATH="$PREFIX" \
        -DFT_DISABLE_{BZIP2,BROTLI,HARFBUZZ}=ON

    popd
}

build_principia() {
    pushd "$ROOT"

    LIB="$PREFIX/lib"
    INCLUDE="$PREFIX/include"
    mkdir -p build-macos; cd build-macos
    cmake .. \
        "${COMMON_CMAKE_ARGS[@]}" \
        -DUSE_VENDORED_SDL3=ON \
        -DUNITY_BUILD=ON \
        -DCMAKE_INSTALL_PREFIX=../build/macos/ \
        -DFREETYPE_INCLUDE_DIR_freetype2="$INCLUDE/freetype2" \
        -DFREETYPE_INCLUDE_DIRS="$INCLUDE/freetype2" \
        -DFREETYPE_LIBRARY="$LIB/libfreetype.a" \
        -DJPEG_INCLUDE_DIR="$INCLUDE" \
        -DJPEG_LIBRARY_RELEASE="$LIB/libjpeg.a" \
        -DPNG_PNG_INCLUDE_DIR="$INCLUDE" \
        -DPNG_LIBRARY_RELEASE="$LIB/libpng16.a" \
        -G Ninja
    ninja

    ninja package

    popd
}

build_deps
build_principia
