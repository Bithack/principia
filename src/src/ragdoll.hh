#pragma once

#include "entity.hh"

#include <set>

struct limb {
    struct tms_entity super;
    b2Body **body;
    tvec3 size;
    b2Vec2 pos;
    float angle;
    float sublayer;

    /* XXX: debugging purposes */
    char name[32];
};

class ragdoll : public entity
{
  private:
    /* head is this->body */
    b2Body *torso;
    b2Body *arm[4];
    b2Body *leg[4];

    struct limb limbs[9];

    b2Vec2 get_limb_pos(int n);
    float get_limb_angle(int n);

    std::set<b2Joint*> destructable_joints;

    bool joint_states[9];
    float body_states[9][3];
    joint_info *ji;

    b2CircleShape head_shape;

  public:
    ragdoll();
    ~ragdoll();

    b2Joint *joints[9];

    void update(void);
    void add_to_world();
    void remove_from_world();
    const char *get_name(void){return "Dummy";};

    void on_pause();
    void setup();

    void restore();
    void write_state(lvlinfo *lvl, lvlbuf *lb);
    void read_state(lvlinfo *lvl, lvlbuf *lb);

    void on_grab(game *g);
    void on_release(game *g);

    uint32_t get_num_bodies();
    b2Body *get_body(uint8_t frame);
    struct limb *get_limb(b2Body *b)
    {
        for (int x=0; x<9; ++x) {
            if ((*this->limbs[x].body) == b) {
                return &limbs[x];
            }
        }

        return 0;
    };

    void set_layer(int layer);
    void set_position(float x, float y, uint8_t frame=0);
    b2Vec2 local_to_world(b2Vec2 p, uint8_t frame);
    void pre_write(void);

    void recreate_head();
    void recreate_head_joint(bool destroy);

    float get_slider_snap(int s);
    float get_slider_value(int s);
    void on_slider_change(int s, float value);
    const char *get_slider_label(int s)
    {
        switch (s) {
            case 0: return "Durability";
            case 1: return "Head size";
        }
        return "";
    };
};
