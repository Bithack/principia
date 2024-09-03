#include "statesaver.hh"
#include "world.hh"
#include "game.hh"

edevice*
statesaver::solve_electronics()
{
    if (!this->s_in[0].is_ready()) {
        return this->s_in[0].get_connected_edevice();
    }

    bool enable = (bool)(int)roundf(this->s_in[0].get_value());

    /* TODO: limit how often we can save */
    if (enable && last_in != enable) {
        tms_debugf("invoking save");
        G->save_state();
    }

    last_in = enable;

    return 0;
}

void
statesaver::write_state(lvlinfo *lvl, lvlbuf *lb)
{
    i1o0gate::write_state(lvl, lb);

    lb->w_s_uint8((uint8_t)this->last_in);
}

void
statesaver::read_state(lvlinfo *lvl, lvlbuf *lb)
{
    i1o0gate::read_state(lvl, lb);
    this->last_in = (bool)lb->r_uint8();
}
