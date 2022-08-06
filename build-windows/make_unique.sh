rm sha1_test
touch sha1_test
rm principia-setup.exe
touch ../src/tms/backends/windows/main.cc
./go --release
makensis principia_install
openssl sha1 principia.exe p-$UNIQUE.exe >> sha1_test
