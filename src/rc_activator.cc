#include "rc_activator.hh"
#include "game.hh"
#include "adventure.hh"

rcactivator::rcactivator()
{
    this->set_flag(ENTITY_HAS_TRACKER, true);

    this->set_num_properties(1);
    this->properties[0].type = P_ID;
    this->properties[0].v.i = 0;
}

edevice*
rcactivator::solve_electronics()
{
    entity *e;
    uint32_t entity_id = this->properties[0].v.i;

    if (entity_id != 0 && !(e = W->get_entity_by_id(entity_id))) {
        goto invalid;
    }

    if (!this->s_out[0].written()) {
        if (entity_id == 0) {
            this->s_out[0].write(0.f);
        } else {
            this->s_out[0].write((G->current_panel == e ? 1.f : 0.f));
        }
    }

    if (!this->s_in[0].is_ready()) {
       return this->s_in[0].get_connected_edevice();
    }

    if (entity_id == 0 || entity_id == this->id) {
        if ((bool)roundf(this->s_in[0].get_value())) {
            entity *target = 0;
            if (W->is_adventure() && adventure::player) {
                target = adventure::player;
                adventure::player->detach();
            }

            G->set_control_panel(target);
        }
    } else if (!this->s_in[0].p || (bool)roundf(this->s_in[0].get_value())) {
        if (G->current_panel != e) {
            G->set_control_panel(e);
        }
    }

    return 0;

invalid:
    this->s_out[0].write(0);
    if (!this->s_in[0].p || (bool)roundf(this->s_in[0].get_value())) {
        G->add_error(this, ERROR_RC_ACTIVATOR_INVALID);
    }
    return 0;
}
