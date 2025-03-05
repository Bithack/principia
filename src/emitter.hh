#pragma once

#include "edevice.hh"

class emitter : public edev_multiconnect
{
  private:
    int size;
    int num_in_field;
    uint64_t emit_interval;

    /* State-variables */
    bool did_emit;
    bool do_accumulate;
    int state;
    uint64_t time;

    float emitter_w;
    float emitter_h;
    float frame_w;
    float frame_h;

    b2Fixture *f, *f_frame;

    tms_entity *frame_entity; /* for multiemitter */
    tms_entity *field;
    float field_life;
    property *emit_properties;

  public:
    emitter(int size);
    const char *get_name() {
        switch (this->size) {
            case 1: return "Emitter";
            case 2: return "Multi-emitter";
            case 0: default: return "Mini emitter";
        }
    };

    void on_touch(b2Fixture *my, b2Fixture *other);
    void on_untouch(b2Fixture *my, b2Fixture *other);
    void add_to_world();

    void on_load(bool created, bool has_state);
    void setup();
    void init();

    void step();

    float get_slider_snap(int s);
    float get_slider_value(int s);
    void on_slider_change(int s, float value);
    const char *get_slider_label(int s){
        if (s == 0)
            return "Emit Interval";
        else
            return "Emit Velocity";
    };

    edevice* solve_electronics();
    void update(void);

    bool can_handle(entity *e) const __attribute__((nonnull(2)));
    void update_effects();

    void recreate_multiemitter_shape();
    void set_partial(uint32_t id); /* for multi-emitter */
    void copy_properties(entity *e);
    void load_properties();

    void write_state(lvlinfo *lvl, lvlbuf *lb)
    {
        entity::write_state(lvl, lb);

        lb->w_s_uint8(this->did_emit ? 1 : 0);
        lb->w_s_uint8(this->do_accumulate ? 1 : 0);
        lb->w_s_uint32(this->state);
        lb->w_s_uint64(this->time);
    }

    void read_state(lvlinfo *lvl, lvlbuf *lb)
    {
        entity::read_state(lvl, lb);

        this->did_emit = (lb->r_uint8() != 0);
        this->do_accumulate = (lb->r_uint8() != 0);
        this->state = lb->r_uint32();
        this->time = lb->r_uint64();
    }
};
