#pragma once

struct sfxemitter_option {
    const char *name;
    void *sound;
    int chunk;
};
#define NUM_SFXEMITTER_OPTIONS 22
extern struct sfxemitter_option sfxemitter_options[NUM_SFXEMITTER_OPTIONS];

#define SFX_CHUNK_RANDOM 0xffffff

#ifdef __cplusplus

#include "edevice.hh"
#include "i2o0gate.hh"

class sfxemitter : public i2o0gate
{
  public:
    sfxemitter();
    edevice* solve_electronics();
    const char* get_name(){return "SFX Emitter";}
};

class sfxemitter_2 : public i2o0gate
{
  public:
    sfxemitter_2();
    edevice* solve_electronics();
    const char* get_name(){return "SFX Emitter";}
};

#endif
