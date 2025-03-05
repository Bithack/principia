#include "material.hh"
#include "model.hh"
#include "world.hh"
#include "game.hh"
#include "receiver.hh"
#include "ui.hh"

receiver::receiver()
    : wireless_plug(CABLE_IN)
{
    this->set_flag(ENTITY_IS_HIGH_PRIO,         true);
    this->set_flag(ENTITY_ALLOW_CONNECTIONS,    false);
    this->set_flag(ENTITY_ALLOW_ROTATION,       false);
    this->set_flag(ENTITY_HAS_CONFIG,           true);
    this->set_flag(ENTITY_DO_UPDATE_EFFECTS,    true);

    this->dialog_id = DIALOG_SET_FREQUENCY;

    this->plug_type = PLUG_JUMPER;
    this->type = ENTITY_EDEVICE;
    this->do_solve_electronics = true;

    this->s = 0;
    this->plugged_edev = 0;

    this->set_mesh(mesh_factory::get_mesh(MODEL_PLUG_MALE));
    this->set_material(&m_pv_colored);

    this->set_num_properties(3);
    this->properties[0].type = P_INT; /* Receiving frequency */
    this->properties[0].v.i = 1;
    this->properties[1].type = P_ID;
    this->properties[1].v.i = 0;
    this->properties[2].type = P_INT;
    this->properties[2].v.i = 0;

    this->update_color();

    this->pending_value = 0.f;
}

void
receiver::remove_from_world()
{
    entity::remove_from_world();

    W->remove_receiver(this->properties[0].v.i, this);
}

void
receiver::setup()
{
    wplug::setup();

    W->add_receiver(this->properties[0].v.i, this);
    this->reset_recv_value();

    this->reconnect();
}

void
receiver::restore()
{
    entity::restore();

    W->add_receiver(this->properties[0].v.i, this);
    this->reconnect();
}

void
receiver::on_pause()
{
    wplug::on_pause();

    this->reset_recv_value();

    this->reconnect();
}

edevice*
receiver::solve_electronics()
{
    if (this->s) {
        socket_in *si = static_cast<socket_in*>(s);
        si->step_count = edev_step_count;
        si->value = this->pending_value;
    }

    this->update_color();
    this->reset_recv_value();

    return 0;
}
