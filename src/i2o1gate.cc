#include "i2o1gate.hh"
#include "game.hh"
#include "material.hh"
#include "robot_base.hh"

i2o1gate::i2o1gate()
{
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

edevice*
xorgate::solve_electronics()
{
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

edevice*
orgate::solve_electronics()
{
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

edevice*
andgate::solve_electronics()
{
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

edevice*
nandgate::solve_electronics()
{
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

edevice*
ifgate::solve_electronics()
{
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

edevice*
memory::solve_electronics()
{
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

edevice*
halfpack::solve_electronics()
{
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

edevice*
emul::solve_electronics()
{
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

edevice*
sum::solve_electronics()
{
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

edevice*
avg::solve_electronics()
{
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

edevice*
emin::solve_electronics()
{
    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();
    if (!this->s_in[1].is_ready())
        return this->s_in[1].get_connected_edevice();

    float a = this->s_in[0].get_value();
    float b = this->s_in[1].get_value();

    this->s_out[0].write(tclampf(!(b < a) ? a : b, 0.f, 1.f));

    return 0;
}

edevice*
emax::solve_electronics()
{
    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();
    if (!this->s_in[1].is_ready())
        return this->s_in[1].get_connected_edevice();

    float a = this->s_in[0].get_value();
    float b = this->s_in[1].get_value();

    this->s_out[0].write(tclampf((a < b) ? b : a, 0.f, 1.f));

    return 0;
}

static void
on_target_absorbed(entity *self, void *userdata)
{
    hp_control *hc = static_cast<hp_control*>(self);
    hc->unsubscribe(hc->target);
    hc->target = 0;
}

void
hp_control::setup()
{
    this->target = 0;

    if (this->properties[0].v.i != 0) {
        entity *e = W->get_entity_by_id(this->properties[0].v.i);
        if (e && e->flag_active(ENTITY_IS_ROBOT)) {
            this->target = static_cast<robot_base*>(e);
            this->subscribe(this->target, ENTITY_EVENT_REMOVE, &on_target_absorbed);
        }
    }
}

void
hp_control::restore()
{
    entity::restore();

    this->target = 0;

    if (this->properties[0].v.i != 0) {
        entity *e = W->get_entity_by_id(this->properties[0].v.i);
        if (e && e->flag_active(ENTITY_IS_ROBOT)) {
            this->target = static_cast<robot_base*>(e);
            this->subscribe(this->target, ENTITY_EVENT_REMOVE, &on_target_absorbed);
        }
    }
}

edevice*
hp_control::solve_electronics()
{
    bool set = false;

    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();
    if (!this->s_in[1].is_ready())
        return this->s_in[1].get_connected_edevice();

    if (this->target) {
        set = (bool)((int)roundf(this->s_in[0].get_value()));
        float hp = this->target->get_hp();

        if (set) {
            float new_hp = this->target->get_max_hp() * this->s_in[1].get_value();
            this->target->set_hp(new_hp);

            if (new_hp != hp) {
                G->add_hp(this->target, new_hp / this->target->get_max_hp());
            }
        }

        this->s_out[0].write(tclampf(hp / this->target->get_max_hp(), 0.f, 1.f));
    } else {
        this->s_out[0].write(0.f);
    }

    return 0;
}

condenser::condenser()
    : value(0.f)
{
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

void
condenser::setup()
{
    this->value = this->properties[1].v.f * this->properties[0].v.f;
}

float
condenser::get_slider_value(int s)
{
    if (s == 0)
        return (this->properties[0].v.f - 1.f) / 31.f;
    else // s == 1
        return this->properties[1].v.f;
}

void
condenser::on_slider_change(int s, float value)
{
    if (s == 0) {
        float v = 1.f + (value * 31.f);
        this->properties[0].v.f = v;
        G->show_numfeed(v);
    } else { // s == 1
        this->properties[1].v.f = value;
        G->show_numfeed(value);
    }
}

edevice*
condenser::solve_electronics()
{
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

edevice*
wrapcondenser::solve_electronics()
{
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

edevice*
wrapadd::solve_electronics()
{
    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();
    if (!this->s_in[1].is_ready())
        return this->s_in[1].get_connected_edevice();

    float a = this->s_in[0].get_value();
    float b = this->s_in[1].get_value();

    this->s_out[0].write(twrapf(a+b, 0.f, 1.0f));

    return 0;
}

edevice*
wrapsub::solve_electronics()
{
    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();
    if (!this->s_in[1].is_ready())
        return this->s_in[1].get_connected_edevice();

    float a = this->s_in[0].get_value();
    float b = this->s_in[1].get_value();

    this->s_out[0].write(twrapf(a-b, 0.f, 1.0f));

    return 0;
}

edevice*
ewrapdist::solve_electronics()
{
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
edevice*
cmpe::solve_electronics()
{
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
edevice*
cmpl::solve_electronics()
{
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
edevice*
cmple::solve_electronics()
{
    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();
    if (!this->s_in[1].is_ready())
        return this->s_in[1].get_connected_edevice();

    int a = (this->s_in[0].get_value() * 100.f);
    int b = (this->s_in[1].get_value() * 100.f);

    this->s_out[0].write(a <= b ? 1.0f : 0.f);

    return 0;
}

