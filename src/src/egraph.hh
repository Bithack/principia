#pragma once

#include "entity.hh"
#include "edevice.hh"

#define GRAPH_BUFSZ 125

class egraph : public brcomp_multiconnect
{
    float buffer[GRAPH_BUFSZ];
    int buf_p;

  public:
    egraph();
    edevice* solve_electronics();
    const char *get_name(){return "Grapher";};

    void setup();
    void on_pause();
    void update_effects(void);

    void write_state(lvlinfo *lvl, lvlbuf *lb)
    {
        entity::write_state(lvl, lb);

        lb->w_s_uint32(this->buf_p);
        for (int x=0; x<GRAPH_BUFSZ; ++x) {
            lb->w_s_float(buffer[x]);
        }
    }

    void read_state(lvlinfo *lvl, lvlbuf *lb)
    {
        entity::read_state(lvl, lb);

        this->buf_p = (int)lb->r_uint32();
        for (int x=0; x<GRAPH_BUFSZ; ++x) {
            buffer[x] = lb->r_float();
        }
    }
};
