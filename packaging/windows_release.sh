
# Commented out because I prefer doing the compilation mzself
#./go --clean --release

rm -rf release
mkdir -p release

cp principia.exe release/

cd release
# collect up GTK3 junk to make it work
PIXBUF_DIR="lib/gdk-pixbuf-2.0/2.10.0"
mkdir -p $PIXBUF_DIR/loaders/
cp /mingw64/$PIXBUF_DIR/loaders.cache $PIXBUF_DIR/
cp /mingw64/$PIXBUF_DIR/loaders/libpixbufloader-{jpeg,png}.dll $PIXBUF_DIR/loaders/

SCHEMAS_DIR="share/glib-2.0/schemas"
mkdir -p $SCHEMAS_DIR
cp /mingw64/$SCHEMAS_DIR/{gschema.dtd,gschemas.compiled} $SCHEMAS_DIR/

cd ..

../packaging/mingw-bundledlls release/principia.exe --copy

cp ../packaging/principia_install.nsi .
cp -r ../packaging/installer/ .

makensis principia_install
