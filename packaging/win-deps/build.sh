#!/bin/bash -eu

# get the script's path
scrdir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

FETCHCACHE="$PWD/dl"
BUILDBASE="$PWD/build"
PACKAGEDEST="$PWD"

# parse command line
if [ "$2" == "64" ]; then
	MINGW_TYPE="win64"
	MINGW_PREFIX=x86_64-w64-mingw32
elif [ "$2" == "32" ]; then
	MINGW_TYPE="win32"
	MINGW_PREFIX=i686-w64-mingw32
elif [ "$2" == "arm64" ]; then
	MINGW_TYPE="winarm64"
	MINGW_PREFIX=aarch64-w64-mingw32
fi

CMAKE_FLAGS=(
	-Wno-author
	-DCMAKE_TOOLCHAIN_FILE="$scrdir/../toolchain-$MINGW_PREFIX.cmake"
	-DCMAKE_BUILD_TYPE=Release
	-DCMAKE_INSTALL_PREFIX=
	-GNinja
)

fetch_tarball () {
	local filename=${1##*/}
	# if filename already exists, skip all the downloading stuff
	if [ ! -f $FETCHCACHE/"$filename" ]; then
		local filedest=$(mktemp -p $FETCHCACHE -u)
		hash=$(wget -O- "$1" | tee >(sha256sum | cut -d " " -f 1) >$filedest)
		if [ "$hash" != "$2" ]; then
			echo "Hash mismatch for $filename" >&2
			echo "  expected: $2" >&2
			echo "  actual: $hash" >&2
			rm $filedest
			return 1
		else
			mv $filedest "$FETCHCACHE/$filename"
		fi
	fi

	local tarfile=$FETCHCACHE/$filename
	shift
	echo "Extracting: $(basename "$tarfile")" >&2
	tar -xa -f "$tarfile" -C "$SRCDIR" --strip-components=1
}

depend_get_path() {
	local p="$BUILDBASE/$1-$MINGW_TYPE/pkg"
	if [ ! -d $p ]; then
		local p2=$(echo "$PACKAGEDEST/$1-"[!-]*"-$MINGW_TYPE.zip")
		if [ ! -f "$p2" ]; then
			echo "The dependency $1 needs to be built first!" >&2
			kill $$ # there doesn't seem to be a good way to exit from here
		else
			echo "Note: The dependency $1 was requested and exists as a ZIP archive, unpacking it." >&2
			mkdir -p "$p"
			unzip -q "$p2" -d "$p" || kill $$
		fi
	fi
	printf '%s' "$p"
}

build() {
	local pkg="$1"
	local ver_var="${pkg}_ver"
	local ver="${!ver_var}"
	local hash_var="${pkg}_hash"
	local hash="${!hash_var}"

	mkdir -p "$FETCHCACHE" "$BUILDBASE"

	local builddir="$BUILDBASE/$pkg-$MINGW_TYPE"
	mkdir -p "$builddir"

	# set up directories
	SRCDIR="$builddir/src"
	INSTALL_DIR="$builddir/pkg"
	rm -rf "$INSTALL_DIR"
	mkdir -p "$SRCDIR" "$INSTALL_DIR"
	cd "$SRCDIR"

	echo "Building $pkg ver $ver..."

	# grab the package's ver with the variable "$1_ver"

	"build_$pkg" "$ver" "$hash"

	# clean up stuff
	pushd $INSTALL_DIR
	find . -name '*.la' -delete
	[ -d lib/pkgconfig ] && rm -r lib/pkgconfig
	[ -d share ] && rm -rf share/{info,man,doc,aclocal}
	find . -depth -type d -exec rmdir {} \; 2>/dev/null # empty directories
	popd

	# package into zip
	[[ -z "$ver" || "$ver" == *" "* ]] && { echo "Version invalid, cannot proceed." >&2; return 1; }
	local zipfile=$PACKAGEDEST/$pkg-$ver-$MINGW_TYPE.zip
	rm -f "$zipfile"

	[ -z "$(ls "$INSTALL_DIR")" ] && { echo "Package empty, cannot proceed." >&2; return 1; }
	pushd $INSTALL_DIR
	zip -9ry "$zipfile" -- *
	popd
}

# pkgs

source "$scrdir/_dep_vers.sh"

build_curl() {
	fetch_tarball "https://curl.se/download/curl-$1.tar.bz2" $2

	zlibpath=$(depend_get_path zlib)
	options=(
		-DBUILD_SHARED_LIBS=OFF
		-DBUILD_STATIC_LIBS=ON -DCURL_USE_SCHANNEL=ON
		-DBUILD_{CURL_EXE,LIBCURL_DOCS,MISC_DOCS}=OFF
		-DCURL_DISABLE_{ALTSVC,AWS,BASIC_AUTH,BEARER_AUTH,BINDLOCAL,DICT,DIGEST_AUTH,DOH,FTP,GOPHER,HSTS,IMAP,IPFS,KERBEROS_AUTH,LDAP,LDAPS,MQTT,NEGOTIATE_AUTH,PARSEDATE,POP3,RTSP,SMTP,SOCKETPAIR,SRP,TELNET,TFTP,WEBSOCKETS}=ON
		-DCURL_USE_LIBPSL=OFF
		-DCURL_ZLIB=ON -DZLIB_INCLUDE_DIR="$zlibpath/include" -DZLIB_LIBRARY="$zlibpath/lib/libz.dll.a"
		-DCURL_ZSTD=OFF
	)

	cmake . "${CMAKE_FLAGS[@]}" "${options[@]}"
	ninja

	DESTDIR=$INSTALL_DIR ninja install
	rm -f $INSTALL_DIR/bin/curl-config
}

build_freetype() {
	fetch_tarball "https://download.savannah.gnu.org/releases/freetype/freetype-$1.tar.gz" $2

	zlibpath=$(depend_get_path zlib)
	mkdir -p build; cd build
	cmake .. "${CMAKE_FLAGS[@]}" \
		-DCMAKE_INSTALL_PREFIX=/ \
		-DBUILD_SHARED_LIBS=OFF -DFT_REQUIRE_ZLIB=ON \
		-DZLIB_INCLUDE_DIR="$zlibpath/include" -DZLIB_LIBRARY="$zlibpath/lib/libz.a"
	ninja

	DESTDIR=$INSTALL_DIR ninja install
	mv $INSTALL_DIR/usr/* $INSTALL_DIR
}

build_libjpeg() {
	which nasm >/dev/null || which yasm >/dev/null || \
		{ echo >&2 "A standalone assembler is required for this build."; exit 1; }

	fetch_tarball "https://github.com/libjpeg-turbo/libjpeg-turbo/releases/download/$1/libjpeg-turbo-$1.tar.gz" $2

	cmake . "${CMAKE_FLAGS[@]}" \
		-DENABLE_SHARED=OFF -DENABLE_STATIC=ON \
		-DWITH_{TOOLS,TESTS,TURBOJPEG}=OFF
	ninja

	DESTDIR=$INSTALL_DIR ninja install
	rm -f $INSTALL_DIR/bin/*.exe
}

build_libpng() {
	fetch_tarball "https://downloads.sourceforge.net/sourceforge/libpng/libpng-$1.tar.xz" $2

	zlibpath=$(depend_get_path zlib)
	cmake . "${CMAKE_FLAGS[@]}" \
		-DPNG_STATIC=ON -DPNG_{SHARED,TESTS,TOOLS}=OFF \
		-DZLIB_INCLUDE_DIR=$zlibpath/include -DZLIB_LIBRARY=$zlibpath/lib/libz.a
	ninja

	DESTDIR=$INSTALL_DIR ninja install
	rm -f $INSTALL_DIR/bin/*-config
}

build_zlib() {
	fetch_tarball "https://zlib.net/fossils/zlib-$1.tar.gz" $2

	sed -i 's/set(zlib_static_suffix "s")/set(zlib_static_suffix "")/' CMakeLists.txt

	cmake . "${CMAKE_FLAGS[@]}" -DZLIB_BUILD_{SHARED,TESTING}=OFF
	ninja

	DESTDIR=$INSTALL_DIR ninja install
}

if [ "$1" == "--all" ]; then
	for pkg in zlib curl freetype libjpeg libpng; do
		build "$pkg" "$2"
	done
else
	"build" "$1"
fi
