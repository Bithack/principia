/*
  SDL_mixer:  An audio mixer library based on the SDL library
  Copyright (C) 1997-2024 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#ifndef SDL_MIXER_H_
#define SDL_MIXER_H_

#include <SDL3/SDL.h>
#include <SDL3/SDL_begin_code.h>

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/**
 * Printable format: "%d.%d.%d", MAJOR, MINOR, MICRO
 */
#define SDL_MIXER_MAJOR_VERSION 3
#define SDL_MIXER_MINOR_VERSION 0
#define SDL_MIXER_MICRO_VERSION 0

/**
 * The default mixer has 8 simultaneous mixing channels
 */
#ifndef MIX_CHANNELS
#define MIX_CHANNELS    8
#endif

/* Good default values for a PC soundcard */
#define MIX_DEFAULT_FREQUENCY   44100
#define MIX_DEFAULT_FORMAT      SDL_AUDIO_S16
#define MIX_DEFAULT_CHANNELS    2
#define MIX_MAX_VOLUME          128 /* Volume of a chunk */

/**
 * The internal format for an audio chunk
 */
typedef struct Mix_Chunk {
    int allocated;
    Uint8 *abuf;
    Uint32 alen;
    Uint8 volume;       /* Per-sample volume, 0-128 */
} Mix_Chunk;

/**
 * The different fading types supported
 */
typedef enum {
    MIX_NO_FADING,
    MIX_FADING_OUT,
    MIX_FADING_IN
} Mix_Fading;

/**
 * Open the default audio device for playback.
 */
extern SDL_DECLSPEC bool SDLCALL Mix_OpenAudio(SDL_AudioDeviceID devid, const SDL_AudioSpec *spec);

/**
 * Suspend or resume the whole audio output.
 */
extern SDL_DECLSPEC void SDLCALL Mix_PauseAudio(int pause_on);

/**
 * Find out what the actual audio device parameters are.
 */
extern SDL_DECLSPEC bool SDLCALL Mix_QuerySpec(int *frequency, SDL_AudioFormat *format, int *channels);

/**
 * Dynamically change the number of channels managed by the mixer.
 */
extern SDL_DECLSPEC int SDLCALL Mix_AllocateChannels(int numchans);

/**
 * Load a supported audio format into a chunk.
 */
extern SDL_DECLSPEC Mix_Chunk * SDLCALL Mix_LoadWAV_RW(SDL_IOStream *src, int freesrc);

/**
 * Load a supported audio format into a chunk.
 */
extern SDL_DECLSPEC Mix_Chunk * SDLCALL Mix_LoadWAV(const char *file);


/**
 * Load a raw audio data from memory as quickly as possible.
 */
extern SDL_DECLSPEC Mix_Chunk * SDLCALL Mix_QuickLoad_RAW(Uint8 *mem, Uint32 len);

/**
 * Free an audio chunk.
 */
extern SDL_DECLSPEC void SDLCALL Mix_FreeChunk(Mix_Chunk *chunk);

/**
 * Get a list of chunk decoders that this build of SDL_mixer provides.
 */
extern SDL_DECLSPEC int SDLCALL Mix_GetNumChunkDecoders(void);

/**
 * Get a chunk decoder's name.
 */
extern SDL_DECLSPEC const char * SDLCALL Mix_GetChunkDecoder(int index);

/**
 * Check if a chunk decoder is available by name.
 */
extern SDL_DECLSPEC bool SDLCALL Mix_HasChunkDecoder(const char *name);

typedef void (SDLCALL *Mix_MixCallback)(void *udata, Uint8 *stream, int len);

/**
 * Set a function that is called after all mixing is performed.

 */
extern SDL_DECLSPEC void SDLCALL Mix_SetPostMix(Mix_MixCallback mix_func, void *arg);

typedef void (SDLCALL *Mix_ChannelFinishedCallback)(int channel);

/**
 * Set a callback that runs when a channel has finished playing.
 */
extern SDL_DECLSPEC void SDLCALL Mix_ChannelFinished(Mix_ChannelFinishedCallback channel_finished);


#define MIX_CHANNEL_POST  (-2)

typedef void (SDLCALL *Mix_EffectFunc_t)(int chan, void *stream, int len, void *udata);

typedef void (SDLCALL *Mix_EffectDone_t)(int chan, void *udata);


/**
 * Register a special effect function.
 */
extern SDL_DECLSPEC bool SDLCALL Mix_RegisterEffect(int chan, Mix_EffectFunc_t f, Mix_EffectDone_t d, void *arg);


/**
 * Explicitly unregister a special effect function.
 */
extern SDL_DECLSPEC bool SDLCALL Mix_UnregisterEffect(int channel, Mix_EffectFunc_t f);

/**
 * Explicitly unregister all special effect functions.
 */
extern SDL_DECLSPEC bool SDLCALL Mix_UnregisterAllEffects(int channel);


#define MIX_EFFECTSMAXSPEED  "MIX_EFFECTSMAXSPEED"

/*
 * These are the internally-defined mixing effects. They use the same API that
 *  effects defined in the application use, but are provided here as a
 *  convenience. Some effects can reduce their quality or use more memory in
 *  the name of speed; to enable this, make sure the environment variable
 *  MIX_EFFECTSMAXSPEED (see above) is defined before you call
 *  Mix_OpenAudio().
 */


/**
 * Set the panning of a channel.
 */
extern SDL_DECLSPEC bool SDLCALL Mix_SetPanning(int channel, Uint8 left, Uint8 right);


/**
 * Set the position of a channel.
 */
extern SDL_DECLSPEC bool SDLCALL Mix_SetPosition(int channel, Sint16 angle, Uint8 distance);


/**
 * Set the "distance" of a channel.
 */
extern SDL_DECLSPEC bool SDLCALL Mix_SetDistance(int channel, Uint8 distance);


/* end of effects API. */



/**
 * Reserve the first channels for the application.
 */
extern SDL_DECLSPEC int SDLCALL Mix_ReserveChannels(int num);


/* Channel grouping functions */

/**
 * Assign a tag to a channel.
 */
extern SDL_DECLSPEC bool SDLCALL Mix_GroupChannel(int which, int tag);

/**
 * Assign several consecutive channels to the same tag.
 */
extern SDL_DECLSPEC bool SDLCALL Mix_GroupChannels(int from, int to, int tag);

/**
 * Finds the first available channel in a group of channels.
 */
extern SDL_DECLSPEC int SDLCALL Mix_GroupAvailable(int tag);

/**
 * Returns the number of channels in a group.
 */
extern SDL_DECLSPEC int SDLCALL Mix_GroupCount(int tag);

/**
 * Find the "oldest" sample playing in a group of channels.
 */
extern SDL_DECLSPEC int SDLCALL Mix_GroupOldest(int tag);

/**
 * Find the "most recent" sample playing in a group of channels.
 */
extern SDL_DECLSPEC int SDLCALL Mix_GroupNewer(int tag);

/**
 * Play an audio chunk on a specific channel.
 */
extern SDL_DECLSPEC int SDLCALL Mix_PlayChannel(int channel, Mix_Chunk *chunk, int loops);

/**
 * Play an audio chunk on a specific channel for a maximum time.
 */
extern SDL_DECLSPEC int SDLCALL Mix_PlayChannelTimed(int channel, Mix_Chunk *chunk, int loops, int ticks);

/**
 * Play an audio chunk on a specific channel, fading in the audio.
 */
extern SDL_DECLSPEC int SDLCALL Mix_FadeInChannel(int channel, Mix_Chunk *chunk, int loops, int ms);

/**
 * Play an audio chunk on a specific channel, fading in the audio, for a
 * maximum time.
 */
extern SDL_DECLSPEC int SDLCALL Mix_FadeInChannelTimed(int channel, Mix_Chunk *chunk, int loops, int ms, int ticks);

/**
 * Set the volume for a specific channel.
 */
extern SDL_DECLSPEC int SDLCALL Mix_Volume(int channel, int volume);

/**
 * Set the volume for a specific chunk.
 */
extern SDL_DECLSPEC int SDLCALL Mix_VolumeChunk(Mix_Chunk *chunk, int volume);

/**
 * Set the master volume for all channels.
 */
extern SDL_DECLSPEC int SDLCALL Mix_MasterVolume(int volume);

/**
 * Halt playing of a particular channel.
 */
extern SDL_DECLSPEC void SDLCALL Mix_HaltChannel(int channel);

/**
 * Halt playing of a group of channels by arbitrary tag.
 */
extern SDL_DECLSPEC void SDLCALL Mix_HaltGroup(int tag);

/**
 * Change the expiration delay for a particular channel.
 */
extern SDL_DECLSPEC int SDLCALL Mix_ExpireChannel(int channel, int ticks);

/**
 * Halt a channel after fading it out for a specified time.
 */
extern SDL_DECLSPEC int SDLCALL Mix_FadeOutChannel(int which, int ms);

/**
 * Halt a playing group of channels by arbitrary tag, after fading them out
 * for a specified time.
 */
extern SDL_DECLSPEC int SDLCALL Mix_FadeOutGroup(int tag, int ms);

/**
 * Query the fading status of a channel.
 */
extern SDL_DECLSPEC Mix_Fading SDLCALL Mix_FadingChannel(int which);

/**
 * Pause a particular channel.
 */
extern SDL_DECLSPEC void SDLCALL Mix_Pause(int channel);

/**
 * Resume a particular channel.
 */
extern SDL_DECLSPEC void SDLCALL Mix_Resume(int channel);

/**
 * Query whether a particular channel is paused.
 */
extern SDL_DECLSPEC int SDLCALL Mix_Paused(int channel);

/**
 * Check the playing status of a specific channel.
 */
extern SDL_DECLSPEC int SDLCALL Mix_Playing(int channel);

/**
 * Get the Mix_Chunk currently associated with a mixer channel.
 */
extern SDL_DECLSPEC Mix_Chunk * SDLCALL Mix_GetChunk(int channel);

/**
 * Close the mixer, halting all playing audio.
 */
extern SDL_DECLSPEC void SDLCALL Mix_CloseAudio(void);


/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#include <SDL3/SDL_close_code.h>

#endif /* SDL_MIXER_H_ */
