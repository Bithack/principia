#!/bin/bash
adb logcat | ndk-stack -sym obj/local/armeabi-v7a/
