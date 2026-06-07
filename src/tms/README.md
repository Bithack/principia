# TMS Game Engine
Game engine for Principia, written in C.

Basically, it uses a concept of setting up different rendering pipelines and then rendering graphs of models in these. It supports everything in terms of graphics that was required for all versions of Principia.

## C++ bindings
While TMS itself is written in C, it contains C++ bindings which allow you to interface with the engine's structures in an object-oriented manner.

When including TMS from C++ code in Principia, you would want to just include this header, which will also include everything else within an `extern "C"` block:

```c++
#include <tms/cpp.hh>
```

Typically this is always just what you need.
