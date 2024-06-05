#pragma once

#include "entity.hh"

static inline uint32_t
pack_rgba(float _r, float _g, float _b, float _a)
{
    int r = _r * 255.f;
    int g = _g * 255.f;
    int b = _b * 255.f;
    int a = _a * 255.f;

    return (uint32_t)(r << 24 | g << 16 | b << 8 | a);
}

static inline void
unpack_rgba(uint32_t color, float *r, float *g, float *b, float *a)
{
    int _r = (color >> 24) & 0xFF;
    int _g = (color >> 16) & 0xFF;
    int _b = (color >>  8) & 0xFF;
    int _a = (color      ) & 0xFF;

    *r = _r / 255.f;
    *g = _g / 255.f;
    *b = _b / 255.f;
    *a = _a / 255.f;
}

enum {
    BG_WOOD_1,
    BG_WOOD_2,
    BG_CONCRETE,
    BG_SPACE,
    BG_WOOD_3,
    BG_OUTDOOR,
    BG_COLORED,
    BG_COLORED_SPACE,
};

class simplebg : public entity
{
    tms::entity *borders[4];

  public:
    bool bottom_only;
    simplebg();
    const char *get_name() { return "Simple BG"; }
    bool set_level_size(uint16_t left, uint16_t right, uint16_t down, uint16_t up);
    void set_repeating(bool repeat);
    void set_color(tvec4 c);
    void set_color(uint32_t color)
    {
        float r,g,b,a;
        unpack_rgba(color, &r, &g, &b, &a);

        this->set_color(tvec4f(r, g, b, a));
    }

    void add_to_world(){};
};
