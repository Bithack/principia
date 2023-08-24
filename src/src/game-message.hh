#pragma once

#include <inttypes.h>

class p_text;
class pscreen;

class game_message
{
  private:
    p_text *text;
    float *alpha; /* alpha of text */
    float *oalpha; /* alpha of text outline */

    uint8_t state;
    double duration;
    double in_duration;
    double out_duration;
    double max_in_duration;
    double max_out_duration;

    inline void set_alpha(float v)
    {
        *this->alpha = v;
        *this->oalpha = v;
    }

    inline float get_alpha()
    {
        return *this->alpha;
    }

  public:
    game_message();
    void set_position(float x, float y);

    void render(pscreen *ps);

    void show(const char *text, double duration=2.5, double in_speed=0.125, double out_speed=0.125);
};
