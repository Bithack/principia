#pragma once

#include "entity.hh"
#include "const.hh"

struct resource_data {
    const char *name;
    tms::material *material;
    struct tms_mesh **mesh;
    int set_uniform;
    tvec4 uniform;
};

#define NUM_RESOURCES_1_5 NUM_RESOURCES

#define RESOURCE_COLOR_RUBY     { 1.f, .1f, .1f, .9f }
#define RESOURCE_COLOR_SAPPHIRE { .1f, .1f, 1.f, .9f }
#define RESOURCE_COLOR_EMERALD  { .1f, 1.f, .1f, .9f }
#define RESOURCE_COLOR_TOPAZ    { 1.f, .6f, .1f, .9f }
#define RESOURCE_COLOR_DIAMOND  { .9f, .9f, .9f, .9f }
#define RESOURCE_COLOR_COPPER   { .72f*.5f, .45f*.5f, .2f*.5f, 1.f }
#define RESOURCE_COLOR_IRON     { .7f*.5f, .7f*.5f, .7f*.5f, 1.f }
#define RESOURCE_COLOR_ALUMINIUM { .9f*.5f, .9f*.5f, .9f*.5f, 1.f }

extern struct resource_data resource_data[NUM_RESOURCES];

class resource : public entity
{
  public:
    resource();
    const char *get_name() { return "Resource"; }
    const char *get_real_name()
    {
        return resource_data[this->resource_type].name;
    }
    void write_tooltip(char *out) {
        strcpy(out, resource_data[this->resource_type].name);
    };
    void write_quickinfo(char *out);

    void add_to_world();
    void on_load(bool created, bool has_state);

    float get_slider_snap(int s) { return 1.f / 14.f; }
    float get_slider_value(int s);
    const char *get_slider_label(int s) { return "Amount"; }
    void on_slider_change(int s, float value);

    void set_resource_type(uint8_t resource_type);
    void set_amount(uint32_t amount);

    inline uint8_t get_resource_type() { return this->resource_type; }
    inline uint32_t get_amount() { return this->amount; }

    uint32_t get_sub_id() { return this->get_resource_type(); }

    uint8_t resource_type;
    uint32_t amount;
};
