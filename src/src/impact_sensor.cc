#include "impact_sensor.hh"
#include "material.hh"
#include "model.hh"
#include "game.hh"

#include <Box2D/Box2D.h>

impact_sensor::impact_sensor(bool pressure)
    : impulse(0.f)
{
    this->pressure = pressure;
    this->num_sliders = 2;
    this->menu_scale = .25f;

    this->set_mesh(mesh_factory::get_mesh(MODEL_IMPACT3));
    this->set_material(&m_red);
    this->set_num_properties(2);
    this->properties[0].type = P_INT; /* size */
    this->properties[0].v.f = 3;

    this->properties[1].type = P_FLT; /* sensitivity */
    this->properties[1].v.f = 10.f;

    this->layer_mask = 15;

    this->num_s_out = 1;
    this->s_out[0].lpos = b2Vec2(.25f, 0.f);

    this->update_appearance();
}

void
impact_sensor::on_load(bool created, bool has_state)
{
    this->update_appearance();
}

void
impact_sensor::update_appearance()
{
    uint32_t size = this->properties[0].v.i;
    if (size > 3) size = 3;
    this->properties[0].v.i = size;

    this->set_mesh(mesh_factory::get_mesh(MODEL_IMPACT0+size));

    this->set_as_rect(((float)size+1.f)/2.f, .15f);

    if (this->body) {
        this->recreate_shape();
    }
}

double logslider(float v, double minv, double maxv)
{
    double minp = 0.f;
    double maxp = 1.f;

    minv = log(minv);
    maxv = log(maxv);

    // calculate adjustment factor
    double scale = (maxv-minv) / (maxp-minp);

    return exp(minv + scale*(v-minp));
}

float
impact_sensor::get_slider_snap(int s)
{
    switch (s) {
        case 0: return 1.f / 3.f;
        case 1: return 1.f / 20.f;
    }

    return 0.f;
}

float
impact_sensor::get_slider_value(int s)
{
    switch (s) {
        case 0: return this->properties[0].v.i / 3.f;
        case 1: return tmath_logstep_position(this->properties[1].v.f, pressure ? 0.25f : 2.5f, pressure ? 100 : 250);
    }

    return 0.f;
}

void
impact_sensor::on_slider_change(int s, float value)
{
    if (s == 0) {
        uint32_t size = (uint32_t)roundf(value * 3.f);
        G->animate_disconnect(this);
        this->disconnect_all();
        this->set_property(0, size);

        this->update_appearance();
        G->show_numfeed(1.f+size);
    } else if (s == 1) {
        double logval = tmath_logstep(value, pressure ? 0.25f : 2.5f, pressure ? 100 : 250);
        this->set_property(1, (float)logval);
        G->show_numfeed(logval);
    }
}

edevice*
impact_sensor::solve_electronics()
{
    float clamped = tclampf(this->impulse / this->properties[1].v.f, 0.f, 1.f);
    this->s_out[0].write(clamped);

    if (this->pressure) {
        if (this->get_body(0)->IsAwake()) {
            this->impulse *= 0.9f;
        }
    } else {
        this->impulse = 0.f;
    }

    return 0;
}
