Principia Open Source Project
=========
Principia is a sandbox physics game originally released in November 2013. It is the successor to the Android hit game "Apparatus".

The project can be built on Windows, Linux, Android or iOS. The iOS version lags behind and lacks a lot of UI. The Android version is outdated and probably needs some work to compile.

Compilation on Windows and Linux should be easy, see further below.

NOTE!
Mote documentation and resources will come.

Getting involved
--------
Please join the Official Unofficial Discord here:
https://discord.gg/qV6APzKfk9

Follow Bithack on Twitter:
https://www.twitter.com/Bithack

Compiling and running
--------

## Building on Windows

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


## Building on Linux (Debian)

$ sudo apt-get install automake libgtk2.0-dev libgl-dev libxss-dev libxxf86vm-dev libasound2-dev libudev-dev valgrind

$ cd build-linux;

$ ./go


If everything goes well, Principia will start but then freeze at the loading screen due some uninitialize directories. Terminate Principia by replying 'y' in the gdb prompt in the terminal, then in the same terminal, go up a directory and launch it from the parent directory instead:

$ cd ..

$ build-linux/apparatus2

(this bug will be fixed in short)


License
---------
See LICENSE.md

