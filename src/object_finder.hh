#pragma once

#include "iomiscgate.hh"

class object_finder : public i0o2gate
{
  public:
    object_finder();
    edevice* solve_electronics();

    float get_slider_snap(int s);
    float get_slider_value(int s);
    void on_slider_change(int s, float value);
    const char *get_slider_label(int s){return "Dist. sensitivity";};
    const char *get_name(){return "Object finder";};
};

class cursor_finder : public i0o2gate
{
  public:
    cursor_finder();

    float get_slider_snap(int s);
    float get_slider_value(int s);
    void on_slider_change(int s, float value);
    const char *get_slider_label(int s){return "Dist. threshold";};
    const char *get_name(){return "Cursor finder";};
    edevice* solve_electronics();
};
