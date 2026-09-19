#!/bin/bash -eu

VERSION=$(date +'%Y.%m.%d')
BRANCH=${VERSION}
NIGHTLY_URL="https://nightly.link/Bithack/principia/workflows"

mkdir -p /tmp/principia_releases
cd /tmp/principia_releases

# Windows 64-bit installer
wget ${NIGHTLY_URL}/windows/${BRANCH}/principia_win64.exe.zip
unzip principia_win64.exe.zip
mv principia_win64.exe principia_${VERSION}_win64.exe

# Windows 64-bit portable
wget ${NIGHTLY_URL}/windows/${BRANCH}/principia_win64.7z.zip
unzip principia_win64.7z.zip
mv principia_win64.7z principia_${VERSION}_win64.7z

# Windows 32-bit installer
wget ${NIGHTLY_URL}/windows/${BRANCH}/principia_win32.exe.zip
unzip principia_win32.exe.zip
mv principia_win32.exe principia_${VERSION}_win32.exe

# Windows 32-bit portable
wget ${NIGHTLY_URL}/windows/${BRANCH}/principia_win32.7z.zip
unzip principia_win32.7z.zip
mv principia_win32.7z principia_${VERSION}_win32.7z

# Linux AppImage
wget ${NIGHTLY_URL}/linux/${BRANCH}/Principia-x86_64.AppImage.zip
unzip Principia-x86_64.AppImage.zip
mv Principia-x86_64.AppImage principia_${VERSION}_x86_64.AppImage

# Android APK
wget ${NIGHTLY_URL}/android/${BRANCH}/principia-release-unsigned.apk.zip
unzip principia-release-unsigned.apk.zip
apksigner sign --ks ~/key.jks --ks-pass pass:${_ANDROID_KEY} --key-pass pass:${_ANDROID_KEY} --out principia-release-signed.apk principia-release-unsigned.apk
mv principia-release-signed.apk principia_${VERSION}.apk
