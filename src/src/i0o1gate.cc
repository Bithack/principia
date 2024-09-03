#include "i0o1gate.hh"
#include "model.hh"

i0o1gate::i0o1gate()
{
    this->set_mesh(mesh_factory::get_mesh(MODEL_I0O1));
    this->set_material(&m_iomisc);

    this->menu_scale = 1.5;

    this->num_s_out = 1;
    this->s_out[0].lpos = b2Vec2(0.f, .0625f);
    this->set_as_rect(.15f, .2f);
}

