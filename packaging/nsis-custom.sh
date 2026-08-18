#!/bin/bash
set -e

# This should be built on a version of Debian/Ubuntu no newer than what is used in CI.
#
# Packages:
# apt install git g++ scons curl unzip zlib1g-dev --no-install-recommends

NSIS_VER=3.12
NSIS_TAG=v312

# if /tmp/nsis-custom/ directory exists, remove it
if [ -d /tmp/nsis-custom ]; then
	rm -rf /tmp/nsis-custom
fi

mkdir -p /tmp/nsis-custom
cd /tmp/nsis-custom

# if nsis/ directory does not exist, download NSIS and extract it there
curl -L -o /tmp/nsis-$NSIS_VER.zip http://downloads.sourceforge.net/project/nsis/NSIS%203/$NSIS_VER/nsis-$NSIS_VER.zip

unzip /tmp/nsis-$NSIS_VER.zip -d /tmp/nsis-$NSIS_VER
mv /tmp/nsis-$NSIS_VER/nsis-$NSIS_VER nsis

git clone --depth 1 --branch $NSIS_TAG https://github.com/NSIS-Dev/nsis src

cd src

scons -j8 SKIPSTUBS=all SKIPPLUGINS=all SKIPUTILS=all SKIPMISC=all NSIS_CONFIG_CONST_DATA_PATH=no PREFIX=../nsis/Bin/ install-compiler

cd ../nsis
rm -rf makensis.exe makensisw.exe NSIS.chm NSIS.exe \
	Bin/*.exe Bin/*.dll Bin/*.bin Docs Examples

# write a shell script "makensis" in the root that runs Bin/makensis
echo '#!/bin/bash' > makensis
echo 'DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"' >> makensis
echo 'exec "$DIR/Bin/makensis" "$@"' >> makensis
chmod +x makensis

cd ..
tar -czf nsis-custom-$NSIS_VER.tar.gz nsis
