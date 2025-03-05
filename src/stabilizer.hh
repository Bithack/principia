#pragma once

#include "edevice.hh"

/* stabilizer using damping */
class estabilizer : public ecomp_multiconnect
{
    float adamp;
    float ldamp;
    bool do_refresh_damping;

    void refresh_damping();

  public:
    estabilizer();
    const char *get_name(void){return "Stabilizer";};

    void step();
    void setup();

    float get_slider_snap(int s)
    {
        return .05f;
    };
    float get_slider_value(int s)
    {
        return this->properties[s].v.f;
    };
    const char *get_slider_label(int s){
        if (s == 0) return "Angular Damping";
        else return "Linear Damping";
    };
    void on_slider_change(int s, float value);

    edevice* solve_electronics();

    void write_state(lvlinfo *lvl, lvlbuf *lb)
    {
        entity::write_state(lvl, lb);

        lb->w_s_float(this->adamp);
        lb->w_s_float(this->ldamp);
    }

    void read_state(lvlinfo *lvl, lvlbuf *lb)
    {
        entity::read_state(lvl, lb);

        this->adamp = lb->r_float();
        this->ldamp = lb->r_float();
    }

    void restore()
    {
        entity::restore();

        this->refresh_damping();
    }
};
