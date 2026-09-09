#include "condenser.hh"
#include "game.hh"

condenser::condenser() : value(0.f) {
    this->s_in[0].tag = SOCK_TAG_INCREASE;
    this->s_in[1].tag = SOCK_TAG_DECREASE;
    this->s_out[0].tag = SOCK_TAG_FRACTION;

    this->set_num_properties(2);

    this->num_sliders = 2;

    this->properties[0].type = P_FLT;
    this->properties[0].v.f = 5.f; /* max value */

    this->properties[1].type = P_FLT;
    this->properties[1].v.f = 0.f; /* initial value */
}

void condenser::setup() {
    this->value = this->properties[1].v.f * this->properties[0].v.f;
}

float condenser::get_slider_value(int s) {
    if (s == 0)
        return (this->properties[0].v.f - 1.f) / 31.f;
    else // s == 1
        return this->properties[1].v.f;
}

void condenser::on_slider_change(int s, float value) {
    if (s == 0) {
        float v = 1.f + (value * 31.f);
        this->properties[0].v.f = v;
        G->show_numfeed(v);
    } else { // s == 1
        this->properties[1].v.f = value;
        G->show_numfeed(value);
    }
}

edevice *condenser::solve_electronics() {
    if (!this->s_out[0].written()) {
        float v = this->value / this->properties[0].v.f;
        this->s_out[0].write(v);
    }

    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();
    if (!this->s_in[1].is_ready())
        return this->s_in[1].get_connected_edevice();

    float a = this->s_in[0].get_value();
    float b = this->s_in[1].get_value();

    this->value = tclampf(this->value + a - b, 0.f, this->properties[0].v.f);

    return 0;
}

edevice *wrapcondenser::solve_electronics() {
    if (!this->s_out[0].written()) {
        float v = this->value / this->properties[0].v.f;
        if (v > 0.999999) {
            v = 0.f;
            this->value = 0.f;
        }
        this->s_out[0].write(v);
    }

    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();
    if (!this->s_in[1].is_ready())
        return this->s_in[1].get_connected_edevice();

    float a = this->s_in[0].get_value();
    float b = this->s_in[1].get_value();

    this->value = fmodf(this->value + a + (this->properties[0].v.f - b), this->properties[0].v.f);

    return 0;
}
