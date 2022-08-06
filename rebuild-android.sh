#!/bin/bash
cd build-android
make native-clean
make java-clean
make clean
make native-release
cd ../build-android-lite
make native-clean
make java-clean
make clean
make native-release
