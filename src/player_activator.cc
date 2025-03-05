#include "player_activator.hh"
#include "adventure.hh"
#include "game.hh"
#include "world.hh"

player_activator::player_activator()
{
    this->set_flag(ENTITY_HAS_TRACKER, true);

    this->set_num_properties(1);
    this->properties[0].type = P_ID;
    this->properties[0].v.i = 0;
}

edevice*
player_activator::solve_electronics()
{
    entity *e = W->get_entity_by_id(this->properties[0].v.i);

    float v = 0.f;

    if (!this->s_out[0].written()) {
        this->s_out[0].write((G->state.adventure_id == this->properties[0].v.i ? 1.f : 0.f));
    }

    if (!this->s_in[0].is_ready()) {
        return this->s_in[0].get_connected_edevice();
    }

    if (!this->s_in[0].p || (bool)roundf(this->s_in[0].get_value())) {
        if (W->level.type == LCAT_ADVENTURE) {
            /* If we have received an input from an PA which was not just activated. */
            if (adventure::player != e) {

                /* If the target exists and is a creature */
                if (e && e->is_creature()) {
                    /* By default, we will also set the RC to the player. */
                    bool set_rc = true;

                    /* However, if there is a current panel, and that panel is an RC,
                     * we will not set the RC to the new player. */
                    if (G->current_panel && G->current_panel->is_control_panel()) {
                        set_rc = false;
                    }

                    adventure::set_player(static_cast<creature*>(e), true, set_rc);
                } else {
                    /* We either received a null-target or a target which is not a creature.
                     * Either way, unset the current player */
                    adventure::set_player(0);
                }
            }
        }
    }

    return 0;
}
