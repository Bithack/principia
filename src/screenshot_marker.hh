#pragma once

#include "entity.hh"
#include "edevice.hh"
#include "i1o0gate.hh"

#define HIDDEN_X -500000.f
#define HIDDEN_Y -500000.f

class screenshot_marker : public entity
{
  private:
    bool hidden;

  public:
    screenshot_marker();
    const char *get_name(){return "Cam Marker";};

    void add_to_world();

    void update();

    void setup();
    void on_pause();

    void hide();
    void show();

    float get_slider_snap(int s){return 1.f / 20.f;};
    float get_slider_value(int s)
    {
        return ((this->properties[0].v.f - 4.f) / 56.f);
    };
    const char *get_slider_label(int s){return "Zoom";};
    void on_slider_change(int s, float value);

    bool is_hidden() {return this->hidden;};

    b2Vec2 saved_position;
};

class camtargeter : public i1o0gate
{
  public:
    camtargeter();
    edevice* solve_electronics();
    const char *get_name(){return "Cam Targeter";};
};

class zoomer : public i1o0gate
{
  public:
    zoomer();
    edevice* solve_electronics();
    const char *get_name(){return "Cam Zoomer";};

    float get_slider_snap(int s){return 1.f / 20.f;};
    float get_slider_value(int s);
    const char *get_slider_label(int s){return s==0?"Zoom":"Smoothness";};
    void on_slider_change(int s, float value);
};
