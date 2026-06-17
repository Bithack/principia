This is a cut down version of SDL3_mixer for use with Principia.

It only allows playing WAV sound files.

Currently we're on the legacy `sdl2-api-on-sdl3` branch of SDL_mixer, which is SDL2_mixer adapted to work with SDL3 but with the mixer API largely unchanged. This branch is not recommended to be used, but is the easiest option for now before Principia's sound code is rewritten to use the new SDL3_mixer ("SDL_remixer") API.
