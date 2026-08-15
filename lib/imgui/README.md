# Dear Imgui
These are source files from the Dear Imgui library, for Principia's experimental Dear Imgui dialog backend.

All files are currently taken from commit 46d39d56febc2a00bdd2270dc88c8a13f2a0441a with the following modifications:

- `imconfig.h` is entirely custom and should be kept
- `imgui_impl_opengl3.cpp`:
	- Desktop GL / GLES2 is dynamically determined at runtime using `_tms.use_gles`

All downstream modifications should have a `// XXX PRINCIPIA XXX` comment for keeping track of them.

To update from upstream, see the `copy.sh` script in this folder.
