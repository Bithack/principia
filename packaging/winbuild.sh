#!/bin/bash
set -e

#if no arch is specified, error out
if [ -z "$1" ]; then
	echo "Usage: $0 <arch>"
	echo "arch: 32, 64, arm64"
	exit 1
fi

ARCH=$1

# set the toolchain file based on the architecture
if [ "$ARCH" == "32" ]; then
	TOOLCHAIN_FILE="../packaging/toolchain-i686-w64-mingw32.cmake"
elif [ "$ARCH" == "64" ]; then
	TOOLCHAIN_FILE="../packaging/toolchain-x86_64-w64-mingw32.cmake"
elif [ "$ARCH" == "arm64" ]; then
	TOOLCHAIN_FILE="../packaging/toolchain-aarch64-w64-mingw32.cmake"
else
	echo "Unknown architecture: $ARCH"
	exit 1
fi

# cd to script dir
cd "$(dirname "$0")"

# grab version information from ../packaging/version_info.txt
VERSION=$(sed -n '3p' version_info.txt)
VERSION_MAJOR=$(echo "$VERSION" | cut -d. -f1)
VERSION_MINOR=$(echo "$VERSION" | cut -d. -f2)
VERSION_BUILD=$(echo "$VERSION" | cut -d. -f3)

cd .. # go to repo root

SRCDIR="$(pwd)"
DEPS_DIR="$SRCDIR/packaging/win-deps"

source "$DEPS_DIR/_dep_vers.sh"

build_deps() {
	echo Building dependencies from source...

	pushd $DEPS_DIR >/dev/null
	./build.sh --all $ARCH
	popd >/dev/null
}

_dl_dep() {
	pushd $DEPS_DIR >/dev/null
	local ver_var="${1}_ver"
	local ver="${!ver_var}"
	local file=$1-$ver-win$ARCH.zip

	#if file already exists, skip downloading
	if [ -f "$file" ]; then
		echo "$file already exists, skipping download"
		popd >/dev/null
		return 0
	fi

	wget https://dl.principia-web.se/deps/win/$file -O $file
	local expected_hash_var="${1}_cache_hash_${ARCH}"
	local expected_hash="${!expected_hash_var}"
	echo "$expected_hash  $file" | sha256sum -c -
	popd >/dev/null
}

download_cached_deps() {
	echo Downloading cached deps...

	_dl_dep curl
	_dl_dep freetype
	_dl_dep libjpeg
	_dl_dep libpng
	_dl_dep zlib
}

prepare_deps() {
	echo Preparing dependencies...

	mkdir -p build_win$ARCH; cd build_win$ARCH

	rm -rf deps
	for zip in ../packaging/win-deps/*-win$ARCH.zip; do
		unzip -o "$zip" -d deps
	done
}

build_principia() {
	echo Building Principia...

	LIB="$PWD/deps/lib"
	INCLUDE="$PWD/deps/include"
	cmake .. \
		-DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN_FILE" \
		-DCMAKE_BUILD_TYPE=Release \
		-DUNITY_BUILD=ON \
		-DUSE_VENDORED_SDL3=ON \
		-DCURL_INCLUDE_DIR="$INCLUDE" \
		-DCURL_LIBRARY="$LIB/libcurl.a;$LIB/libz.a;ws2_32.lib;crypt32.lib;bcrypt.lib;secur32.lib;iphlpapi.lib" \
		-DFREETYPE_INCLUDE_DIR_freetype2="$INCLUDE/freetype2/freetype" \
		-DFREETYPE_INCLUDE_DIRS="$INCLUDE/freetype2" \
		-DFREETYPE_LIBRARY="$LIB/libfreetype.a" \
		-DJPEG_INCLUDE_DIR="$INCLUDE" \
		-DJPEG_LIBRARY_RELEASE="$LIB/libjpeg.a" \
		-DPNG_PNG_INCLUDE_DIR="$INCLUDE" \
		-DPNG_LIBRARY_RELEASE="$LIB/libpng16.a" \
		-DZLIB_INCLUDE_DIR="$INCLUDE" \
		-DZLIB_LIBRARY_RELEASE="$LIB/libz.a" \
		-DCMAKE_C_FLAGS="-ffunction-sections -fdata-sections" \
		-DCMAKE_CXX_FLAGS="-ffunction-sections -fdata-sections -DCURL_STATICLIB" \
		-DCMAKE_EXE_LINKER_FLAGS="-Wl,--gc-sections -static" \
		-G Ninja
	ninja
}

download_nsis() {
	NSIS_VER=3.12
	NSIS_HASH="56581f90db321581c5381193d796fffcf2d24b2f8fed2160a6c6a3baa67f2c4f"

	echo Downloading NSIS...

	# if nsis/ directory does not exist, download NSIS and extract it there
	if [ ! -d "nsis" ]; then
		echo "Downloading NSIS..."
		curl -L -o /tmp/nsis-$NSIS_VER.zip http://downloads.sourceforge.net/project/nsis/NSIS%203/$NSIS_VER/nsis-$NSIS_VER.zip

		echo "$NSIS_HASH  /tmp/nsis-$NSIS_VER.zip" | sha256sum -c -
		if [ $? -ne 0 ]; then
			echo "NSIS hash mismatch"
			exit 1
		fi

		echo "Extracting NSIS..."
		unzip /tmp/nsis-$NSIS_VER.zip -d /tmp/nsis-$NSIS_VER
		mv /tmp/nsis-$NSIS_VER/nsis-$NSIS_VER nsis
	fi
}

make_installer() {
	download_nsis

	echo Making installer...

	cp ../packaging/principia_install.nsi .
	wine nsis/makensis.exe \
		/DARCH_NAME=win$ARCH \
		/DVER_MAJOR=$VERSION_MAJOR \
		/DVER_MINOR=$VERSION_MINOR \
		/DVER_BUILD=$VERSION_BUILD \
		/DVERSION=$VERSION \
		principia_install.nsi
}

make_portable() {
	echo Making portable version...

	mkdir -p portable
	cd portable
	mkdir -p Principia
	cp ../principia.exe Principia/
	cp ../register-protocol-handler.exe Principia/
	cp -r ../../data Principia/
	touch Principia/portable.txt

	7z a ../principia_win${ARCH}.7z Principia/

	cd ..
}

if [ "$FORCE_BUILD_DEPS" == "1" ]; then
	build_deps
else
	download_cached_deps
fi
prepare_deps
build_principia
make_installer
make_portable
