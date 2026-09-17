# Dear Imgui
These are source files from the Dear Imgui library, for Principia's experimental Dear Imgui dialog backend.

All files are currently taken from commit 8d3e37eb5343d4a5aa15718ea0a1889c6e4f2d6c with the following modifications:

- `imconfig.h` is entirely custom and should be kept
- `imgui_impl_opengl3.cpp`:
	- Desktop GL / GLES2 is dynamically determined at runtime using `_tms.use_gles`
	- Fix for Haiku OS

All downstream modifications should have a `// XXX PRINCIPIA XXX` comment for keeping track of them.

To update from upstream, see the `copy.sh` script in this folder.
