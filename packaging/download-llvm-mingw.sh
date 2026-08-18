#!/bin/bash
set -e
topdir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
if [ -z "$1" ]; then
	echo "Usage: $0 <dest path>"
	exit 1
fi

date=20260616
name=llvm-mingw-${date}-ucrt-ubuntu-22.04-x86_64.tar.xz
wget "https://github.com/mstorsjo/llvm-mingw/releases/download/$date/$name" -O "$name"
# check the sha256sum of the downloaded file
sha256sum -c <<EOF
534b92e067b22a6b4441f48ae9240a3341b17825d04d577eab0cf85c44b4deda $name
EOF

tar -xaf "$name" -C "$1" --strip-components=1
rm -f "$name"
