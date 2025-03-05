#include "mini_transmitter.hh"
#include "receiver.hh"
#include "material.hh"
#include "model.hh"
#include "world.hh"
#include "game.hh"
#include "ui.hh"

mini_transmitter::mini_transmitter()
    : wireless_plug(CABLE_OUT)
{
    this->set_flag(ENTITY_IS_HIGH_PRIO,         true);
    this->set_flag(ENTITY_ALLOW_CONNECTIONS,    false);
    this->set_flag(ENTITY_ALLOW_ROTATION,       false);
    this->set_flag(ENTITY_HAS_CONFIG,           true);
    this->set_flag(ENTITY_DO_UPDATE_EFFECTS,    true);

    this->dialog_id = DIALOG_SET_FREQUENCY;

    this->plug_type = PLUG_MINI_TRANSMITTER;
    this->type = ENTITY_EDEVICE;
    this->do_solve_electronics = true;

    this->s = 0;
    this->plugged_edev = 0;

    this->set_mesh(mesh_factory::get_mesh(MODEL_PLUG_TRANSMITTER));
    this->set_material(&m_pv_colored);

    this->set_num_properties(3);
    this->properties[0].type = P_INT; /* Transmit frequency */
    this->properties[0].v.i = 1;
    this->properties[1].type = P_ID; /* Connected ID */
    this->properties[1].v.i = 0;
    this->properties[2].type = P_INT; /* Socket ID */
    this->properties[2].v.i = 0;

    this->update_color();

    this->pending_value = 0.f;
}

void
mini_transmitter::setup()
{
    wplug::setup();

    this->pending_value = 0.f;

    this->reconnect();
}

void
mini_transmitter::on_pause()
{
    wplug::on_pause();

    this->pending_value = 0.f;

    this->reconnect();
}

edevice*
mini_transmitter::solve_electronics()
{
    if (this->s) {
        uint32_t freq = this->properties[0].v.i;

        if (this->pending_value > 0.f) {
            std::pair<std::multimap<uint32_t, receiver_base*>::iterator, std::multimap<uint32_t, receiver_base*>::iterator> range = W->receivers.equal_range(freq);
            for (std::multimap<uint32_t, receiver_base*>::iterator
                    i = range.first;
                    i != range.second && i != W->receivers.end();
                    i++) {
                i->second->pending_value = this->pending_value;
                i->second->no_broadcast = true;
            }
        }
    }

    this->update_color();
    this->pending_value = 0.f;

    return 0;
}
