#pragma once

#include "edevice.hh"

class fan : public ecomp_multiconnect
{
  private:
    class cb_handler : public b2RayCastCallback
    {
      private:
        fan *f;

      public:
        cb_handler(fan *f)
        {
            this->f = f;
        };

        float32 ReportFixture(b2Fixture *f, const b2Vec2 &pt, const b2Vec2 &nor, float32 fraction);
    };

    cb_handler *handler;

    b2Fixture  *found;
    b2Vec2      found_pt;

    struct tms_entity blades;
    float blade_rot;
    float blade_rot_speed;
    float force;

  public:
    fan();
    const char* get_name(){ return "Fan"; }

    void setup(){this->force = 0.f;};
    void step();
    void update();

    edevice* solve_electronics();

    void write_state(lvlinfo *lvl, lvlbuf *lb)
    {
        entity::write_state(lvl, lb);
        lb->w_s_float(this->force);
    }

    void read_state(lvlinfo *lvl, lvlbuf *lb)
    {
        entity::read_state(lvl, lb);
        this->force = lb->r_float();
    }
};
