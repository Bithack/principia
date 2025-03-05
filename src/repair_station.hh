#pragma once

#include "edevice.hh"
#include "activator.hh"
#include "ud2.hh"
#include <set>
#include <vector>

class creature;

enum {
    RS_PENDING,
    RS_PRE_REPAIR,
    RS_WAIT_FOR_ROBOT,
    RS_CLOSE_DOORS,
    RS_CHECK_COSTS,
    RS_BEGIN_REPAIRS,

    RS_OPEN_DOORS,
    RS_EJECT_ROBOT,

    RS_END,
};

struct rs_item {
    uint32_t g_id;
    uint32_t item_type; /* item type or scrap type? */
    uint32_t category;
    bool dragging;
    tvec2 pos;
    float durability;
};

class repair_station : public edev_multiconnect, public activator
{
  private:
    b2Fixture *absorb_sensor;
    b2Fixture *detection_sensor;
    b2Fixture *bottom;
    b2Fixture *activator_sensor;
    b2Fixture *fx_ladder;
    b2Fixture *fx_ladder_step;
    b2Fixture *fx_control;
    const float sensor_radius;
    const b2Vec2 sensor_offset;
    struct ud2_info ladder_ud2;
    struct ud2_info ladder_step_ud2;

    b2Vec2 top_size;
    b2Vec2 top_offset;
    b2Vec2 bottom_size;
    b2Vec2 bottom_offset;

    float side_door_state;
    float front_door_state;
    struct tms_entity *side_doors[2];
    struct tms_entity *front_doors[2];

    std::set<uint32_t>  targets;
    p_id                repair_target_id;

    uint8_t repair_state;
    uint64_t down_pid;
    uint32_t repair_elapsed;
    uint32_t repair_time;

  public:
    repair_station();
    const char *get_name() { return "Repair Station"; }
    activator *get_activator() { return this; }
    entity *get_activator_entity() { return this; }
    float get_activator_radius() { return this->sensor_radius; }
    b2Vec2 get_activator_pos() { return this->get_position() + this->sensor_offset; }
    void activate(creature *by);

    void read_state(lvlinfo *lvl, lvlbuf *lb);
    void write_state(lvlinfo *lvl, lvlbuf *lb);

    void update();
    void update_effects();

    void add_to_world();

    void init();
    void restore();
    void setup();
    void on_pause();

    void on_target_disappear();

    void clear_slots();
    void load_equipments(creature *c);
    void apply_equipments(creature *c);

    void on_touch(b2Fixture *my, b2Fixture *other);
    void on_untouch(b2Fixture *my, b2Fixture *other);

    void step();

    std::vector<struct rs_item*> inventory;
    uint32_t down_step;

    friend class game;
};
