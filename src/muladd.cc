#include "muladd.hh"
#include "game.hh"

muladd::muladd()
{
    this->num_sliders = 2;

    this->set_num_properties(2);
    this->properties[0].type = P_FLT;
    this->properties[0].v.f = 1.f;
    this->properties[1].type = P_FLT;
    this->properties[1].v.f = 0.f;
}

edevice*
muladd::solve_electronics()
{
    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();

    float v = this->s_in[0].get_value();
    this->s_out[0].write(
            tclampf(v * this->properties[0].v.f + this->properties[1].v.f, 0.f, 1.f)
            );

    return 0;

}

void
muladd::on_slider_change(int s, float value)
{
    float v = value;
    if (s == 0) { /* mul */
        v *= 2;
    }
    this->properties[s].v.f = v;
    G->show_numfeed(v);
}
