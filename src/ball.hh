#pragma once

#include "entity.hh"

void ball_update_customz(struct tms_entity *e);

/**
 * Class representing all ball objects (Ball, Metal Ball, Interactive Ball).
 *
 * Player Wiki ref:
 * - https://principia-web.se/wiki/Ball
 * - https://principia-web.se/wiki/Metal_Ball
 * - https://principia-web.se/wiki/Interactive_Ball
 */
class ball : public entity {
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
            return "Metal Ball";
        else
            return "Interactive Ball";
    };
};
