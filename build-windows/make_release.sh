make clean
./go --release
rm release/*
mkdir release
cp principia.exe release/
./mingw-bundledlls release/principia.exe --copy
makensis principia_install
mv principia-setup.exe release/
