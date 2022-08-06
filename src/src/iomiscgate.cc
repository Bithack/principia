#include "iomiscgate.hh"
#include "model.hh"
#include "material.hh"

i0o2gate::i0o2gate()
{
    this->set_mesh(mesh_factory::get_mesh(MODEL_I0O2));
    this->set_material(&m_iomisc);

    this->num_s_in = 0;
    this->num_s_out = 2;

    this->s_out[0].ctype = CABLE_RED;
    this->s_out[0].lpos = b2Vec2(-.25f, 0.f);
    this->s_out[1].ctype = CABLE_RED;
    this->s_out[1].lpos = b2Vec2(.25f, 0.f);

    this->set_as_rect(.525f, .175f);
}

i0o3gate::i0o3gate()
{
    this->set_mesh(mesh_factory::get_mesh(MODEL_I0O3));
    this->set_material(&m_iomisc);

    this->num_s_in = 0;
    this->num_s_out = 3;

    this->s_out[0].ctype = CABLE_RED;
    this->s_out[0].lpos = b2Vec2(-.25f, 0.f);
    this->s_out[1].ctype = CABLE_RED;
    this->s_out[1].lpos = b2Vec2(0.f, 0.f);
    this->s_out[2].ctype = CABLE_RED;
    this->s_out[2].lpos = b2Vec2(0.25f, 0.f);

    this->set_as_rect(.525f, .175f);
}

i1o2gate::i1o2gate()
{
    this->set_mesh(mesh_factory::get_mesh(MODEL_I1O2));
    this->set_material(&m_iomisc);

    this->num_s_in = 1;
    this->num_s_out = 2;

    this->s_in[0].lpos = b2Vec2(.0f, -.125f);
    this->s_out[0].lpos = b2Vec2(.125f, .125f);
    this->s_out[1].lpos = b2Vec2(-.125f, .125f);

    this->set_as_rect(.275f, .275f);
}

i1o3gate::i1o3gate()
{
    this->set_mesh(mesh_factory::get_mesh(MODEL_I1O3));
    this->set_material(&m_iomisc);

    this->num_s_in = 1;
    this->num_s_out = 3;

    this->s_in[0].ctype = CABLE_RED;
    this->s_in[0].lpos = b2Vec2(0.f, -.175f);
    this->s_out[0].ctype = CABLE_RED;
    this->s_out[0].lpos = b2Vec2(-.25f, .125f);
    this->s_out[1].ctype = CABLE_RED;
    this->s_out[1].lpos = b2Vec2(0.f, .125f);
    this->s_out[2].ctype = CABLE_RED;
    this->s_out[2].lpos = b2Vec2(.25f, .125f);

    this->set_as_rect(.5f, .3f);
}

i1o4gate::i1o4gate()
{
    this->set_mesh(mesh_factory::get_mesh(MODEL_I1O4));
    this->set_material(&m_iomisc);

    this->scaleselect = true;
    this->scalemodifier = 4.0f;

    this->num_s_in = 1;
    this->num_s_out = 4;

    this->s_in[0].ctype = CABLE_RED;
    this->s_in[0].lpos = b2Vec2(-.0f, .0f);

    this->s_out[0].ctype = CABLE_RED;
    this->s_out[0].lpos = b2Vec2(.0f,  .25f);
    this->s_out[1].ctype = CABLE_RED;
    this->s_out[1].lpos = b2Vec2( .25f, .0f);
    this->s_out[2].ctype = CABLE_RED;
    this->s_out[2].lpos = b2Vec2(.0f, -.25f);
    this->s_out[3].ctype = CABLE_RED;
    this->s_out[3].lpos = b2Vec2(-.25f, .0f);

    this->set_as_rect(.375f, .375f);
}

i1o8gate::i1o8gate()
{
    this->set_mesh(mesh_factory::get_mesh(MODEL_I1O8));
    this->set_material(&m_iomisc);

    this->scaleselect = true;
    this->scalemodifier = 4.0f;

    this->num_s_in = 1;
    this->num_s_out = 8;

    this->s_in[0].ctype = CABLE_RED;
    this->s_in[0].lpos = b2Vec2(-.0f, .0f);

    this->s_out[0].ctype = CABLE_RED;
    this->s_out[0].lpos = b2Vec2( .0f,   .25f);
    this->s_out[1].ctype = CABLE_RED;
    this->s_out[1].lpos = b2Vec2( .25f,  .25f);
    this->s_out[2].ctype = CABLE_RED;
    this->s_out[2].lpos = b2Vec2( .25f,  .0f);
    this->s_out[3].ctype = CABLE_RED;
    this->s_out[3].lpos = b2Vec2( .25f, -.25f);
    this->s_out[4].ctype = CABLE_RED;
    this->s_out[4].lpos = b2Vec2( .0f,  -.25f);
    this->s_out[5].ctype = CABLE_RED;
    this->s_out[5].lpos = b2Vec2(-.25f, -.25f);
    this->s_out[6].ctype = CABLE_RED;
    this->s_out[6].lpos = b2Vec2(-.25f,  .0f);
    this->s_out[7].ctype = CABLE_RED;
    this->s_out[7].lpos = b2Vec2(-.25f,  .25f);

    this->set_as_rect(.375f, .375f);
}

i2o2gate::i2o2gate()
{
    this->set_mesh(mesh_factory::get_mesh(MODEL_I2O2));
    this->set_material(&m_iomisc);

    this->num_s_in  = 2;
    this->num_s_out = 2;

    this->s_in[0].ctype = CABLE_RED;
    this->s_in[0].lpos = b2Vec2(-.15f, -.12f);
    this->s_in[1].ctype = CABLE_RED;
    this->s_in[1].lpos = b2Vec2( .15f, -.12f);

    this->s_out[0].ctype = CABLE_RED;
    this->s_out[0].lpos = b2Vec2(-.15f, .12f);
    this->s_out[1].ctype = CABLE_RED;
    this->s_out[1].lpos = b2Vec2( .15f, .12f);

    this->set_as_rect(.375f, .25f);
}

i3o1gate::i3o1gate()
{
    this->set_mesh(mesh_factory::get_mesh(MODEL_I3O1));
    this->set_material(&m_iomisc);

    this->num_s_in  = 3;
    this->num_s_out = 1;

    this->s_in[0].ctype = CABLE_RED;
    this->s_in[0].lpos = b2Vec2(-.225f, 0.f);
    this->s_in[1].ctype = CABLE_RED;
    this->s_in[1].lpos = b2Vec2(0.f,  -0.000001f);
    this->s_in[2].ctype = CABLE_RED;
    this->s_in[2].lpos = b2Vec2(.25f, .0f);

    this->s_out[0].ctype = CABLE_RED;
    this->s_out[0].lpos = b2Vec2(-.15f, .12f);

    this->set_as_rect(.375f, .275f);
}

i4o1gate::i4o1gate()
{
    this->set_mesh(mesh_factory::get_mesh(MODEL_I4O1));
    this->set_material(&m_iomisc);
    this->scaleselect = true;
    this->scalemodifier = 4.f;

    this->num_s_in = 4;
    this->num_s_out = 1;

    this->s_out[0].ctype = CABLE_RED;
    this->s_out[0].lpos = b2Vec2(-.0f, 0.f);

    this->s_in[0].ctype = CABLE_RED;
    this->s_in[0].lpos = b2Vec2(.0f,  .25f);

    this->s_in[1].ctype = CABLE_RED;
    this->s_in[1].lpos = b2Vec2( .25f, .0f);

    this->s_in[2].ctype = CABLE_RED;
    this->s_in[2].lpos = b2Vec2(.0f, -.25f);

    this->s_in[3].ctype = CABLE_RED;
    this->s_in[3].lpos = b2Vec2(-.25f, .0f);

    this->set_as_rect(.375f, .375f);
}

edevice*
ifselect::solve_electronics()
{
    if (!this->s_in[0].is_ready()) {
        return this->s_in[0].get_connected_edevice();
    }
    if (!this->s_in[1].is_ready()) {
        return this->s_in[1].get_connected_edevice();
    }
    if (!this->s_in[2].is_ready()) {
        return this->s_in[2].get_connected_edevice();
    }

    float v1 = this->s_in[0].get_value();
    float v2 = this->s_in[1].get_value();

    bool conditional = (bool)(roundf(this->s_in[2].get_value()));

    this->s_out[0].write(conditional ? v2 : v1);

    return 0;
}
