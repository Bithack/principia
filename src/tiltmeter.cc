#include "tiltmeter.hh"
#include "model.hh"
#include "material.hh"
#include "game.hh"

tiltmeter::tiltmeter()
{
    this->set_mesh(mesh_factory::get_mesh(MODEL_TILTMETER));
    this->set_material(&m_metal);

    this->num_s_out = 2;
    this->num_s_in = 0;

    this->num_sliders = 1;

    this->s_out[0].lpos = b2Vec2(-.125f,-.25f);
    this->s_out[0].ctype = CABLE_RED;

    this->s_out[1].lpos = b2Vec2(.125f,-.25f);
    this->s_out[1].ctype = CABLE_RED;

    this->set_num_properties(1);
    this->properties[0].type = P_FLT;
    this->properties[0].v.f = 8.f;

    this->set_as_rect(.375f, .5f);
}

edevice*
tiltmeter::solve_electronics()
{
    float a = (this->get_angle()/(M_PI*2.f)) * this->properties[0].v.f;

    if (a < 0.f) {
        a = -a;
        if (a > 1.f) a = 1.f;
        this->s_out[0].write(0.f);
        this->s_out[1].write(a);
    } else {
        if (a > 1.f) a = 1.f;
        this->s_out[0].write(a);
        this->s_out[1].write(0.f);
    }

    return 0;
}

float
tiltmeter::get_slider_value(int s)
{
    return (this->properties[0].v.f - 1.f)/20.f;
}

float
tiltmeter::get_slider_snap(int s)
{
    return .05f;
}

const char*
tiltmeter::get_slider_label(int s)
{
    return "Sensitivity";
}

void
tiltmeter::on_slider_change(int s, float value)
{
    float v = 1.f + value * 20.f;
    this->properties[0].v.f = v;
    G->show_numfeed(v);
}
