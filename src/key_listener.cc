#include "key_listener.hh"
#include "ui.hh"

key_listener::key_listener()
{
    this->set_flag(ENTITY_HAS_CONFIG, true);

    this->dialog_id = DIALOG_KEY_LISTENER;

    this->set_num_properties(1);
    this->properties[0].type = P_INT; /* key to listen to */
    this->properties[0].v.i = TMS_KEY_SPACE;

    this->active = false;
}

void
key_listener::setup()
{
    this->active = false;
}

edevice*
key_listener::solve_electronics()
{
    this->s_out[0].write(this->active ? 1.f : 0.f);

    return 0;
}
