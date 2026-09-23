#!/bin/bash -eu

# Manually install libdecor, for satisfying the build-time dependency of libdecor
# in the Linux CI build. Debian 11 does not have libdecor, so we can't just install
# it through apt.

if [ "$1" = "x86_64" ]; then
    arch="amd64"
elif [ "$1" = "aarch64" ]; then
    arch="arm64"
else
    echo "Unsupported architecture: $1"
    exit 1
fi

cd /tmp

wget https://dl.principia-web.se/deps/linux/libdecor-0-0_0.2.2-2_${arch}.deb
wget https://dl.principia-web.se/deps/linux/libdecor-0-dev_0.2.2-2_${arch}.deb

dpkg-deb -x libdecor-0-0_0.2.2-2_${arch}.deb /
dpkg-deb -x libdecor-0-dev_0.2.2-2_${arch}.deb /
