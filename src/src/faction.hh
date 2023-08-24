#pragma once

#include "mood.hh"

#include <tms/math/vector.h>
#include <inttypes.h>

#define ENEMY    0
#define FRIENDLY 1
#define NEUTRAL  2

enum {
    FACTION_ENEMY,
    FACTION_FRIENDLY,
    FACTION_NEUTRAL,
    FACTION_CHAOTIC,

    NUM_FACTIONS
};

/* TODO: Store behavioural information here */

extern class faction_info
{
  public:
    uint8_t id;
    const char *name;
    tvec4 color;
    uint8_t relationship[NUM_FACTIONS];
    float mood_base[NUM_MOODS];
    float mood_decr[NUM_MOODS];
    float mood_sensitivity[NUM_MOODS];
} factions[NUM_FACTIONS];
