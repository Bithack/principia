#!/bin/bash
set -eu

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"

SDK="${IOS_SDK:-iphoneos}"
ARCH="${IOS_ARCH:-arm64}"
DEPLOYMENT_TARGET="${IOS_DEPLOYMENT_TARGET:-15.0}"

build_deps() {
	./packaging/ios/build-ios-deps.sh
}

build_principia() {
	LIB="$PWD/build-ios-deps/lib"
	INCLUDE="$PWD/build-ios-deps/include"
	mkdir -p build-ios; cd build-ios
	cmake .. \
		-DCMAKE_SYSTEM_NAME=iOS \
		-DCMAKE_SYSTEM_PROCESSOR="${ARCH}" \
		-DCMAKE_OSX_SYSROOT="${SDK}" \
		-DCMAKE_OSX_ARCHITECTURES="${ARCH}" \
		-DCMAKE_OSX_DEPLOYMENT_TARGET="$DEPLOYMENT_TARGET" \
		-DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_C_FLAGS="-Wno-deprecated-declarations" \
		-DCMAKE_CXX_FLAGS="-Wno-deprecated-declarations" \
		-DUSE_VENDORED_SDL3=ON \
		-DCURL_INCLUDE_DIR="$INCLUDE" \
		-DCURL_LIBRARY="$LIB/libcurl.a" \
		-DFREETYPE_INCLUDE_DIR_freetype2="$INCLUDE/freetype2" \
		-DFREETYPE_INCLUDE_DIRS="$INCLUDE/freetype2" \
		-DFREETYPE_LIBRARY="$LIB/libfreetype.a" \
		-DJPEG_INCLUDE_DIR="$INCLUDE" \
		-DJPEG_LIBRARY_RELEASE="$LIB/libjpeg.a" \
		-DPNG_PNG_INCLUDE_DIR="$INCLUDE" \
		-DPNG_LIBRARY_RELEASE="$LIB/libpng16.a" \
		-DZLIB_INCLUDE_DIR="$INCLUDE" \
		-DZLIB_LIBRARY="$LIB/libz.a" \
		-G Ninja
	ninja
}

build_bundle() {
	cd "$ROOT/build-ios"
	APP="$PWD/principia.app"
	IOS="$ROOT/packaging/ios"
	ASSETS="$PWD/Assets.xcassets"
	ASSET_BUILD="$PWD/asset-build"
	PLIST="$APP/Info.plist"
	PARTIAL_PLIST="$PWD/assetcatalog-partial.plist"

	cp -r "$ROOT/data" "$APP/data"

	rm -rf "$ASSETS"
	mkdir -p "$ASSETS"

	cp -R "$IOS/AppIcon.appiconset" "$ASSETS/AppIcon.appiconset"

	rm -rf "$ASSET_BUILD"
	rm -rf "$PARTIAL_PLIST"

	mkdir -p "$ASSET_BUILD"

	xcrun actool \
		--compile "$ASSET_BUILD" \
		--platform iphoneos \
		--minimum-deployment-target 13.0 \
		--target-device iphone \
		--target-device ipad \
		--app-icon AppIcon \
		--output-partial-info-plist "$PARTIAL_PLIST" \
		"$ASSETS"

	cp "$ASSET_BUILD/Assets.car" "$APP/Assets.car"

	cp "$IOS/Info.plist" "$PLIST"

	rm -rf "$ASSETS"
}

build_ipa() {
	cd "$ROOT/build-ios"

	rm -rf package
	mkdir -p package/Payload
	ditto principia.app package/Payload/Principia.app
	cd package
	zip -qry "$ROOT/Principia.ipa" Payload
}

# if not SKIP_BUILD_DEPS is set, build the dependencies first
if [ -z "${SKIP_BUILD_DEPS:-}" ]; then
	build_deps
fi

build_principia
build_bundle
build_ipa
