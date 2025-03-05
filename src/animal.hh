#pragma once

#include "creature.hh"
#include "activator.hh"

struct animal_data {
    const char *name;
    uint8_t body_shape;
    tvec2 body_size;
    tvec2 neck_pos;
    tvec2 head_pos;
    tvec2 neck_angle;
    uint8_t feet_type;
    uint8_t head_type;
    float feet_offset;
    float feet_width; /* distance from the first feet pair to the second, only for quadruped feet */
    float default_speed;
    float default_jump_strength;
    tms::material *material;
    struct tms_mesh **mesh;
    tvec3 color;
};

enum {
    ANIMAL_TYPE_COW,
    ANIMAL_TYPE_PIG,
    ANIMAL_TYPE_OSTRICH,

    NUM_ANIMAL_TYPES
};

enum {
    AGE_CHILD,
    AGE_TEENAGER,
    AGE_ADULT
};

#define BODY_SHAPE_RECT   0
#define BODY_SHAPE_CIRCLE 1

#define ANIMAL_ANGULAR_DAMPING 10.f

extern struct animal_data animal_data[NUM_ANIMAL_TYPES];

class animal : public creature, public activator
{
  public:
    animal(uint32_t animal_type);

    b2Fixture *f_body;
    b2Fixture *fx_sensor;
    float speed;
    float target_neck_angle;

    uint8_t previous_age_state;
    uint8_t age_state;

    bool do_recreate_shape;
    bool do_update_fixture;

    void reset_friction();

    b2Fixture *get_body_fixture() {return f_body;};

    const char *get_name()
    {
        if (this->get_animal_type() < NUM_ANIMAL_TYPES)
            return animal_data[this->get_animal_type()].name;

        return "Animal";
    }

    float get_feet_speed(){return .5f;};

    bool is_roaming();

    inline uint32_t get_animal_type()
    {
        return this->properties[0].v.i;
    }

    void set_speed(float s) { this->speed = s; }

    void init();
    void setup();
    void restore();
    void on_pause();
    void write_state(lvlinfo *lvl, lvlbuf *lb);
    void read_state(lvlinfo *lvl, lvlbuf *lb);

    void attack(int add_cooldown=0);
    void attack_stop();

    void update_fixture();
    void create_fixtures();

    void action_on();
    void action_off();

    void update();
    void tick();
    void step();
    void mstep();

    void update_age(bool force=false);

    void recreate_shape();
    void create_head_joint();
    void set_animal_type(uint32_t ct);

    void on_load(bool created, bool has_state);

    void reset_damping();

    void perform_logic();

    int get_default_feet_type()
    {
        uint32_t at = this->get_animal_type();
        return animal_data[at].feet_type;
    }

    int get_default_head_type()
    {
        uint32_t at = this->get_animal_type();
        return animal_data[at].head_type;
    }

    float get_slider_snap(int s)
    {
        return .1f;
    };

    float get_slider_value(int s)
    {
        return this->properties[1].v.f;
    }

    void on_slider_change(int s, float value)
    {
        if (s == 0) {
            this->properties[1].v.f = value;
            this->update_age(true);
            this->do_recreate_shape = true;
        }
    }

    const char *get_slider_label(int s) {
        if (s == 0) {
            return "Age";
        } else {
            return "";
        }
    };

    activator *get_activator() { return this; }
    entity *get_activator_entity() { return this; }
    float get_activator_radius() { return 1.f; }
    b2Vec2 get_activator_pos() { return this->get_position(); }
    void activate(creature *by);
    void on_touch(b2Fixture *my, b2Fixture *other);
    void on_untouch(b2Fixture *my, b2Fixture *other);
};
