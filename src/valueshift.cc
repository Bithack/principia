#include "valueshift.hh"
#include "game.hh"

valueshift::valueshift()
{
    this->set_mesh(mesh_factory::get_mesh(MODEL_I1O1_EMPTY));
    this->num_sliders = 1;
    this->set_num_properties(1);
    this->set_property(0, 0.5f);
}

void
valueshift::on_slider_change(int s, float value)
{
    this->properties[s].v.f = value;
    G->show_numfeed(value);
};

edevice*
valueshift::solve_electronics()
{
    if (!this->s_in[0].is_ready())
        this->s_in[0].get_connected_edevice();

    float v = this->s_in[0].get_value();
    v = fmodf(v + this->properties[0].v.f, 1.f);

    this->s_out[0].write(v);

    return 0;
}
