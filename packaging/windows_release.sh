#!/bin/bash -eu

# This script assumes you already have built the executable

rm -rf release
mkdir -p release

cp principia.exe release/

../packaging/bundledlls release/principia.exe release/

cp ../packaging/principia_install.nsi .
makensis principia_install
