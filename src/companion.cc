#include "companion.hh"
#include "model.hh"
#include "faction.hh"

companion::companion()
{
    this->set_flag(ENTITY_IS_INTERACTIVE, true);
    this->robot_type = ROBOT_TYPE_COMPANION;
    this->damage_multiplier = 0.5f;

    this->set_mesh(mesh_factory::get_mesh(MODEL_BOT));
    this->set_material(&m_item);

    this->properties[0].v.f = ROBOT_DEFAULT_SPEED; /* Robot speed */
    this->properties[1].v.i8 = CREATURE_IDLE; /* Robot state */
    this->properties[6].v.i8 = FACTION_NEUTRAL;
    //this->properties[2].v.i8 = 0; /* Roaming */
}

void
companion::roam_update_dir()
{
    b2Vec2 target_pos = this->roam_target->get_position();

    if (this->target_dist > 2.f) {
        /* move toward the target */
        if (this->get_tangent_distance(target_pos) < 0.f)
            new_dir = DIR_LEFT;
        else
            new_dir = DIR_RIGHT;
    }
}
