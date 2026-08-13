#pragma once

#include "edevice.hh"

#define CLIP_INTERFACE 0
#define CLIP_SIGNAL    1

/**
 * Class representing the Signal and Interface Clip objects.
 *
 * Player Wiki ref:
 * - https://principia-web.se/wiki/Signal_Clip
 * - https://principia-web.se/wiki/Interface_Clip
 */
class clip : public brcomp_multiconnect {
  private:
    int clip_type;

  public:
    connection c;
    clip(int _clip_type);

    const char *get_name() {
        switch (this->clip_type) {
            case CLIP_INTERFACE: return "Interface Clip";
            case CLIP_SIGNAL: return "Signal Clip";
            default: return "";
        }
    }

    edevice* solve_electronics();
};
