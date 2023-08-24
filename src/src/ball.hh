#pragma once

#include "entity.hh"

void ball_update_customz(struct tms_entity *e);

class ball : public entity
{
  private:
    int btype;

  public:
    float z;
    float target_z;

    ball(int type);

    int saved_layer;

    float layer_new, layer_blend, layer_old;

    void on_load(bool created, bool has_state);
    void setup();
    void on_pause();

    void construct();
    void set_layer(int l);
    void layermove(int dir);

    void add_to_world();
    const char* get_name(){
        if (btype == 0)
            return "Ball";
        else if (btype == 1)
            return "Metal ball";
        else
            return "Interactive ball";
    };
};
