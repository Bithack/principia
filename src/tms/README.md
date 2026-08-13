# TMS Game Engine
The TMS Game Engine is the backing in-house engine used to power Principia. It is a very minimal engine with just what is needed for Principia, and primarily consists of a rendering engine utilising OpenGL. SDL is used as a portability layer for things such as windowing, events and obtaining an OpenGL context.

TMS is not a general purpose game engine and contains a lot of hardcoded logic for Principia as the game developed, but it is still under the same license as the rest of Principia.

The rendering engine uses a concept of setting up different rendering pipelines and then rendering graphs of models in these. The engine also supports handling shaders, textures, meshes, materials and has an entity system that can be used to create game objects out of these. In addition it contains various utility functions such as math needed for 3D graphics, a camera system and a simple widget system for basic GUI elements.

The `backend/` folder contains the main entrypoint for the game and other glue code which is used to interface between SDL and TMS for things such as events and the game window.

## C++ bindings
While TMS itself is written in C, it contains C++ bindings which allow you to interface with the engine's structures in an object-oriented manner.

When including TMS from C++ code in Principia, you would want to just include this header, which will also include everything else within an `extern "C"` block:

```c++
#include <tms/cpp.hh>
```

Typically this is always just what you need.
