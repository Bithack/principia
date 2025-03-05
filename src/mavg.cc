#include "mavg.hh"
#include "ledbuffer.hh"

mavg::mavg()
    : value(0.f)
{
    this->num_sliders = 1;
    this->menu_scale = 1.0f;
    this->set_flag(ENTITY_DO_UPDATE_EFFECTS, true);

    this->set_num_properties(1);
    this->properties[0].type = P_INT;
    this->properties[0].v.f = 1.f - (.5f / 8.f);
}

edevice*
mavg::solve_electronics()
{
    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();

    float v = this->s_in[0].get_value();

    float f = this->properties[0].v.f;
    this->value = f * this->value + (1.f - f)*v;
    this->s_out[0].write(tclampf(this->value, 0.f, 1.f));

    return 0;
}

void
mavg::update_effects(void)
{
    float z = this->get_layer() * LAYER_DEPTH + LED_Z_OFFSET;
    b2Vec2 p = this->get_position();
    ledbuffer::add(p.x, p.y, z, this->value);
}
