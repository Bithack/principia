#include "i1o0gate.hh"
#include "model.hh"
#include "material.hh"
#include "world.hh"
#include "game.hh"

i1o0gate::i1o0gate()
{
    this->menu_scale = 1.5f;

    this->set_mesh(mesh_factory::get_mesh(MODEL_I1O0));
    this->set_material(&m_iomisc);

    this->num_s_out = 0;
    this->num_s_in = 1;

    this->s_in[0].ctype = CABLE_RED;
    this->s_in[0].lpos = b2Vec2(0.f, 0.f);

    this->set_as_rect(.2f, .2f);
    float qw = this->width/2.f + 0.15f;
    float qh = this->height/2.f + 0.15f;
    this->query_sides[0].Set(0.f,  qh); /* up */
    this->query_sides[1].Set(-qw, 0.f); /* left */
    this->query_sides[2].Set(0.f, -qh); /* down */
    this->query_sides[3].Set( qw, 0.f); /* right */
}

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

