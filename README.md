Principia Open Source Project
=========
Principia is a sandbox physics game originally released in November 2013. It is the successor to the Android hit game "Apparatus".

The project can be built on Windows, Linux, Android or iOS. The iOS version lags behind and lacks a lot of UI. The Android version is outdated and probably needs some work to compile.

Compilation on Windows and Linux should be easy, see below.

Compiling and running
--------

Building on Windows

- Install MSYS2 64bit

$ pacman -Syu

Terminal will ask to close it self when done. GO to start menu and run MSYS Mingw32 64-bit

$ pacman -S --neded base-devel mingw-w64-x86_64-toolchain autotools
$ pacman -S mingw-w64-x86_64-gtk2
$ pacman -S mingw-w64-x86_64-curl

Navigate to the 'build-windows' folder inside where you cloned Principia, for example:

$ cd /c/Users/<username>/Principia/build-windows

$ ./autogen.sh
$ ./configure
$ ./go

Clean with:
$ make clean

See the build-linux directory for Linux instructions. More info coming soon.


License
---------
See LICENSE.md

