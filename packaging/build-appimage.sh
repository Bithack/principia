#!/bin/bash

# Builds an AppImage using appimagetool.
# You need to run this inside of a build directory.

# Remove old appdir
rm -rf AppDir

mkdir -p AppDir

DESTDIR="AppDir/" ninja install

cd AppDir

ln -s share/applications/principia.desktop principia.desktop
ln -s share/icons/hicolor/128x128/apps/principia.png principia.png
ln -s principia.png .DirIcon

cat > AppRun <<\EOF
#!/bin/sh
PATH="$(dirname "$(readlink -f "${0}")")"
export LD_LIBRARY_PATH="${PATH}"/lib/:"${LD_LIBRARY_PATH}"
exec "${PATH}/bin/principia" "@$"
EOF
chmod +x AppRun

mkdir -p lib

# Copy over libraries
ldd bin/principia | awk 'NF == 4 { system("cp " $3 " lib/") }'

cd lib/
# Remove some libraries that break things
rm libc.so.6 libdl.so.2 libdrm.so.2 libgcc_s.so.1 libGL.so.1 libGLdispatch.so.0 libGLX.so.0 libm.so.6 libOpenGL.so.0 libpthread.so.0 librt.so.1 libstdc++.so.6
rm libresolv.so.2 libxcb.so.1 libX11.so.6 libasound.so.2 libgdk_pixbuf-2.0.so.0 libfontconfig.so.1 libthai.so.0 libfreetype.so.6 libharfbuzz.so.0 libcom_err.so.2 libexpat.so.1 libglib-2.0.so.0 libgpg-error.so.0 libkeyutils.so.1 libp11-kit.so.0 libuuid.so.1 libz.so.1
cd ..

# Actually build the appimage
cd ..
ARCH=x86_64 appimagetool AppDir/
