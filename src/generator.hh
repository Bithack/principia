#pragma once

#include "edevice.hh"

/**
 * Class representing the Power Supply object.
 *
 * Player Wiki ref: https://principia-web.se/wiki/Power_Supply
 */
class generator : public brcomp_multiconnect {
    float voltage;
  public:
    generator();

    float get_slider_value(int s);
    float get_slider_snap(int s);
    void on_slider_change(int s, float value);
    const char *get_slider_label(int s) { return "Voltage"; }
    void on_load(bool created, bool has_state);

    edevice* solve_electronics();
    const char* get_name() { return "Power Supply"; }
    void write_quickinfo(char *out);
};
