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
