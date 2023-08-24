#pragma once

#include "composable.hh"

enum {
    BOX_NORMAL,
    BOX_INTERACTIVE,
    BOX_PLASTIC,
};

class box : public composable, public b2QueryCallback
{
  private:
    connection c_back;
    connection c_front;
    connection c_side[4];

    entity *q_result;
    b2Fixture *q_result_fx;
    uint8_t q_frame;
    int q_dir;
    int box_type;
    b2Vec2 q_point;

  public:
    box(int box_type);

    const char *get_name(void){
        switch (this->box_type) {
            case BOX_NORMAL: return "Box";
            case BOX_INTERACTIVE: return "Interactive Box";
            case BOX_PLASTIC: return "Plastic Box";
        }
        return "";
    };

    void setup();
    float get_slider_snap(int s);
    float get_slider_value(int s);
    const char *get_slider_label(int s){if (s == 0) return "Size"; else return "Density scale";};
    void on_slider_change(int s, float value);

    connection* load_connection(connection &conn);
    void on_load(bool created, bool has_state);

    bool ReportFixture(b2Fixture *f);
    void find_pairs();

    void set_density_scale(float v)
    {
        if (this->box_type == BOX_PLASTIC) {
            this->properties[4].v.f = v;
        }
    }
    float get_density_scale(){if (this->box_type == 2) return this->properties[4].v.f; else return 1.f;};

    void set_color(tvec4 c);
    tvec4 get_color();
};
