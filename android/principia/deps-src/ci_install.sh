#!/bin/bash -e
ndk_version=r27b

# Build tools and stuff
sudo apt-get update
sudo apt-get install -y wget unzip zip gcc-multilib make cmake

# NDK
wget --progress=bar:force "http://dl.google.com/android/repository/android-ndk-${ndk_version}-linux.zip"
unzip -q "android-ndk-${ndk_version}-linux.zip"
rm "android-ndk-${ndk_version}-linux.zip"

printf 'export ANDROID_NDK="%s"\nexport JOBS=4' "$PWD/android-ndk-${ndk_version}" >env.sh
