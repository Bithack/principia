#ifndef _I2O0GATE__H_
#define _I2O0GATE__H_

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
#include "model.hh"

class i2o0gate : public brcomp_multiconnect
{
  public:
    i2o0gate();
};

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

class var_setter : public i2o0gate
{
  public:
    var_setter();
    const char* get_name(){return "Var setter";}

    edevice* solve_electronics();
    void write_quickinfo(char *out);
    bool compatible_with(entity *o);
};

class camera_rotator : public i2o0gate
{
  public:
    const char *get_name() { return "Cam Rotator"; }

    edevice *solve_electronics();
};

#endif
#endif
