#include "battery.hh"
#include "material.hh"
#include "model.hh"
#include "world.hh"

battery::battery(int what)
    : voltage(3.f)
{
    this->set_mesh(mesh_factory::get_mesh(MODEL_BATTERY3V));
    this->set_material(&m_battery);

    this->set_uniform("~color", .1f, .1f, .1f, 1.f);

    this->menu_scale = 1.f/1.5f;

    this->num_s_in = 0;
    this->num_s_out = 1;

    this->s_out[0].lpos = b2Vec2(.0,.465f);
    this->s_out[0].angle = M_PI/2.f;
    this->s_out[0].ctype = CABLE_BLACK;

    this->set_as_rect(.75f*.5f, 1.2f*.5f);
}

edevice*
battery::solve_electronics()
{
    if (W->level.version >= 25) {
        this->s_out[0].write(this->voltage*3.f);
    } else {
        this->s_out[0].write(this->voltage);
    }

    return 0;
}

