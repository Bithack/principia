#include "pkgwarp.hh"
#include "model.hh"
#include "material.hh"
#include "game.hh"
#include "pkgman.hh"
#include "ui.hh"

pkgwarp::pkgwarp()
{
    this->set_flag(ENTITY_HAS_CONFIG, true);

    this->dialog_id = DIALOG_SET_PKG_LEVEL;

    this->set_num_properties(1);
    this->properties[0].type = P_INT8;
    this->properties[0].v.i8 = 1;
}

edevice*
pkgwarp::solve_electronics()
{
    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();

    bool warp = (bool)(int)roundf(this->s_in[0].get_value());

    if (warp) {
        tms_infof("Attempting to warp to %d", this->properties[0].v.i8);
        pkginfo *p = G->state.pkg;

        if (p) {
            uint32_t lvl = p->get_level_by_index(this->properties[0].v.i8);

            if (lvl != 0 && lvl != W->level.local_id) {
                G->state.ending = true;
                G->state.end_action = GAME_END_WARP;
                G->state.end_warp = lvl;
            }
        }
    }

    return 0;
}

pkgstatus::pkgstatus()
{
    this->set_flag(ENTITY_HAS_CONFIG, true);

    this->dialog_id = DIALOG_SET_PKG_LEVEL;

    this->cached_percent = 0.f;
    this->cached_lock = 0.f;
    this->scaleselect = true;
    this->scalemodifier = 6.5f;

    this->s_out[0].set_description("Percent completed. 1.0 if the level has been completed, otherwise top_score/final_score or 0.0");
    this->s_out[1].set_description("Unlock status. Returns 1.0 if the level is unlocked according to the package unlock count.");
    this->s_out[2].set_description("1.0 if this level was the last played level before the current one.");

    this->set_num_properties(1);
    this->properties[0].type = P_INT8;
    this->properties[0].v.i8 = 1;
}

void
pkgstatus::init()
{
    if (G->state.pkg) {
        lvl_progress *p = progress::get_level_progress(G->state.pkg->type, G->state.pkg->get_level_by_index(this->properties[0].v.i8));
        if (p->completed)
            this->cached_percent = 1.f;
        else
            this->cached_percent = 0.f;
    } else {
        this->cached_percent = 0.f;
    }

    if (G->state.pkg) {
        this->cached_lock = (float)G->state.pkg->is_level_locked(this->properties[0].v.i8);
    } else {
        this->cached_lock = 1.f;
    }
}

edevice*
pkgstatus::solve_electronics()
{
    this->s_out[0].write(this->cached_percent);
    this->s_out[1].write(this->cached_lock);
    this->s_out[2].write((this->properties[0].v.i8 == G->previous_level)?1.f:0.f);

    return 0;
}
