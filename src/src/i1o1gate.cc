#include "i1o1gate.hh"
#include "model.hh"
#include "material.hh"

i1o1gate::i1o1gate()
{
    this->set_material(&m_i1o1);

    this->num_s_in = 1;
    this->num_s_out = 1;

    this->menu_scale = 1.5f;

    this->s_in[0].ctype = CABLE_RED;
    this->s_in[0].lpos = b2Vec2(0.f, -.25f);
    this->s_in[0].angle = -M_PI/2.f;
    this->s_out[0].ctype = CABLE_RED;
    this->s_out[0].lpos = b2Vec2(0.f, .25f);
    this->s_out[0].angle = M_PI/2.f;

    this->set_as_rect(.15f, .375f);
}

i1o1gate_mini::i1o1gate_mini()
{
    this->set_mesh(mesh_factory::get_mesh(MODEL_CCLIP));
    this->set_material(&m_iomisc);

    this->num_s_in = 1;
    this->num_s_out = 1;

    this->menu_scale = 1.5f;

    this->s_in[0].lpos = b2Vec2(0.f, -.125f);
    this->s_out[0].lpos = b2Vec2(0.f, .125f);

    this->set_as_rect( .25f/2.f, .5f/2.f);
}

i1o1gate_fifo::i1o1gate_fifo()
{
    this->set_mesh(mesh_factory::get_mesh(MODEL_FIFO));
    this->set_material(&m_iomisc);

    this->num_s_in = 1;
    this->num_s_out = 1;

    this->s_in[0].lpos = b2Vec2(0.f, -.35f);
    this->s_out[0].lpos = b2Vec2(0.f, .35f);

    this->set_as_rect(.25f, .5f);
}


















