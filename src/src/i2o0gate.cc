#include "i2o0gate.hh"
#include "model.hh"
#include "material.hh"

i2o0gate::i2o0gate()
{
    this->set_material(&m_iomisc);
    this->set_mesh(mesh_factory::get_mesh(MODEL_I2O0));

    this->num_s_in = 2;
    this->num_s_out = 0;

    this->menu_scale = 1.5f;

    this->s_in[0].lpos = b2Vec2(-.125f, -.025f);
    this->s_in[1].lpos = b2Vec2( .125f, -.025f);

    this->set_as_rect(.25f, .175f);
}
