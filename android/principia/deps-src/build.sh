#!/bin/bash
set -e

cd "$(dirname "$0")"

source _dep_vers.sh

# Some helpers for use by the scripts
get_tar_archive () {
	# $1: source directory name, relative to $srcdir
	# $2: URL
	local dest="$srcdir/$1"
	local url="$2"
	local filename="${url##*/}"

	[ -d "$dest" ] && return 0

	wget -c "$url" -O "$srcdir/$filename"

	echo "$3 $srcdir/$filename" | sha256sum -c

	mkdir -p "$dest"
	tar -xaf "$srcdir/$filename" -C "$dest" --strip-components=1
}

make_install_copy () {
	make DESTDIR=$PWD install
	mv usr/local/lib/*.a $pkgdir/
	if [ -d $pkgdir/include ]; then
		cp -a usr/local/include $pkgdir/
	else
		mv usr/local/include $pkgdir/
	fi
}

# Actual code used here
_setup_toolchain () {
	local toolchain=$(echo "$ANDROID_NDK"/toolchains/llvm/prebuilt/*)
	if [ ! -d "$toolchain" ]; then
		echo "Android NDK path not specified or incorrect"; return 1
	fi
	echo "Using NDK at: $ANDROID_NDK"
	echo "Toolchain: $toolchain"
	export PATH="$toolchain/bin:$ANDROID_NDK:$PATH"

	unset CFLAGS CPPFLAGS CXXFLAGS

	TARGET_ABI="$1"
	API=21
	if [ "$TARGET_ABI" == armeabi-v7a ]; then
		CROSS_PREFIX=armv7a-linux-androideabi
		CFLAGS="-mthumb"
		CXXFLAGS="-mthumb"
	elif [ "$TARGET_ABI" == arm64-v8a ]; then
		CROSS_PREFIX=aarch64-linux-android
	elif [ "$TARGET_ABI" == x86 ]; then
		CROSS_PREFIX=i686-linux-android
		CFLAGS="-mssse3 -mfpmath=sse"
		CXXFLAGS="-mssse3 -mfpmath=sse"
	elif [ "$TARGET_ABI" == x86_64 ]; then
		CROSS_PREFIX=x86_64-linux-android
	else
		echo "Invalid ABI given"; return 1
	fi
	export CC=$CROSS_PREFIX$API-clang
	export CXX=$CROSS_PREFIX$API-clang++
	export AR=llvm-ar
	export RANLIB=llvm-ranlib
	export CFLAGS="-fPIC ${CFLAGS}"
	export CXXFLAGS="-fPIC ${CXXFLAGS}"

	CMAKE_FLAGS=(
		"-Wno-author"
		"-DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake"
		"-DANDROID_ABI=$TARGET_ABI" "-DANDROID_NATIVE_API_LEVEL=$API"
		"-DCMAKE_BUILD_TYPE=Release"
		"-DCMAKE_INSTALL_PREFIX=/usr/local/"
		"-DCMAKE_C_FLAGS_RELEASE=-g0"
	)

	# make sure pkg-config doesn't interfere
	export PKG_CONFIG=/bin/false

	export MAKEFLAGS="-j$(nproc)"
}

_run_build () {
	builddir=$PWD/build/$1-$2
	pkgdir=$PWD/deps/$2
	srcdir=$PWD/src

	mkdir -p "$builddir" "$pkgdir" "$srcdir"

	pushd "$builddir"
	build_$1
	popd
}

build_curl() {
	get_tar_archive mbedtls "https://github.com/Mbed-TLS/mbedtls/releases/download/mbedtls-${mbedtls_ver}/mbedtls-${mbedtls_ver}.tar.bz2" $mbedtls_hash
	get_tar_archive curl "https://curl.se/download/curl-${curl_ver}.tar.bz2" $curl_hash

	# Build mbedtls first
	mkdir -p mbedtls
	local mbedtls=$PWD/mbedtls
	pushd $srcdir/mbedtls
	make -s clean # necessary
	make lib
	make DESTDIR=$mbedtls install
	popd

	# if you need to debug curl on android, remove the disabling of verbose strings.
	echo $mbedtls
	cmake "${CMAKE_FLAGS[@]}" \
		-DBUILD_SHARED_LIBS=OFF -DBUILD_STATIC_LIBS=ON \
		-DCURL_DISABLE_VERBOSE_STRINGS=ON \
		-DBUILD_{CURL_EXE,EXAMPLES,LIBCURL_DOCS,MISC_DOCS,TESTING}=OFF \
		-DCURL_DISABLE_{ALTSVC,AWS,BASIC_AUTH,BEARER_AUTH,BINDLOCAL,DICT,DIGEST_AUTH,DOH,FTP,GOPHER,HSTS,IMAP,IPFS,KERBEROS_AUTH,LDAP,LDAPS,MQTT,NEGOTIATE_AUTH,NETRC,PARSEDATE,POP3,PROGRESS_METER,RTSP,SMTP,SOCKETPAIR,SRP,TELNET,TFTP,WEBSOCKETS}=ON \
		-DENABLE_{CURL_MANUAL,UNIX_SOCKETS}=OFF \
		-DCURL_USE_MBEDTLS=ON \
		-DMBEDTLS_INCLUDE_DIR="$mbedtls/include" \
		-DMBEDTLS_LIBRARY="$mbedtls/lib/libmbedtls.a" \
		-DMBEDX509_LIBRARY="$mbedtls/lib/libmbedx509.a" \
		-DMBEDCRYPTO_LIBRARY="$mbedtls/lib/libmbedcrypto.a" \
		-DCURL_USE_LIBPSL=OFF \
		$srcdir/curl
	make
	make_install_copy

	# For mbedtls install only the libraries
	cp $mbedtls/lib/*.a $pkgdir/
}

build_freetype() {
	get_tar_archive freetype "https://download.savannah.gnu.org/releases/freetype/freetype-${freetype_ver}.tar.gz" $freetype_hash

	cmake "${CMAKE_FLAGS[@]}" \
		-DBUILD_SHARED_LIBS=OFF -DFT_REQUIRE_ZLIB=ON \
		$srcdir/freetype
	make

	make_install_copy
}

build_libjpeg() {
	get_tar_archive libjpeg "https://github.com/libjpeg-turbo/libjpeg-turbo/releases/download/${libjpeg_ver}/libjpeg-turbo-${libjpeg_ver}.tar.gz" $libjpeg_hash

	cmake "${CMAKE_FLAGS[@]}" \
		-DENABLE_SHARED=OFF -DENABLE_STATIC=ON \
		-DWITH_{TOOLS,TESTS,TURBOJPEG}=OFF \
		$srcdir/libjpeg
	make
	make_install_copy
}

build_libpng() {
	get_tar_archive libpng "https://download.sourceforge.net/libpng/libpng-${libpng_ver}.tar.gz" $libpng_hash

	cmake "${CMAKE_FLAGS[@]}" \
		-DPNG_STATIC=ON -DPNG_{SHARED,TESTS,TOOLS}=OFF \
		$srcdir/libpng
	make
	make_install_copy

	rm "$pkgdir/libpng.a"
	mv "$pkgdir/libpng16.a" "$pkgdir/libpng.a"
}

if [ $# -lt 2 ]; then
	echo "Usage: build.sh <target> <ABI>"
	exit 1
fi

_setup_toolchain "$2"

if [ "$1" == "--all" ]; then
	_run_build curl "$2"
	_run_build freetype "$2"
	_run_build libjpeg "$2"
	_run_build libpng "$2"
	echo "Full build for ABI $2 successful."
else
	_run_build "$1" "$2"
	echo "Build of $1 for ABI $2 successful."
fi

exit 0
