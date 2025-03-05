#include "jumper.hh"
#include "material.hh"
#include "model.hh"
#include "world.hh"
#include "game.hh"
#include "ui.hh"

jumper::jumper()
    : wplug(CABLE_IN)
{
    this->set_flag(ENTITY_IS_HIGH_PRIO,         true);
    this->set_flag(ENTITY_ALLOW_CONNECTIONS,    false);
    this->set_flag(ENTITY_ALLOW_ROTATION,       false);
    this->set_flag(ENTITY_HAS_CONFIG,           true);

    this->dialog_id = DIALOG_JUMPER;

    this->plug_type = PLUG_JUMPER;
    this->type = ENTITY_EDEVICE;
    this->num_sliders = 1;
    this->do_solve_electronics = true;
    this->s = 0;
    this->plugged_edev = 0;

    this->set_mesh(mesh_factory::get_mesh(MODEL_PLUG_MALE));
    this->set_material(&m_pv_colored);

    this->set_num_properties(3);
    this->properties[0].type = P_FLT;
    this->properties[0].v.f = 1.f;
    this->properties[1].type = P_ID;
    this->properties[1].v.i = 0;
    this->properties[2].type = P_INT;
    this->properties[2].v.i = 0;

    tmat4_load_identity(this->M);
    tmat3_load_identity(this->N);
}

void
jumper::setup()
{
    wplug::setup();

    this->reconnect();
}

void
jumper::on_pause()
{
    wplug::on_pause();

    this->reconnect();
}

edevice*
jumper::solve_electronics()
{
    if (this->s) {
        socket_in *si = static_cast<socket_in*>(s);
        si->step_count = edev_step_count;
        si->value = this->properties[0].v.f;
    }

    return 0;
}

float
jumper::get_slider_snap(int s)
{
    return 0.05f;
}

float
jumper::get_slider_value(int s)
{
    return this->properties[0].v.f;
}

void
jumper::on_slider_change(int s, float value)
{
    this->set_property(0, value);
    G->show_numfeed(value);
    this->update_color();
}

void
jumper::write_quickinfo(char *out)
{
    sprintf(out, "%s (%.5f)", this->get_name(), this->properties[0].v.f);
}
