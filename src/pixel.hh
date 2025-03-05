#pragma once

#include "basepixel.hh"
#include "receiver.hh"

#define PIXEL_DYN_BUF 3

class pixel : public basepixel,
              public receiver_base
{
  private:
    float alpha;
    float last_size;

    int buf_p;
    bool buf[PIXEL_DYN_BUF];
    bool dynamic;
    bool optimized_render;

    float r, g, b;

  public:
    pixel();

    static void initialize();
    static struct tms_entity *get_entity(int x);
    static void reset_counter();
    static void upload_buffers();
    void remove_from_world();

    const char *get_name(void){return "Pixel";};

    void set_optimized_render(bool enable);
    void mstep();
    void update();
    void on_pause();
    void init();
    void setup();

    void recreate_shape(bool skip_search=false, bool dynamic=false);
    void on_load(bool created, bool has_state);
    void update_appearance();
    void update_fixture();
    void construct();

    inline float get_alpha()
    {
        return this->alpha;
    }
    void set_color(tvec4 c);
    tvec4 get_color();

    void write_state(lvlinfo *lvl, lvlbuf *lb)
    {
        entity::write_state(lvl, lb);

        lb->w_s_float(this->alpha);
        lb->w_s_float(this->last_size);
        lb->w_s_float(this->pending_value);
    }

    void read_state(lvlinfo *lvl, lvlbuf *lb)
    {
        entity::read_state(lvl, lb);

        this->alpha = lb->r_float();
        this->last_size = lb->r_float();
        this->pending_value = lb->r_float();
    }
};
