#pragma once

#ifndef SCREENSHOT_BUILD
#define ENABLE_SOUND
#endif

#include "const.hh"
#include <tms/core/tms.h>

#ifdef ENABLE_SOUND
#include "SDL_mixer.h"
#else

// Dummy typedef to make it still compile when SDL2_mixer is not present
typedef struct Mix_Chunk {
    int allocated;
    uint8_t *abuf;
    uint32_t alen;
    uint8_t volume;
} Mix_Chunk;

#endif

#define SM_MAX_CHANNELS     18
#define SM_FRONT_BIAS       -2.f /* front bias for 3d effect */
#define SM_DIST_FACTOR      4.8f
#define SM_MAX_CHUNKS       8
#define SM_MIN_VOLUME       0.02f
#define SM_IMPACT_THRESHOLD    2.f
#define SM_MAX_CONCUR       3

#define SM_GENWAVE_PRETICKS 10
#define SM_GENWAVE_NUM_TICKS 16

#define SM_GENWAVE_NO_COMMAND 0
#define SM_GENWAVE_STOP       1
#define SM_GENWAVE_START      2

struct genwave_data
{
    bool available;
    bool started;
    double phase;

    float bitcrushing;
    float volume_vibrato;
    float volume_vibrato_width;
    float freq_vibrato;
    float freq_vibrato_width;
    float pulse_width;
    int waveform;
    int bitcrush_counter;

    struct tick_data {
        float volume;
        double freq;
        int command;
    } ticks[SM_GENWAVE_NUM_TICKS];
};

struct sm_chunk
{
    Mix_Chunk *chunk;
    const char *name;
};

class sm_sound
{
  public:
    sm_chunk   chunks[SM_MAX_CHUNKS];
    int        num_chunks;
    int        last_chan;
    uint64_t   last_time;
    long       min_repeat_ms;
    const char *name;

    sm_sound(){this->num_chunks=0;this->reset();this->min_repeat_ms = 50;};

    void reset()
    {
        this->last_chan = -1;
        this->last_time = 0;
    };

    void add_chunk(const char *filename, const char *chunk_name);
};

class sm_channel
{
  public:
    int        chan;
    tvec2      position;
    bool       playing;
    sm_sound  *sound;
    void      *ident;
    uint8_t    distance;
    float      volume;
    bool       global;

    void update_position();
};

class sm
{
    static bool  initialized;

  public:
    static sm_sound* sound_lookup[SND__NUM];
    static sm_sound* get_sound_by_id(uint32_t sound_id);
    static sm_channel channels[SM_MAX_CHANNELS];
    static bool     enabled;
    static tvec2    position; /* position of the listener */

    static void load_settings();

    static float volume;

    static int      tick_counter;
    static int      remainder_counter;
    static uint64_t read_counter;
    static uint64_t write_counter;

    static genwave_data generated[SM_MAX_CHANNELS];
    static Mix_Chunk *genchunk;

    /* all sound effects */
    static sm_sound       test;
    static sm_sound       wood_metal;
    static sm_sound       wood_wood;
    static sm_sound       wood_hollowwood;
    static sm_sound       metal_metal2;
    static sm_sound       metal_metal;
    static sm_sound       click;
    static sm_sound       drop_absorb;
    static sm_sound       robot;
    static sm_sound       robot_shoot;
    static sm_sound       shotgun_shoot;
    static sm_sound       shotgun_cock;
    static sm_sound       railgun_shoot;
    static sm_sound       robot_bomb;
    static sm_sound       rocket;
    static sm_sound       thruster;
    static sm_sound       explosion;
    static sm_sound       explosion_light;
    static sm_sound       sheet_metal;
    static sm_sound       rubber;
    static sm_sound       absorb;
    static sm_sound       emit;
    static sm_sound       win;
    static sm_sound       lose;
    static sm_sound       ding;
    static sm_sound       weird;
    static sm_sound       detect;
    static sm_sound       warning;
    static sm_sound       drum1;
    static sm_sound       drum2;
    static sm_sound       motor_startstop;
    static sm_sound       bubbles;
    static sm_sound       rocket_launcher_shoot;
    static sm_sound       cash_register;
    static sm_sound       discharge;
    static sm_sound       plasma_shoot;
    static sm_sound       buster_shoot;
    static sm_sound       buster_shoot_maxcharge;
    static sm_sound       buster_charge;
    static sm_sound       stone_stone;
    static sm_sound       chest_open;
    static sm_sound       chest_open_rare;
    static sm_sound       mining_hit_ore;
    static sm_sound       zapper;
    static sm_sound       happy;
    static sm_sound       compressor;
    static sm_sound       compressor_reverse;
    static sm_sound       saw_loop;
    static sm_sound       chop_wood;
    static sm_sound       swish_hammer;
    static sm_sound       swish_blade;
    static sm_sound       swish_spear;
    static sm_sound       swish_axe;

    static bool gen_started;
    static void pause_all(void);
    static void resume_all(void);
    static void play_gen(int x);
    static void step();
    static void init();
    static bool stop(sm_sound *snd, void *ident);
    static void stop_all(void);
    static void play(sm_sound *snd, float x, float y, uint8_t random, float volume, bool loop=false, void *ident=0, bool global=false);
};
