/*
    TiMidity -- Experimental MIDI to WAVE converter
    Copyright (C) 1995 Tuukka Toivonen <toivonen@clinet.fi>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the Perl Artistic License, available in COPYING.
 */

/* This is for use with the SDL library */
#define SDL
#include "SDL_config.h"
#include "SDL_endian.h"

#define TIMIDITY_ERROR_SIZE 1024

/* When a patch file can't be opened, one of these extensions is
   appended to the filename and the open is tried again.
 */
#define PATCH_EXT_LIST { ".pat", 0 }

/* Acoustic Grand Piano seems to be the usual default instrument. */
#define DEFAULT_PROGRAM 0

/* 9 here is MIDI channel 10, which is the standard percussion channel.
   Some files (notably C:\WINDOWS\CANYON.MID) think that 16 is one too. 
   On the other hand, some files know that 16 is not a drum channel and
   try to play music on it. This is now a runtime option, so this isn't
   a critical choice anymore. */
#define DEFAULT_DRUMCHANNELS (1<<9)

/* A somewhat arbitrary frequency range. The low end of this will
   sound terrible as no lowpass filtering is performed on most
   instruments before resampling. */
#define MIN_OUTPUT_RATE 	4000
#define MAX_OUTPUT_RATE 	65000

/* In percent. */
/* #define DEFAULT_AMPLIFICATION 	70 */
/* #define DEFAULT_AMPLIFICATION 	50 */
#define DEFAULT_AMPLIFICATION 	30

/* Default sampling rate, default polyphony, and maximum polyphony.
   All but the last can be overridden from the command line. */
#define DEFAULT_RATE	32000
/* #define DEFAULT_VOICES	32 */
/* #define MAX_VOICES	48 */
#define DEFAULT_VOICES	256
#define MAX_VOICES	256
#define MAXCHAN		16
/* #define MAXCHAN		64 */
#define MAXNOTE		128

/* 1000 here will give a control ratio of 22:1 with 22 kHz output.
   Higher CONTROLS_PER_SECOND values allow more accurate rendering
   of envelopes and tremolo. The cost is CPU time. */
#define CONTROLS_PER_SECOND 1000

/* Strongly recommended. This option increases CPU usage by half, but
   without it sound quality is very poor. */
#define LINEAR_INTERPOLATION

/* This is an experimental kludge that needs to be done right, but if
   you've got an 8-bit sound card, or cheap multimedia speakers hooked
   to your 16-bit output device, you should definitely give it a try.

   Defining LOOKUP_HACK causes table lookups to be used in mixing
   instead of multiplication. We convert the sample data to 8 bits at
   load time and volumes to logarithmic 7-bit values before looking up
   the product, which degrades sound quality noticeably.

   Defining LOOKUP_HACK should save ~20% of CPU on an Intel machine.
   LOOKUP_INTERPOLATION might give another ~5% */
/* #define LOOKUP_HACK
   #define LOOKUP_INTERPOLATION */

/* Make envelopes twice as fast. Saves ~20% CPU time (notes decay
   faster) and sounds more like a GUS. There is now a command line
   option to toggle this as well. */
/* #define FAST_DECAY */

/* How many bits to use for the fractional part of sample positions.
   This affects tonal accuracy. The entire position counter must fit
   in 32 bits, so with FRACTION_BITS equal to 12, the maximum size of
   a sample is 1048576 samples (2 megabytes in memory). The GUS gets
   by with just 9 bits and a little help from its friends...
   "The GUS does not SUCK!!!" -- a happy user :) */
#define FRACTION_BITS 12

#define MAX_SAMPLE_SIZE (1 << (32-FRACTION_BITS))

typedef double FLOAT_T;

/* For some reason the sample volume is always set to maximum in all
   patch files. Define this for a crude adjustment that may help
   equalize instrument volumes. */
#define ADJUST_SAMPLE_VOLUMES

/* The number of samples to use for ramping out a dying note. Affects
   click removal. */
#define MAX_DIE_TIME 20

/* On some machines (especially PCs without math coprocessors),
   looking up sine values in a table will be significantly faster than
   computing them on the fly. Uncomment this to use lookups. */
/* #define LOOKUP_SINE */

/* Shawn McHorse's resampling optimizations. These may not in fact be
   faster on your particular machine and compiler. You'll have to run
   a benchmark to find out. */
#define PRECALC_LOOPS

/* If calling ldexp() is faster than a floating point multiplication
   on your machine/compiler/libm, uncomment this. It doesn't make much
   difference either way, but hey -- it was on the TODO list, so it
   got done. */
/* #define USE_LDEXP */

/**************************************************************************/
/* Anything below this shouldn't need to be changed unless you're porting
   to a new machine with other than 32-bit, big-endian words. */
/**************************************************************************/

/* change FRACTION_BITS above, not these */
#define INTEGER_BITS (32 - FRACTION_BITS)
#define INTEGER_MASK (0xFFFFFFFF << FRACTION_BITS)
#define FRACTION_MASK (~ INTEGER_MASK)

/* This is enforced by some computations that must fit in an int */
#define MAX_CONTROL_RATIO 255

typedef unsigned int uint32;
typedef int int32; 
typedef unsigned short uint16;
typedef short int16;
typedef unsigned char uint8;
typedef char int8;

/* Instrument files are little-endian, MIDI files big-endian, so we
   need to do some conversions. */

#define XCHG_SHORT(x) ((((x)&0xFF)<<8) | (((x)>>8)&0xFF))
# define XCHG_LONG(x) ((((x)&0xFF)<<24) | \
		      (((x)&0xFF00)<<8) | \
		      (((x)&0xFF0000)>>8) | \
		      (((x)>>24)&0xFF))

#if SDL_BYTEORDER == SDL_LIL_ENDIAN
#define LE_SHORT(x) x
#define LE_LONG(x) x
#define BE_SHORT(x) XCHG_SHORT(x)
#define BE_LONG(x) XCHG_LONG(x)
#else
#define BE_SHORT(x) x
#define BE_LONG(x) x
#define LE_SHORT(x) XCHG_SHORT(x)
#define LE_LONG(x) XCHG_LONG(x)
#endif

#define MAX_AMPLIFICATION 800

/* You could specify a complete path, e.g. "/etc/timidity.cfg", and
   then specify the library directory in the configuration file. */
#define CONFIG_FILE	"timidity.cfg"
#define CONFIG_FILE_ETC "/etc/timidity.cfg"
#define CONFIG_FILE_ETC_TIMIDITY_FREEPATS "/etc/timidity/freepats.cfg"

#if defined(__WIN32__) || defined(__OS2__)
#define DEFAULT_PATH	"C:\\TIMIDITY"
#else
#define DEFAULT_PATH	"/etc/timidity"
#define DEFAULT_PATH1	"/usr/share/timidity"
#define DEFAULT_PATH2	"/usr/local/share/timidity"
#define DEFAULT_PATH3	"/usr/local/lib/timidity"
#endif

/* These affect general volume */
#define GUARD_BITS 3
#define AMP_BITS (15-GUARD_BITS)

#ifdef LOOKUP_HACK
   typedef int8 sample_t;
   typedef uint8 final_volume_t;
#  define FINAL_VOLUME(v) (~_l2u[v])
#  define MIXUP_SHIFT 5
#  define MAX_AMP_VALUE 4095
#else
   typedef int16 sample_t;
   typedef int32 final_volume_t;
#  define FINAL_VOLUME(v) (v)
#  define MAX_AMP_VALUE ((1<<(AMP_BITS+1))-1)
#endif

typedef int16 resample_t;

#ifdef USE_LDEXP
#  define FSCALE(a,b) ldexp((a),(b))
#  define FSCALENEG(a,b) ldexp((a),-(b))
#else
#  define FSCALE(a,b) (float)((a) * (double)(1<<(b)))
#  define FSCALENEG(a,b) (float)((a) * (1.0L / (double)(1<<(b))))
#endif

/* Vibrato and tremolo Choices of the Day */
#define SWEEP_TUNING 38
#define VIBRATO_AMPLITUDE_TUNING 1.0L
#define VIBRATO_RATE_TUNING 38
#define TREMOLO_AMPLITUDE_TUNING 1.0L
#define TREMOLO_RATE_TUNING 38

#define SWEEP_SHIFT 16
#define RATE_SHIFT 5

#define VIBRATO_SAMPLE_INCREMENTS 32

#ifndef PI
  #define PI 3.14159265358979323846
#endif

/* The path separator (D.M.) */
#if defined(__WIN32__) || defined(__OS2__)
#  define PATH_SEP '\\'
#  define PATH_STRING "\\"
#else
#  define PATH_SEP '/'
#  define PATH_STRING "/"
#endif
