#pragma once

#include "edevice.hh"

class trampoline : public edev
{
  private:
    int status;
    float last_force;
    float last_factor;
    float input_voltage;

    class tpad : public entity
    {
        private:
          trampoline *parent;
        public:
          tpad(trampoline *parent);
          const char *get_name() { return "Trampoline pad"; }
          b2Vec2 get_position();
          float get_angle();
          void update();
          void add_to_world(){};
    };

    tpad *pad;

  public:
    float k;
    b2Body *pad_body;
    b2PrismaticJoint *joint;
    trampoline();
    void update(void);
    void ghost_update(void);
    void pre_write(void);
    void step();
    void add_to_world();
    void remove_from_world();
    bool allow_connection(entity *asker, uint8_t frame, b2Vec2 p);
    void set_position(float x, float y, uint8_t frame=0);
    void set_angle(float a);

    uint32_t get_num_bodies();
    b2Body* get_body(uint8_t body);

    void setup(){this->input_voltage = 0.f;};

    edevice* solve_electronics();

    const char *get_name(){return "Trampoline";};
};
