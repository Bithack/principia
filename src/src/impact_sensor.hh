#pragma once

#include "edevice.hh"

class impact_sensor : public ecomp_multiconnect
{
  private:
    bool pressure;

  public:
    float impulse;

    impact_sensor(bool pressure);
    const char* get_name(){
        if (this->pressure) return "Pressure sensor";
        return "Impact sensor";
    }

    void on_load(bool created, bool has_state);

    const char *get_slider_label(int s){
        switch (s) {
            case 0: return "Size";
            case 1:
                if (this->pressure) {
                    return "Sensitivity";
                } else {
                    return "Threshold";
                }
        }
        return "";
    };
    float get_slider_snap(int s);
    float get_slider_value(int s);
    void on_slider_change(int s, float value);

    void update_appearance();

    edevice* solve_electronics();

    inline void add_impulse(float impulse)
    {
        if (this->pressure) {
            this->impulse = this->impulse * .95f + impulse * .05f;
        } else {
            this->impulse += impulse;
        }
    }

    void write_state(lvlinfo *lvl, lvlbuf *lb)
    {
        entity::write_state(lvl, lb);

        lb->w_s_float(this->impulse);
    }

    void read_state(lvlinfo *lvl, lvlbuf *lb)
    {
        entity::read_state(lvl, lb);

        this->impulse = lb->r_float();
    }

    friend class explosive;
};
