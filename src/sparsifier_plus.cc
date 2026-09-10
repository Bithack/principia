#include "sparsifier_plus.hh"
#include "model.hh"

besserwisser::besserwisser() : last(false) {
    this->set_mesh(mesh_factory::get_mesh(MODEL_I1O1_BESSERWISSER));
}

edevice *besserwisser::solve_electronics() {
    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();

    bool v = (bool)((int)roundf(this->s_in[0].get_value()));

    this->s_out[0].write((v != last) ? 1.f : 0.f);

    last = v;

    return 0;
}

void besserwisser::write_state(lvlinfo *lvl, lvlbuf *lb) {
    entity::write_state(lvl,lb);
    lb->ensure(sizeof(uint8_t));
    lb->w_uint8(this->last);
}

void besserwisser::read_state(lvlinfo *lvl, lvlbuf *lb) {
    entity::read_state(lvl,lb);
    this->last = lb->r_uint8();
}
