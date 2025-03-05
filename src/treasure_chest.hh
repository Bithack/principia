#pragma once

#include "composable.hh"
#include "activator.hh"
#include <vector>

struct treasure_chest_item
{
    int g_id;
    int sub_id;
    int count;

    treasure_chest_item(int _g_id, int _sub_id, int _count)
        : g_id(_g_id)
        , sub_id(_sub_id)
        , count(_count)
    { }
};

class treasure_chest : public composable_simpleconnect, public activator
{
  protected:
    void emit_item(struct treasure_chest_item &tci);
    void emit_contents();

  public:
    treasure_chest();
    const char *get_name() { return "Treasure Chest"; }
    activator *get_activator() { return this; }


    float get_sensor_radius()
    {
        return 1.25f;
    }

    b2Vec2 get_sensor_offset()
    {
        return b2Vec2(0.f, 0.f);
    }

    entity *get_activator_entity() { return this; }
    float get_activator_radius() { return this->get_sensor_radius(); }
    b2Vec2 get_activator_pos() { return this->get_position() + this->get_sensor_offset(); }
    void activate(creature *by);

    uint32_t randomize_loot();

    void on_touch(b2Fixture *my, b2Fixture *other);
    void on_untouch(b2Fixture *my, b2Fixture *other);

    static struct treasure_chest_item parse_item(char *str);
    static std::vector<struct treasure_chest_item> parse_items(char *str);
};
