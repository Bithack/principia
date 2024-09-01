#pragma once

#include "edevice.hh"

#include <Box2D/Box2D.h>

#define SCANNER_MAX_POINTS 40
#define SCANNER_MIN_LEN .05f
#define SCANNER_REACH 200.f

class scanner : public ecomp_multiconnect
{
  private:
    class cb_handler : public b2RayCastCallback
    {
      private:
        scanner *self;

      public:
        cb_handler(scanner *s)
        {
            this->self = s;
        };

        float32 ReportFixture(b2Fixture *f, const b2Vec2 &pt, const b2Vec2 &nor, float32 fraction);
    };

    cb_handler *handler;
    b2Vec2 points[SCANNER_MAX_POINTS];
    int num_points;

    b2Fixture *result_fx;
    b2Vec2     result_nor;
    b2Vec2     result_pt;

    bool active;

  public:
    scanner();
    ~scanner();

    void init();

    void step();
    void tick();
    void update_effects();
    edevice* solve_electronics();
    const char *get_name(){return "Laser";};

    const char *get_slider_label(int s){return "Wavelength";};
    float get_slider_snap(int s){return .1f;};
    float get_slider_value(int s){return this->properties[0].v.f;}
    void on_slider_change(int s, float value);

    void write_quickinfo(char *out);
};

class mirror : public composable_multiconnect
{
  public:
    mirror();

    const char *get_name(){return "Laser bouncer";};
};

class laser_sensor : public ecomp_simpleconnect
{
  public:
    laser_sensor();
    bool laserhit;

    const char *get_name(){return "Laser sensor";};
    void setup(){this->laserhit=false;};
    edevice* solve_electronics(void);
    void on_load(bool created, bool has_state);

    void write_quickinfo(char *out){sprintf(out, "%s (wavelength: %.1f)", this->get_name(), this->properties[0].v.f);};
    const char *get_slider_label(int s){if (s==0)return "Wavelength";else return "Size";};
    float get_slider_snap(int s){if (s == 0) return .1f; else return 1.f;};
    float get_slider_value(int s){if (s == 0) return this->properties[0].v.f; else return (float)this->properties[1].v.i;};
    void on_slider_change(int s, float value);
};
