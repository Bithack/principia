#pragma once

#include "entity.hh"

class ball;

class pipeline : public entity_simpleconnect
{
  private:
    struct tms_entity *house;
    struct tms_entity *piston;

    float house_open;

  public:
    b2Joint *joint;
    bool used;
    ball *b;
    int s;
    pipeline();

    void update(void);
    void step(void);
    void add_to_world();
    const char *get_name(){return "Ball Pipeline";};
    void take(ball *b);
    void toggle_axis_rot();
};
