#!/bin/bash -eu

# Copies over the files that are vendored here from a cloned Imgui source tree, to update them.

# Location to full cloned Imgui dir
IMGUI="../../../imgui"

cp ${IMGUI}/imconfig.h .
cp ${IMGUI}/imgui.{cpp,h} .
cp ${IMGUI}/imgui_{demo,draw,tables,widgets}.cpp .
cp ${IMGUI}/imgui_internal.h .
cp ${IMGUI}/imstb_{rectpack,textedit,truetype}.h .
cp ${IMGUI}/backends/imgui_impl_opengl3{.cpp,.h,_loader.h} .
cp ${IMGUI}/misc/cpp/imgui_stdlib.{cpp,h} .
