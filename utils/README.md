# `utils`
This directory contains utility scripts and programs used for Principia development or for working with file formats.

## Scripts
- `prepare_release_builds.sh`:
- `set_version.lua`: Update the version number across the codebase when preparing a release (See [Making a Release](https://principia-web.se/wiki/Making_a_Release#incrementing-version-tagging)).
- `update-sandbox-menu.sh`: Update the pre-rendered sandbox and item menu

## Programs
Most of these programs rely on source files from the main Principia codebase and need to be compiled to run. Likely only works on Linux. You can use the `Makefile` in each directory or run `make` in the `utils` directory to build all of them.

- `lvledit`: Edit metadata of Principia level files
- `progress-get`: Get leaderboard score for a given level from a data.bin file
