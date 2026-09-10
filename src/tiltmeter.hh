#pragma once

#include "edevice.hh"

/**
 * Class representing the Tiltmeter object.
 *
 * Player Wiki ref: https://principia-web.se/wiki/Tiltmeter
 */
class tiltmeter : public ecomp_multiconnect {
  public:
    tiltmeter();
    edevice* solve_electronics();
    const char *get_name() { return "Tiltmeter"; }

    float get_slider_snap(int s);
    float get_slider_value(int s);
    const char *get_slider_label(int s);
    void on_slider_change(int s, float value);
};
