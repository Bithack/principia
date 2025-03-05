#pragma once

#include "edevice.hh"

class button : public edev_multiconnect
{
  private:
    class bswitch : public entity
    {
      private:
        button *parent;

      public:
        bswitch(button *parent);
        const char *get_name() { return "Button switch"; }
        virtual b2Vec2 get_position();
        virtual float get_angle();
        virtual void update();
        void add_to_world(){};
    };

    bswitch *mswitch;

    int32_t num_blocking;
    uint8_t step_action;
    uint8_t button_type;
    float down_time;
    bool pressed;

    /* Variable loaded from state to store whether the switch was
     * a sensor or not. */
    bool switch_sensor_status;

  public:
    b2Fixture *switch_fx;

    void press();
    button(int button_type);
    const char *get_name(){if (this->button_type==0)return "Button"; else return "Toggle Button";};

    void step();
    void add_to_world();
    void on_touch(b2Fixture *my, b2Fixture *other);
    void on_untouch(b2Fixture *my, b2Fixture *other);

    void update(void);
    void ghost_update(void);

    void restore();
    void set_layer(int z);
    edevice* solve_electronics();

    void write_state(lvlinfo *lvl, lvlbuf *lb)
    {
        entity::write_state(lvl, lb);

        lb->w_s_bool(this->pressed);
        lb->w_s_bool(this->switch_fx->IsSensor());
    }

    void
    read_state(lvlinfo *lvl, lvlbuf *lb)
    {
        entity::read_state(lvl, lb);

        this->pressed = lb->r_bool();
        this->switch_sensor_status = lb->r_bool();
    }
};
