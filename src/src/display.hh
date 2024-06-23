#pragma once

#include "edevice.hh"
#include <stdint.h>

#ifdef TMS_BACKEND_PC
#define DISPLAY_MAX_TOTAL_SQUARES 16384
#else
#define DISPLAY_MAX_TOTAL_SQUARES 4096
#endif

#define DISPLAY_MAX_SYMBOLS 40

class display : public brcomp_multiconnect
{

  public:
    display();
    void construct();
    void load_symbols();
    virtual const char *get_name() = 0;
    void update_effects();
    virtual edevice* solve_electronics() = 0;
    void setup();
    void on_pause();
    void on_load(bool created, bool has_state);

    void write_state(lvlinfo *lvl, lvlbuf *lb)
    {
        entity::write_state(lvl,lb);
        lb->w_s_uint8(this->active_symbol);
    }

    void read_state(lvlinfo *lvl, lvlbuf *lb)
    {
        entity::read_state(lvl,lb);
        this->active_symbol = lb->r_uint8();
    }

    void init()
    {
        this->active = (this->s_in[0].p == 0);
    }

    inline int get_num_symbols()
    {
        return this->num_symbols;
    }

    void set_active_symbol(int pos)
    {
        this->active_symbol = pos;
    }

    inline uint64_t get_symbol(int index)
    {
        if (index < 0 || index > this->num_symbols) {
            return 0;
        }

        return this->symbols[index];
    }

    static void _init();
    static void reset();
    static void upload();
    static void add(float x, float y, float z, float sn, float cs, float col);
    static void add_custom(float x, float y, float z, float width, float height, float sn, float cs, float col);
    static tms::entity *get_full_entity();

    uint64_t symbols[DISPLAY_MAX_SYMBOLS];
    int      num_symbols;
    int      active_symbol;
    bool     active;
};

class passive_display : public display
{
  public:
    passive_display();
    const char *get_name() { return "Passive display"; };
    edevice* solve_electronics();
};

class active_display : public display
{
  public:
    active_display();
    const char *get_name() { return "Active display"; };
    edevice* solve_electronics();
};
