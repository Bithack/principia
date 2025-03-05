#include "gyroscope.hh"
#include "model.hh"
#include "material.hh"

gyroscope::gyroscope()
{
    this->set_mesh(mesh_factory::get_mesh(MODEL_GYROSCOPE));
    this->set_material(&m_metal);

    this->num_s_out = 1;
    this->num_s_in = 0;

    this->s_out[0].lpos = b2Vec2(0.f,-.25f);
    this->s_out[0].ctype = CABLE_RED;

    this->set_as_rect(.75f/2.f, .5f);
}

edevice*
gyroscope::solve_electronics()
{
    double b = (double)this->get_angle() + M_PI/2.;
    b = fmod(b, M_PI*2.);
    if (b < 0.) b += M_PI*2.;
    b /= M_PI * 2.;

    this->s_out[0].write((float)b);
    return 0;
}
