#pragma once

#include "edevice.hh"
#include "item.hh"

#include <vector>

#define FACTORY_QUEUE_SIZE 6
#define FACTORY_SPEED 1

enum {
    FACTORY_GENERIC,
    FACTORY_ROBOT,
    FACTORY_ARMORY,
    FACTORY_OIL_MIXER,

    NUM_FACTORIES,
};

class p_text;

#define FACTORY_NUM_EXTRA_PROPERTIES 6

#define IS_FACTORY(x) (x == O_FACTORY || x == O_ROBOT_FACTORY || x == O_ARMORY || x == O_OIL_MIXER)

struct factory_object {
    uint32_t gid;
    float scale_x; /* rendering scale */
    float scale_y;
    int set_prop_0;

    struct worth worth;

    factory_object()
        : gid(0)
        , scale_x(1.f)
        , scale_y(1.f)
        , set_prop_0(-1)
    { }

    struct factory_object& g_id(uint32_t g_id)
    {
        this->gid = g_id;

        return *this;
    }

    struct factory_object& item(uint32_t item_id)
    {
        this->gid = item_id;
        this->worth = item_options[item_id].worth;

        return *this;
    }

    struct factory_object& prop0(int prop0)
    {
        this->set_prop_0 = prop0;

        return *this;
    }

    struct factory_object& scale(float x, float y=1.f)
    {
        this->scale_x = x;
        this->scale_y = y;

        return *this;
    }

    /* Add num resources */
    struct factory_object& add(uint8_t resource_type, uint32_t num)
    {
        this->worth.add(resource_type, num);

        return *this;
    }

    /* Add oil */
    struct factory_object& add_oil(float val)
    {
        this->worth.add_oil(val);

        return *this;
    }

    /* Add resources and oil from another worth object */
    struct factory_object& add(const struct worth& _worth)
    {
        this->worth.add(_worth);

        return *this;
    }
};

struct factory_queue_item {
    int item;
    int count;
    uint64_t completed;
    float build_speed;
};

class factory : public edev_simpleconnect
{
  private:
    tvec2 sz_top;
    tvec2 sz_bottom;

  public:
    uint64_t                  build_accum;
    struct factory_queue_item queue[FACTORY_QUEUE_SIZE];
    int                       queue_size;

    b2Fixture *emit_sensor, *absorb_sensor;
    b2Fixture *bottom;
    int        factory_type;
    bool       last_absorbed;
    bool       item_completed;
    bool       error;
    bool       very_hungry;
    std::vector<uint32_t> autorecipe_list;
    float      conveyor_speed;
    bool       conveyor_invert;
    uint32_t   num_absorbed;
    uint64_t   down_pid;
    uint32_t   down_step;

    float get_oil() const
    {
        return this->properties[1].v.f;
    }
    uint32_t get_num_resources(int resource) const
    {
        return this->properties[FACTORY_NUM_EXTRA_PROPERTIES+resource].v.i;
    }

    void add_oil(float v){this->properties[1].v.f += v;};
    void set_num_resources(int resource, uint32_t i)
    {
        this->properties[FACTORY_NUM_EXTRA_PROPERTIES+resource].v.i = i;
    };

    void add_resources(int resource, uint32_t i)
    {
        this->properties[FACTORY_NUM_EXTRA_PROPERTIES+resource].v.i += i;
    };

    factory(int factory_type);
    void step();
    int handle_event(uint32_t type, uint64_t pointer_id, tvec2 pos);
    void setup();
    void restore();
    void init();
    bool can_afford(const struct factory_object& fo) const;
    const char *get_name() {
        switch (this->factory_type) {
            case FACTORY_ROBOT:
                return "Robot Factory";

            case FACTORY_ARMORY:
                return "Armory";

            case FACTORY_OIL_MIXER:
                return "Oil Mixer";

            case FACTORY_GENERIC:
            default:
                return "Factory";
        }
    };
    void read_state(lvlinfo *lvl, lvlbuf *lb);
    void write_state(lvlinfo *lvl, lvlbuf *lb);
    void add_to_world();
    void on_touch(b2Fixture *my, b2Fixture *other);
    void on_untouch(b2Fixture *my, b2Fixture *other);
    int add_to_queue(int sel);
    void cleanup_completed();
    float get_tangent_speed();
    edevice *solve_electronics();

    std::vector<struct factory_object>& objects();

    static void generate_recipes(std::vector<uint32_t> *vec, const char *buf);
    static void init_recipes();
};
