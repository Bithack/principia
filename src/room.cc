#include "room.hh"
#include "game.hh"
#include "model.hh"

room::room() {
    this->set_flag(ENTITY_IS_DEV, true);
    this->set_flag(ENTITY_ALLOW_CONNECTIONS, true);
    this->set_flag(ENTITY_IS_LOW_PRIO, true);
    this->layer_mask = 1;
    this->num_sliders = 2;

    this->set_num_properties(2);
    this->properties[0].type = P_INT;
    this->properties[0].v.i = 1;
    this->properties[1].type = P_INT;
    this->properties[1].v.i = 1;

    this->set_mesh(mesh_factory::get_mesh(MODEL_ROOM_BG));
    this->set_material(&m_room);
    this->set_uniform("size", 2.f, 2.f, 0.f, 0.f);
    this->set_uniform("ao_mask2", 1.f, 0.f, 0.f, 0.f);

    this->set_as_rect(2.48f/2.f, 2.48f/2.f);
}

void room::set_layer(int z) {
    switch (z) {
        case 0: this->set_uniform("ao_mask2", 1.f, 0.f, 0.f, 0.f); break;
        case 1: this->set_uniform("ao_mask2", 0.f, 1.f, 0.f, 0.f); break;
        case 2: this->set_uniform("ao_mask2", 0.f, 0.f, 1.f, 0.f); break;
    }
    entity::set_layer(z);
}

void room::on_slider_change(int s, float value) {
    if (s == 0) {
        uint32_t num_corners = (uint32_t)roundf(value * 4.f);
        G->animate_disconnect(this);
        this->disconnect_all();
        this->set_property(0, num_corners);
    } else {
        this->properties[1].v.i = roundf(value);
    }
}

void room::create_sensor() {
    /* abuse of the create_sensor function, we actually force the fixture not to collide with dynamic objects */
    b2Filter d = world::get_filter_for_layer(this->get_layer(), 1);
    d.groupIndex = 1+this->get_layer();
    this->fx->SetFilterData(d);
}
