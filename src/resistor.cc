#include "resistor.hh"
#include "game.hh"

resistor::resistor()
{
    this->set_mesh(mesh_factory::get_mesh(MODEL_I2O1_EMPTY));

    this->s_in[0].ctype = CABLE_BLACK;
    this->s_out[0].ctype = CABLE_BLACK;

    this->s_in[1].tag = SOCK_TAG_MULTIPLIER;

    if (W->level.version < LEVEL_VERSION_1_2_3) {
        this->set_as_rect(.15f, .375f);
    }
}

edevice*
resistor::solve_electronics()
{
    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();
    if (!this->s_in[1].is_ready())
        return this->s_in[1].get_connected_edevice();

    float resistance = 1.f - this->s_in[1].get_value();

    if (this->s_in[1].p == 0) {
        /* cable is disconnected, default resistance = 0 */
        resistance = 0.f;
    }

    float v = this->s_in[0].get_value() * resistance;

    this->s_out[0].write(v);

    return 0;
}
