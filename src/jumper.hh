#pragma once

#include "wplug.hh"
#include "edevice.hh"
#include "cable.hh"

class socket;

class jumper : public wplug
{
  public:
    jumper();
    edevice *get_edevice(){return (edevice*)this;};
    entity *get_entity(){return (entity*)this;};
    const char *get_name(){return "Jumper";};
    void write_quickinfo(char *out);

    int get_dir(){return CABLE_IN;}

    void setup();
    void on_pause();

    edevice* solve_electronics();

    float get_slider_snap(int s);
    float get_slider_value(int s);
    void on_slider_change(int s, float value);
    const char *get_slider_label(int s){return "Value";};

    void on_load(bool created, bool has_state){};

    inline void update_color()
    {
        float v = this->properties[0].v.f;
        this->set_uniform("~color", .2f, .5f+(v/2.f), .2f, 1.f);
    }

    void restore()
    {
        entity::restore();

        this->reconnect();
    }
};
