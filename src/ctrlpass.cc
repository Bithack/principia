#include "ctrlbase.hh"
#include "model.hh"
#include "material.hh"
#include "ifdevice.hh"

ctrlpass::ctrlpass()
{
    this->menu_scale = .75f;

    this->num_s_in = 1;
    this->num_s_out = 1;
    this->scaleselect = true;

    this->s_out[0].ctype = CABLE_BLUE;
    this->s_out[0].lpos = b2Vec2(-.15f, .0f);
    this->s_out[0].angle = M_PI/2.f;

    this->s_in[0].ctype = CABLE_BLACK;
    this->s_in[0].lpos = b2Vec2(.3f, 0.f);
    this->s_in[0].angle = 0;

    this->set_mesh(mesh_factory::get_mesh(MODEL_CTRLPASS));
    this->set_material(&m_misc);

    this->set_as_rect(.950f*.5f, .313f*.5f);
}

edevice*
ctrlpass::solve_electronics(void)
{
    if (!this->s_out[0].p)
        return 0;

    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();

    float voltage = this->s_in[0].get_value();

    if (this->s_out[0].p->
        c->get_other((plug*)this->s_out[0].p)
        ->plugged_edev) {
        ifdevice *i = this->s_out[0].p->
            c->get_other((plug*)this->s_out[0].p)
            ->plugged_edev->get_ifdevice();

        i->ifstep(voltage, 1.f, 0.f, 0.f, false, false);
    }

    return 0;
}

