#include "clamp.hh"
#include "game.hh"

clamp::clamp()
    : prev_value(0.f)
{
    this->num_sliders = 2;

    this->set_num_properties(2);
    this->properties[0].type = P_FLT;
    this->properties[0].v.f = 0.f;
    this->properties[1].type = P_FLT;
    this->properties[1].v.f = 1.f;
}

edevice*
clamp::solve_electronics()
{
    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();

    float v = this->s_in[0].get_value();

    /* If the maximum value is lower than the minimum value, invert the clamp */
    if (this->properties[1].v.f < this->properties[0].v.f) {
        if (v >= this->properties[1].v.f && v <= this->properties[0].v.f)
            v = this->prev_value;

        this->s_out[0].write(v);
        this->prev_value = v;
    } else {
        this->s_out[0].write(
                tclampf(v, this->properties[0].v.f, this->properties[1].v.f)
                );
    }

    return 0;

}

void
clamp::on_slider_change(int s, float value)
{
    this->properties[s].v.f = value;
    G->show_numfeed(value);
}
