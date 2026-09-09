#include "hp_control.hh"
#include "game.hh"
#include "robot_base.hh"

hp_control::hp_control() : target(0) {
    this->set_num_properties(1);
    this->properties[0].type = P_INT;
    this->set_flag(ENTITY_HAS_TRACKER, true);
    this->properties[0].v.i = 0;
}

static void on_target_absorbed(entity *self, void *userdata) {
    hp_control *hc = static_cast<hp_control*>(self);
    hc->unsubscribe(hc->target);
    hc->target = 0;
}

void hp_control::setup() {
    this->target = 0;

    if (this->properties[0].v.i != 0) {
        entity *e = W->get_entity_by_id(this->properties[0].v.i);
        if (e && e->flag_active(ENTITY_IS_ROBOT)) {
            this->target = static_cast<robot_base*>(e);
            this->subscribe(this->target, ENTITY_EVENT_REMOVE, &on_target_absorbed);
        }
    }
}

void hp_control::restore() {
    entity::restore();

    this->target = 0;

    if (this->properties[0].v.i != 0) {
        entity *e = W->get_entity_by_id(this->properties[0].v.i);
        if (e && e->flag_active(ENTITY_IS_ROBOT)) {
            this->target = static_cast<robot_base*>(e);
            this->subscribe(this->target, ENTITY_EVENT_REMOVE, &on_target_absorbed);
        }
    }
}

edevice *hp_control::solve_electronics() {
    bool set = false;

    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();
    if (!this->s_in[1].is_ready())
        return this->s_in[1].get_connected_edevice();

    if (this->target) {
        set = (bool)((int)roundf(this->s_in[0].get_value()));
        float hp = this->target->get_hp();

        if (set) {
            float new_hp = this->target->get_max_hp() * this->s_in[1].get_value();
            this->target->set_hp(new_hp);

            if (new_hp != hp)
                G->add_hp(this->target, new_hp / this->target->get_max_hp());
        }

        this->s_out[0].write(tclampf(hp / this->target->get_max_hp(), 0.f, 1.f));
    } else
        this->s_out[0].write(0.f);

    return 0;
}
