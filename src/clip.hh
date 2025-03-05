#pragma once

#include "edevice.hh"

#define CLIP_INTERFACE 0
#define CLIP_SIGNAL    1

class clip : public brcomp_multiconnect
{
  private:
    int clip_type;

  public:
    connection c;
    clip(int _clip_type);

    const char *get_name() { switch (this->clip_type) { case CLIP_INTERFACE: return "Interface clip"; case CLIP_SIGNAL: return "Signal clip"; } return ""; };

    edevice* solve_electronics();
};
