#pragma once

#include "entity.hh"
#include "material.hh"

enum {
    DECORATION_STONE1,
    DECORATION_STONE2,
    DECORATION_STONE3,
    DECORATION_STONE4,
    DECORATION_STONE5,
    DECORATION_STONE6,
    DECORATION_MUSHROOM1,
    DECORATION_MUSHROOM2,
    DECORATION_MUSHROOM3,
    DECORATION_MUSHROOM4,
    DECORATION_MUSHROOM5,
    DECORATION_MUSHROOM6,
    DECORATION_SIGN1,
    DECORATION_SIGN2,
    DECORATION_SIGN3,
    DECORATION_SIGN4,
    DECORATION_FENCE_WOOD,
    DECORATION_STONE_HEAD,
    DECORATION_PLANT1,
    DECORATION_PLANT2,
    DECORATION_PLANT3,
    DECORATION_PLANT4,
   //DECORATION_BANNER,

    NUM_DECORATIONS,
};

extern struct decoration_info {
    const char      *name;
    tvec2            size;
    b2Vec2           vertices[6];
    int              num_vertices;
    m               *material;
    struct tms_mesh **mesh;
    bool             is_rect;
    bool             can_rotate;
} decorations[NUM_DECORATIONS];

class decoration : public entity
{
  private:
    void recreate_shape();

  public:
    decoration();
    const char* get_name() { return "Decoration"; }
    void tick();
    void init();
    void on_load(bool created, bool has_state);
    void write_quickinfo(char *out);
    void add_to_world();
    void set_decoration_type(uint32_t t);

    void update();

    inline uint32_t get_decoration_type()
    {
        uint32_t t = this->properties[0].v.i;
        return t >= NUM_DECORATIONS ? NUM_DECORATIONS-1 : t;
    }

    float get_slider_snap(int s){return .1f;};
    float get_slider_value(int s){return this->properties[1].v.f;};
    const char *get_slider_label(int s){return "Rotation";};
    void on_slider_change(int s, float value){ this->properties[1].v.f = value;};
    bool do_recreate_shape;
};
