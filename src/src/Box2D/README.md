Multithreaded Box2D
===================

This is a modified version of Box2D+LiquidFun.

You should probably not use it outside Principia, it is fine-tuned for Principia physics and even includes some of our math-stuff for determinism and speed.

On the other hand, it fixes many Box2D issues that were present at the time (most of them still).

For instance, island sleeping is much, much more stable and noise tolerant, which was a requirements for the dynamic trees in Principia.

Probably the most interesting feature is that our implementation of multi-threading. Two separate parts are multithreaded, Island solving and collision checking.

