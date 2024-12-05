#!/bin/bash

# Builds an AppImage using appimagetool.
# You need to run this inside of a build directory in the source tree,
# it will generate the build files and compile Principia for you.

# This script should be run on Debian 11 Bullseye.

# Download appimagetool
if [ ! -f appimagetool ]; then
	wget https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage -O appimagetool
	# Newer appimagetool uses Zstd compression which appimagelauncher doesn't support. :/
	#wget https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-x86_64.AppImage -O appimagetool
	chmod +x appimagetool
fi

# Remove old appdir
rm -rf AppDir

# Compile
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_INSTALL_PREFIX=AppDir/usr/
ninja

# Strip binary and create debug symbol file
objcopy --only-keep-debug principia principia.debug
objcopy --strip-debug --add-gnu-debuglink=principia.debug principia

# Install into AppDir
ninja install
cd AppDir

# Put desktop and icon at root
cp usr/share/applications/principia.desktop principia.desktop
cp usr/share/icons/hicolor/128x128/apps/principia.png principia.png
ln -s principia.png .DirIcon

cat > AppRun <<\APPRUN
#!/bin/bash

if ! command -v -- "xdg-mime" > /dev/null 2>&1; then
	echo "Required XDG helper scripts required by Principia are not found."
	echo "Please install 'xdg-utils' from your system's package manager."
	exit 43
fi

# Register URL handler, which points at the AppImage executable itself ($APPIMAGE)

URL_HANDLER=~/.local/share/applications/principia-url-handler.desktop
mkdir -p ~/.local/share/applications/

echo "[Desktop Entry]" > "$URL_HANDLER"
echo "Name=Principia URL Handler" >> "$URL_HANDLER"
echo "Exec=${APPIMAGE// /\\ } %u" >> "$URL_HANDLER" # Escape spaces if path has them
echo "Type=Application" >> "$URL_HANDLER"
echo "Terminal=false" >> "$URL_HANDLER"
echo "NoDisplay=true" >> "$URL_HANDLER"
echo "MimeType=x-scheme-handler/principia;" >> "$URL_HANDLER"

xdg-mime default principia-url-handler.desktop x-scheme-handler/principia

# Now launch it...

APP_PATH="$(dirname "$(readlink -f "${0}")")"
export LD_LIBRARY_PATH="${APP_PATH}"/usr/lib/:"${LD_LIBRARY_PATH}"
exec "${APP_PATH}/usr/bin/principia" "$@"
APPRUN
chmod +x AppRun

# List of libraries from the system that should be bundled in the AppImage.
# We are very conservative with what is bundled, GTK3 is expected to be already
# installed by the user and other libraries like libcurl and libfreetype are
# expected to already exist on the system.
INCLUDE_LIBS=(
	libGLEW.so.2.1
	libjpeg.so.62
	libpng16.so.16
)

mkdir -p usr/lib/
for i in "${INCLUDE_LIBS[@]}"; do
	cp /usr/lib/x86_64-linux-gnu/$i usr/lib/
done

# Copy our own built SDL2
cp /usr/lib/libSDL2-2.0.so.0 usr/lib/

# Actually build the appimage
cd ..
# LZMA compression since appimagelauncher doesn't support Zstd
ARCH=x86_64 ./appimagetool --appimage-extract-and-run --comp xz AppDir/
