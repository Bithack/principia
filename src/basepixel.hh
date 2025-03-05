#pragma once

#include "entity.hh"
#include <set>

#define PIXEL_GRID_MAX 64
#define BASE_PIXEL_RADIUS 32

class basepixel : public entity,
                  public b2QueryCallback
{
  public:
    basepixel();

    virtual void on_pause();
    virtual void setup();

    inline float get_size(){return (float)((1 << this->properties[0].v.i))*.25f;};

    float get_slider_snap(int s);
    float get_slider_value(int s);
    const char *get_slider_label(int s){return "Size";};
    void on_slider_change(int s, float value);
    void add_to_world();
    virtual void recreate_shape(bool skip_search=false, bool dynamic=false);
    void gather_connected_pixels(std::set<entity*> *entities, int depth=-1);
    bool search(float start_x, float start_y, int search_width, float *rx, float *ry);
    void set_position(float x, float y, uint8_t fr=0);
    void on_load(bool created, bool has_state);
    virtual void update_appearance() = 0;
    virtual void update_fixture();
    virtual void construct();
    bool ReportFixture(b2Fixture *f);

    static float start_x, start_y;
    static int search_width;
    static basepixel *found[PIXEL_GRID_MAX*PIXEL_GRID_MAX];
    static bool disable_search;
    bool got_pos;
    static int radius;
};
