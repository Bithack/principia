#pragma once

#include "composable.hh"

#define BEAM_THICK   0
#define BEAM_THIN    1
#define BEAM_RUBBER  2
#define BEAM_PLASTIC 3
#define BEAM_SEP     4

class beam_ray_cb;

/**
 * Class representing various beam-like objects (Thick Plank, Plank, Rubber Beam, Plastic Beam, Separator).
 *
 * Player Wiki ref:
 * - https://principia-web.se/wiki/Plank
 * - https://principia-web.se/wiki/Thick_Plank
 * - https://principia-web.se/wiki/Rubber_Beam
 * - https://principia-web.se/wiki/Plastic_Beam
 * - https://principia-web.se/wiki/Separator
 */
class beam : public composable {
  private:
    connection c[2];
    int btype;

  public:
    beam(int btype);

    const char* get_name() {
        switch (this->btype) {
            case BEAM_THICK:   return "Thick Plank";
            case BEAM_THIN:    return "Plank";
            case BEAM_RUBBER:  return "Rubber Beam";
            case BEAM_PLASTIC: return "Plastic Beam";
            case BEAM_SEP:     return "Separator";
            default:           return "";
        }
    }
    void on_load(bool created, bool has_state);

    void find_pairs();
    connection* load_connection(connection &conn);

    float get_slider_snap(int s) {
        if (s == 0)
            return 1.f / 3.f;
        else
            return .05f;
    }
    float get_slider_value(int s) {
        if (s == 0)
            return this->properties[0].v.i / 3.f;
        else
            return ((float)this->properties[4].v.f - ENTITY_DENSITY_SCALE_MIN) / ENTITY_DENSITY_SCALE_MAX;
    }
    const char *get_slider_label(int s){
        if(s == 0)
            return "Size";
        else
            return "Density scale";
    }

    void on_slider_change(int s, float value);
    void update_fixture();
    void tick();

    int get_beam_type() { return this->btype; }

    bool do_update_fixture;

    void set_color(tvec4 c);
    tvec4 get_color();

    void set_density_scale(float v) {
        if (this->btype == BEAM_PLASTIC)
            this->properties[4].v.f = v;
    }

    float get_density_scale() {
        if (this->btype == BEAM_PLASTIC)
            return this->properties[4].v.f;
        else
            return 1.f;
    }

    friend class beam_ray_cb;
};
