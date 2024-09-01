#pragma once

#include "entity.hh"
#include <Box2D/Box2D.h>

#define EXPLOSIVE_MAX_HP 25.f

#define EXPLOSIVE_BOMB     0
#define EXPLOSIVE_LANDMINE 1
#define EXPLOSIVE_TRIGGER  2

class explosive : public entity, public b2QueryCallback
{
  private:
    int    explosive_type;
    float hl_time;

    float  hp;
    uint64_t time;

    b2Fixture *found;
    b2Vec2     found_pt;

    void trigger(void);

  public:
    explosive(int explosive_type);

    void init();
    void setup();

    void pre_step(void);

    void add_to_world();

    const char* get_name(){
        switch (this->explosive_type) {
            case 0: return "Bomb";
            case 1: return "Land mine";
        }
        return "";
    }

    void damage(float amount)
    {
        if (this->hp > 0.f) {
            this->hp -= amount;
        }
    }

    bool ReportFixture(b2Fixture *f);

    float get_slider_snap(int s);
    float get_slider_value(int s);
    void on_slider_change(int s, float value);
    const char *get_slider_label(int s)
    {
        if (s == 0) {
            if (this->explosive_type == EXPLOSIVE_BOMB) {
                return "Fuse Timer";
            } else {
                return "Threshold";
            }
        } else {
            return "Damage";
        }
    };

    void write_state(lvlinfo *lvl, lvlbuf *lb)
    {
        entity::write_state(lvl, lb);

        lb->w_s_float(this->hp);
        if (this->explosive_type == EXPLOSIVE_BOMB) {
            lb->w_s_uint64(this->time);
        }
    }

    void read_state(lvlinfo *lvl, lvlbuf *lb)
    {
        entity::read_state(lvl, lb);

        this->hp = lb->r_float();
        if (this->explosive_type == EXPLOSIVE_BOMB) {
            this->time = lb->r_uint64();
        }
    }

    bool triggered;
    uint64_t trigger_time;
};
