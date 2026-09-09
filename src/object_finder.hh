#pragma once

#include "iomiscgate.hh"

/**
 * Class representing the Object Finder object.
 *
 * Player Wiki ref: https://principia-web.se/wiki/Object_Finder
 */
class object_finder : public i0o2gate {
  public:
    object_finder();
    edevice* solve_electronics();

    float get_slider_snap(int s);
    float get_slider_value(int s);
    void on_slider_change(int s, float value);
    const char *get_slider_label(int s) { return "Dist. sensitivity"; }
    const char *get_name() { return "Object Finder"; }
};

/**
 * Class representing the Cursor Finder object.
 *
 * Player Wiki ref: https://principia-web.se/wiki/Cursor_Finder
 */
class cursor_finder : public i0o2gate {
  public:
    cursor_finder();

    float get_slider_snap(int s);
    float get_slider_value(int s);
    void on_slider_change(int s, float value);
    const char *get_slider_label(int s) { return "Dist. threshold"; }
    const char *get_name() { return "Cursor Finder"; }
    edevice* solve_electronics();
};
