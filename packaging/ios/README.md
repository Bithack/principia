# Principia iOS buildscripts
This directory contains buildscripts for building Principia on iOS. The port is in a very early and buggy state at the moment.

## Building

Building for a real iOS device:

```bash
IOS_SDK=iphoneos IOS_ARCH=arm64 ./build-ios.sh
```

Building for iPhone Simulator on Apple Silicon Macs:

```bash
IOS_SDK=iphonesimulator IOS_ARCH=arm64 ./build-ios.sh
```

Building for iPhone Simulator on Intel hardware or in a macOS VM:

```bash
IOS_SDK=iphonesimulator IOS_ARCH=x86_64 ./build-ios.sh
```
