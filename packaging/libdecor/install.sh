#!/bin/bash -eu

cd "$(dirname "$0")"

mkdir -p /usr/include/libdecor-0
cp libdecor.h /usr/include/libdecor-0/

wget https://files.voxelmanip.se/principia/libdecor-0.so.0.200.2
chmod +x libdecor-0.so.0.200.2

mkdir -p /usr/lib/x86_64-linux-gnu
cp libdecor-0.so.0.200.2 /usr/lib/x86_64-linux-gnu
ln -s libdecor-0.so.0.200.2 /usr/lib/x86_64-linux-gnu/libdecor-0.so.0
ln -s libdecor-0.so.0.200.2 /usr/lib/x86_64-linux-gnu/libdecor-0.so

mkdir -p /usr/lib/x86_64-linux-gnu/pkgconfig
cp libdecor-0.pc /usr/lib/x86_64-linux-gnu/pkgconfig
