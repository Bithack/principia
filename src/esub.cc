#include "esub.hh"
#include "game.hh"

esub::esub()
{
    this->num_sliders = 1;

    this->set_num_properties(2);
    this->properties[0].type = P_FLT;
    this->properties[0].v.f = 0.f;
}

void
esub::write_quickinfo(char *out)
{
    sprintf(out, "%s (-%.5f)", this->get_name(), this->properties[0].v.f);
}

edevice*
esub::solve_electronics()
{
    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();

    float v = this->s_in[0].get_value();
    this->s_out[0].write(tclampf(v - this->properties[0].v.f, 0.f, 1.f));

    return 0;
}

void
esub::on_slider_change(int s, float value)
{
    this->properties[s].v.f = value;
    G->show_numfeed(value);
}
