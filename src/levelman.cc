#include "levelman.hh"
#include "model.hh"
#include "game.hh"
#include "main.hh"

levelman::levelman()
{
    this->set_mesh(mesh_factory::get_mesh(MODEL_I4O0));
    this->set_material(&m_iomisc);

    this->scaleselect = true;
    this->scalemodifier = 4.f;

    this->num_s_in = 4;
    this->num_s_out = 0;

    this->s_in[0].ctype = CABLE_RED;
    this->s_in[0].lpos = b2Vec2(-.15f, .12f);

    this->s_in[1].ctype = CABLE_RED;
    this->s_in[1].lpos = b2Vec2(.15f, .12f);

    this->s_in[2].ctype = CABLE_RED;
    this->s_in[2].lpos = b2Vec2(-.15f, -.12f);

    this->s_in[3].ctype = CABLE_RED;
    this->s_in[3].lpos = b2Vec2(.15f, -.12f);

    this->set_as_rect(.375f, .3f);
}

edevice*
levelman::solve_electronics()
{
    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();
    if (!this->s_in[1].is_ready())
        return this->s_in[1].get_connected_edevice();

    if ((bool)(roundf(this->s_in[0].get_value()))) {
        G->tmp_ambientdiffuse.x = P.default_ambient + (this->s_in[1].get_value() * 2.f) - 1.f;
    }

    if ((bool)(roundf(this->s_in[2].get_value()))) {
        G->tmp_ambientdiffuse.y = P.default_diffuse + (this->s_in[3].get_value() * 2.f) - 1.f;
    }

    return 0;
}
