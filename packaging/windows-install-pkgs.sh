#!/bin/bash

# Download and install custom built packages with less dependencies than MSYS2 counterparts

REPO=https://github.com/principia-game/windows-deps

wget ${REPO}/releases/download/latest/mingw-w64-clang-x86_64-curl-winssl-8.16.0-1-any.pkg.tar.zst
wget ${REPO}/releases/download/latest/mingw-w64-clang-x86_64-freetype-2.14.1-1-any.pkg.tar.zst

pacman -U --noconfirm *.pkg.tar.zst
