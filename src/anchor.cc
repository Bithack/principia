#include "anchor.hh"
#include "material.hh"
#include "model.hh"
#include "game.hh"
#include "faction.hh"
#include "ui.hh"

anchor::anchor(int anchor_type)
    : active(true)
{
    this->anchor_type = anchor_type;

    this->set_flag(ENTITY_HAS_CONFIG, true);

    this->dialog_id = DIALOG_SET_FACTION;

    this->set_mesh(mesh_factory::get_mesh(MODEL_GUARDPOINT));
    this->set_material(&m_pv_colored);

    this->set_num_properties(1);
    this->properties[0].type = P_INT8; // faction id
    this->properties[0].v.i8 = FACTION_FRIENDLY;
    this->set_faction(FACTION_FRIENDLY);

    this->num_s_in = 1;
    this->num_s_out = 0;

    this->set_as_rect(.22f, .7f);
    this->s_in[0].lpos = b2Vec2(0.f, -.56f);
}

void
anchor::on_load(bool created, bool has_state)
{
    this->set_faction(this->properties[0].v.i8);
}

faction_info*
anchor::set_faction(uint8_t faction_id)
{
    if (faction_id > NUM_FACTIONS) return 0;

    return this->set_faction(&factions[faction_id]);
}

faction_info*
anchor::set_faction(faction_info *faction)
{
    this->properties[0].v.i8 = faction->id;
    this->faction = faction;

    this->set_uniform("~color", this->faction->color.r, this->faction->color.g, this->faction->color.b, 1.f);

    return faction;
}

edevice*
anchor::solve_electronics()
{
    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();

    if (this->s_in[0].p == 0) {
        this->active = true;
    } else {
        this->active = (bool)((int)roundf(this->s_in[0].get_value()));
    }

    return 0;
}
