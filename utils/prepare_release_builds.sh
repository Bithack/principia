#!/bin/bash -eu

VERSION=$(date +'%Y.%m.%d')
BRANCH=${VERSION}
NIGHTLY_URL="https://nightly.link/Bithack/principia/workflows"

mkdir -p /tmp/principia_releases
cd /tmp/principia_releases

# Windows installer
wget ${NIGHTLY_URL}/windows/${BRANCH}/principia-setup.exe.zip
unzip principia-setup.exe.zip
mv principia-setup.exe principia_${VERSION}_win64.exe

# Windows portable
wget ${NIGHTLY_URL}/windows/${BRANCH}/principia-portable.7z.zip
unzip principia-portable.7z.zip
mv principia-portable.7z principia_${VERSION}_win64.7z

# Linux AppImage
wget ${NIGHTLY_URL}/linux/${BRANCH}/Principia-x86_64.AppImage.zip
unzip Principia-x86_64.AppImage.zip
mv Principia-x86_64.AppImage principia_${VERSION}_x86_64.AppImage

# Android APK
wget ${NIGHTLY_URL}/android/${BRANCH}/principia-release-unsigned.apk.zip
unzip principia-release-unsigned.apk.zip
apksigner sign --ks ~/key.jks --ks-pass pass:${_ANDROID_KEY} --key-pass pass:${_ANDROID_KEY} --out principia-release-signed.apk principia-release-unsigned.apk
mv principia-release-signed.apk principia_${VERSION}.apk
