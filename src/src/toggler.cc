#include "toggler.hh"
#include "game.hh"
#include "ledbuffer.hh"
#include "world.hh"

toggler::toggler()
    : value(false)
{
    this->menu_scale = 1.0f;

    if (W->level.version < LEVEL_VERSION_1_1_6) {
        this->set_as_rect(.15f, .375f);
    }

    this->set_flag(ENTITY_DO_UPDATE_EFFECTS, true);
    this->set_num_properties(1);
    this->properties[0].type = P_INT8;
    this->properties[0].v.i8 = 0;

    this->num_sliders = 1;
}

void
toggler::on_slider_change(int s, float v)
{
    this->properties[0].v.i8 = (int)roundf(v);
    this->value = (this->properties[0].v.i8 > 0);
    G->show_numfeed(v);
}

float
toggler::get_slider_value(int s)
{
    return (this->properties[0].v.i8 > 0 ? 1.f : 0.f);
}

void
toggler::setup()
{
    this->value = (this->properties[0].v.i8 > 0);
}

void
toggler::on_pause()
{
    this->value = (this->properties[0].v.i8 > 0);
    this->setup();
}

void
toggler::update_effects(void)
{
    float z = this->get_layer() * LAYER_DEPTH + LED_Z_OFFSET;
    b2Vec2 p = this->get_position();
    ledbuffer::add(p.x, p.y, z, this->value?1.f:0.f);
}

edevice*
toggler::solve_electronics()
{
    if (!this->s_in[0].is_ready())
        this->s_in[0].get_connected_edevice();

    float v = this->s_in[0].get_value();

    if ((bool)roundf(v))
        this->value = !this->value;

    this->s_out[0].write(this->value ? 1.f : 0.f);

    return 0;
}
