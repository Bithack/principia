#include "i2o1gate.hh"
#include "game.hh"
#include "material.hh"
#include "model.hh"

i2o1gate::i2o1gate() {
    this->set_material(&m_i2o1);

    tmat4_load_identity(this->M);
    tmat3_load_identity(this->N);

    this->num_s_in = 2;
    this->num_s_out = 1;

    this->menu_scale = 1.5f;

    this->s_in[0].lpos  = b2Vec2(-.15f, -.25f);
    this->s_in[1].lpos  = b2Vec2(.15f, -.25f);
    this->s_out[0].lpos = b2Vec2(0.f, .25f);

    this->set_as_rect(.25f, .375f);
}

i2o1gate_empty::i2o1gate_empty() {
    this->set_mesh(mesh_factory::get_mesh(MODEL_I2O1_EMPTY));
}

xorgate::xorgate() {
    this->set_mesh(mesh_factory::get_mesh(MODEL_I2O1_XOR));
    this->s_in[0].tag = SOCK_TAG_GENERIC_BOOL;
    this->s_in[1].tag = SOCK_TAG_GENERIC_BOOL;
    this->s_out[0].tag = SOCK_TAG_GENERIC_BOOL;
}

edevice *xorgate::solve_electronics() {
    bool v1 = false;
    bool v2 = false;

    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();
    if (!this->s_in[1].is_ready())
        return this->s_in[1].get_connected_edevice();

    v1 = (bool)((int)roundf(this->s_in[0].get_value()));
    v2 = (bool)((int)roundf(this->s_in[1].get_value()));

    this->s_out[0].write((v1 != v2) ? 1.f : 0.f);

    return 0;
}

orgate::orgate() {
    this->set_mesh(mesh_factory::get_mesh(MODEL_I2O1_OR));
    this->s_in[0].tag = SOCK_TAG_GENERIC_BOOL;
    this->s_in[1].tag = SOCK_TAG_GENERIC_BOOL;
    this->s_out[0].tag = SOCK_TAG_GENERIC_BOOL;
}

edevice *orgate::solve_electronics() {
    bool v1 = false;
    bool v2 = false;

    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();
    if (!this->s_in[1].is_ready())
        return this->s_in[1].get_connected_edevice();

    v1 = (bool)((int)roundf(this->s_in[0].get_value()));
    v2 = (bool)((int)roundf(this->s_in[1].get_value()));

    this->s_out[0].write((v1 || v2) ? 1.f : 0.f);

    return 0;
}

andgate::andgate() {
    this->set_mesh(mesh_factory::get_mesh(MODEL_I2O1_AND));
    this->s_in[0].tag = SOCK_TAG_GENERIC_BOOL;
    this->s_in[1].tag = SOCK_TAG_GENERIC_BOOL;
    this->s_out[0].tag = SOCK_TAG_GENERIC_BOOL;
}

edevice *andgate::solve_electronics() {
    bool v1 = false;
    bool v2 = false;

    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();
    if (!this->s_in[1].is_ready())
        return this->s_in[1].get_connected_edevice();

    v1 = (bool)((int)roundf(this->s_in[0].get_value()));
    v2 = (bool)((int)roundf(this->s_in[1].get_value()));

    this->s_out[0].write((v1 && v2) ? 1.f : 0.f);

    return 0;
}

nandgate::nandgate() {
    this->set_mesh(mesh_factory::get_mesh(MODEL_I2O1_NAND));
    this->s_in[0].tag = SOCK_TAG_GENERIC_BOOL;
    this->s_in[1].tag = SOCK_TAG_GENERIC_BOOL;
    this->s_out[0].tag = SOCK_TAG_GENERIC_BOOL;
}

edevice *nandgate::solve_electronics() {
    bool v1 = false;
    bool v2 = false;

    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();
    if (!this->s_in[1].is_ready())
        return this->s_in[1].get_connected_edevice();

    v1 = (bool)((int)roundf(this->s_in[0].get_value()));
    v2 = (bool)((int)roundf(this->s_in[1].get_value()));

    this->s_out[0].write((v1 && v2) ? 0.f : 1.f);

    return 0;
}

ifgate::ifgate() {
    this->s_in[0].tag = SOCK_TAG_VALUE;
    this->s_in[1].tag = SOCK_TAG_GENERIC_BOOL;
    this->s_out[0].tag = SOCK_TAG_VALUE;
}

edevice *ifgate::solve_electronics() {
    float v1;
    bool v2 = false;

    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();
    if (!this->s_in[1].is_ready())
        return this->s_in[1].get_connected_edevice();

    v1 = this->s_in[0].get_value();
    v2 = (bool)((int)roundf(this->s_in[1].get_value()));

    this->s_out[0].write(v2 ? v1 : 0.f);

    return 0;
}

memory::memory() : store(0.f) {
    this->s_in[0].tag = SOCK_TAG_SET_ENABLE;
    this->s_in[1].tag = SOCK_TAG_VALUE;
    this->s_out[0].tag = SOCK_TAG_VALUE;
}

edevice *memory::solve_electronics() {
    bool set = false;

    if (!this->s_out[0].written())
        this->s_out[0].write(this->store);

    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();
    if (!this->s_in[1].is_ready())
        return this->s_in[1].get_connected_edevice();

    set = ((int)roundf(this->s_in[0].get_value()));
    if (set) {
        if (!this->s_in[1].is_ready())
            return this->s_in[1].get_connected_edevice();

        this->store = this->s_in[1].get_value();
    }

    return 0;
}

edevice *halfpack::solve_electronics() {
    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();
    if (!this->s_in[1].is_ready())
        return this->s_in[1].get_connected_edevice();

    float v1 = this->s_in[0].get_value();
    float v2 = this->s_in[1].get_value();
    float packed;

    if (v2 > 0.f)
        packed = .5f + v2*.5f;
    else
        packed = .5f - v1*.5f;

    this->s_out[0].write(packed);

    return 0;
}

edevice *emul::solve_electronics() {
    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();
    if (!this->s_in[1].is_ready())
        return this->s_in[1].get_connected_edevice();

    float v1 = this->s_in[0].get_value();
    float v2 = this->s_in[1].get_value();

    float r = tclampf(v1*v2, 0.f, 1.f);

    this->s_out[0].write(r);

    return 0;
}

sum::sum() {
    this->set_mesh(mesh_factory::get_mesh(MODEL_I2O1_SUM));
}

edevice *sum::solve_electronics() {
    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();
    if (!this->s_in[1].is_ready())
        return this->s_in[1].get_connected_edevice();

    float v1 = this->s_in[0].get_value();
    float v2 = this->s_in[1].get_value();
    float sum = tclampf(v1 + v2, 0.f, 1.f);

    this->s_out[0].write(sum);

    return 0;
}

edevice *avg::solve_electronics() {
    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();
    if (!this->s_in[1].is_ready())
        return this->s_in[1].get_connected_edevice();

    float v1 = this->s_in[0].get_value();
    float v2 = this->s_in[1].get_value();
    float avg = (v1 + v2)/2.f;

    this->s_out[0].write(tclampf(avg,0.f,1.f));

    return 0;
}

edevice *emin::solve_electronics() {
    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();
    if (!this->s_in[1].is_ready())
        return this->s_in[1].get_connected_edevice();

    float a = this->s_in[0].get_value();
    float b = this->s_in[1].get_value();

    this->s_out[0].write(tclampf(!(b < a) ? a : b, 0.f, 1.f));

    return 0;
}

edevice *emax::solve_electronics() {
    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();
    if (!this->s_in[1].is_ready())
        return this->s_in[1].get_connected_edevice();

    float a = this->s_in[0].get_value();
    float b = this->s_in[1].get_value();

    this->s_out[0].write(tclampf((a < b) ? b : a, 0.f, 1.f));

    return 0;
}

wrapadd::wrapadd() {
    this->set_mesh(mesh_factory::get_mesh(MODEL_I2O1_WRAP_ADD));
}

edevice *wrapadd::solve_electronics() {
    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();
    if (!this->s_in[1].is_ready())
        return this->s_in[1].get_connected_edevice();

    float a = this->s_in[0].get_value();
    float b = this->s_in[1].get_value();

    this->s_out[0].write(twrapf(a+b, 0.f, 1.0f));

    return 0;
}

wrapsub::wrapsub() {
    this->set_mesh(mesh_factory::get_mesh(MODEL_I2O1_WRAP_SUB));
}

edevice *wrapsub::solve_electronics() {
    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();
    if (!this->s_in[1].is_ready())
        return this->s_in[1].get_connected_edevice();

    float a = this->s_in[0].get_value();
    float b = this->s_in[1].get_value();

    this->s_out[0].write(twrapf(a-b, 0.f, 1.0f));

    return 0;
}

edevice *ewrapdist::solve_electronics() {
    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();
    if (!this->s_in[1].is_ready())
        return this->s_in[1].get_connected_edevice();

    float a = this->s_in[0].get_value();
    float b = this->s_in[1].get_value();

    if (a > b) {
        float tmp = a;
        a = b;
        b = tmp;
    }

    float o = fminf(b-a, (a+1.f)-b);

    this->s_out[0].write(tclampf(o, 0.f, 1.0f));

    return 0;
}

/* Equal, == */
cmpe::cmpe() {
    this->set_mesh(mesh_factory::get_mesh(MODEL_I2O1_EQUAL));
}

edevice *cmpe::solve_electronics() {
    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();
    if (!this->s_in[1].is_ready())
        return this->s_in[1].get_connected_edevice();

    int a,b;
    if (W->level.version >= LEVEL_VERSION_1_2_1) {
        a = roundf(this->s_in[0].get_value() * 100.f);
        b = roundf(this->s_in[1].get_value() * 100.f);
    } else {
        a = (this->s_in[0].get_value() * 100.f);
        b = (this->s_in[1].get_value() * 100.f);
    }

    this->s_out[0].write(a == b ? 1.0f : 0.f);

    return 0;
}

/* Lesser than, < */
cmpl::cmpl() {
    this->set_mesh(mesh_factory::get_mesh(MODEL_I2O1_LESS));
}

edevice *cmpl::solve_electronics() {
    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();
    if (!this->s_in[1].is_ready())
        return this->s_in[1].get_connected_edevice();

    float a = this->s_in[0].get_value();
    float b = this->s_in[1].get_value();

    this->s_out[0].write(a < b ? 1.0f : 0.f);

    return 0;
}

/* Lesser than or equal, <= */
cmple::cmple() {
    this->set_mesh(mesh_factory::get_mesh(MODEL_I2O1_LESS_EQUAL));
}

edevice *cmple::solve_electronics() {
    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();
    if (!this->s_in[1].is_ready())
        return this->s_in[1].get_connected_edevice();

    int a = (this->s_in[0].get_value() * 100.f);
    int b = (this->s_in[1].get_value() * 100.f);

    this->s_out[0].write(a <= b ? 1.0f : 0.f);

    return 0;
}

