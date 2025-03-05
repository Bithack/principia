#pragma once

#include "i2o0gate.hh"

#define NUM_WAVEFORMS 5
extern const char *speaker_options[NUM_WAVEFORMS];

class speaker : public i2o0gate
{
    int slot;

  public:
    speaker();
    const char* get_name(){return "Synthesizer";};
    edevice* solve_electronics(void);

    void init()
    {
        this->slot = -1;
    }
};
