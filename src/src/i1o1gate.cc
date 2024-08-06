#include "i1o1gate.hh"
#include "model.hh"
#include "material.hh"
#include "ledbuffer.hh"
#include "world.hh"
#include "game.hh"
#include "ui.hh"

i1o1gate::i1o1gate()
{
    this->set_material(&m_i1o1);

    this->num_s_in = 1;
    this->num_s_out = 1;

    this->menu_scale = 1.5f;

    this->s_in[0].ctype = CABLE_RED;
    this->s_in[0].lpos = b2Vec2(0.f, -.25f);
    this->s_in[0].angle = -M_PI/2.f;
    this->s_out[0].ctype = CABLE_RED;
    this->s_out[0].lpos = b2Vec2(0.f, .25f);
    this->s_out[0].angle = M_PI/2.f;

    this->set_as_rect(.15f, .375f);
}

i1o1gate_mini::i1o1gate_mini()
{
    this->set_mesh(mesh_factory::get_mesh(MODEL_CCLIP));
    this->set_material(&m_iomisc);

    this->num_s_in = 1;
    this->num_s_out = 1;

    this->menu_scale = 1.5f;

    this->s_in[0].lpos = b2Vec2(0.f, -.125f);
    this->s_out[0].lpos = b2Vec2(0.f, .125f);

    this->set_as_rect( .25f/2.f, .5f/2.f);
}

i1o1gate_fifo::i1o1gate_fifo()
{
    this->set_mesh(mesh_factory::get_mesh(MODEL_FIFO));
    this->set_material(&m_iomisc);

    this->num_s_in = 1;
    this->num_s_out = 1;

    this->s_in[0].lpos = b2Vec2(0.f, -.35f);
    this->s_out[0].lpos = b2Vec2(0.f, .35f);

    this->set_as_rect(.25f, .5f);
}

edevice*
invertergate::solve_electronics()
{
    if (!this->s_in[0].is_ready()) {
        return this->s_in[0].get_connected_edevice();
    }

    float v = 1.f - this->s_in[0].get_value();

    this->s_out[0].write(v);

    return 0;
}

edevice*
sparsifier::solve_electronics()
{
    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();

    bool v = (bool)((int)roundf(this->s_in[0].get_value()));

    this->s_out[0].write(
            (v == true && last != true) ? 1.f : 0.f
        );

    last = v;

    return 0;
}

void
sparsifier::write_state(lvlinfo *lvl, lvlbuf *lb)
{
    entity::write_state(lvl,lb);
    lb->ensure(sizeof(uint8_t));
    lb->w_uint8(this->last);
}

void
sparsifier::read_state(lvlinfo *lvl, lvlbuf *lb)
{
    entity::read_state(lvl,lb);
    this->last = lb->r_uint8();
}

edevice*
besserwisser::solve_electronics()
{
    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();

    bool v = (bool)((int)roundf(this->s_in[0].get_value()));

    this->s_out[0].write(
            (v != last) ? 1.f : 0.f
        );

    last = v;

    return 0;
}

void
besserwisser::write_state(lvlinfo *lvl, lvlbuf *lb)
{
    entity::write_state(lvl,lb);
    lb->ensure(sizeof(uint8_t));
    lb->w_uint8(this->last);
}

void
besserwisser::read_state(lvlinfo *lvl, lvlbuf *lb)
{
    entity::read_state(lvl,lb);
    this->last = lb->r_uint8();
}

edevice*
ceilgate::solve_electronics()
{
    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();

    float v = ceilf(this->s_in[0].get_value());

    this->s_out[0].write(v);

    return 0;
}

edevice*
integergate::solve_electronics()
{
    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();

    float v = (float)((int)this->s_in[0].get_value());

    this->s_out[0].write(v);

    return 0;
}

edevice*
squaregate::solve_electronics()
{
    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();

    float v = this->s_in[0].get_value();
    v *= v;

    this->s_out[0].write(v);

    return 0;
}

edevice*
sqrtgate::solve_electronics()
{
    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();

    float v = this->s_in[0].get_value();
    v = sqrtf(v);

    this->s_out[0].write(v);

    return 0;
}

edevice*
epsilon::solve_electronics()
{
    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();

    float v = this->s_in[0].get_value();
    v = fminf(1.f, v+1e-5);

    this->s_out[0].write(v);

    return 0;
}

fifo::fifo()
    : ptr(0)
{
    for (int x=0; x<8; x++) {
        this->queue[x] = 0.f;
    }

    this->menu_scale = 1.0f;
    this->set_flag(ENTITY_DO_UPDATE_EFFECTS, true);

    this->ptr = 0;
}

void
fifo::write_state(lvlinfo *lvl, lvlbuf *lb)
{
    entity::write_state(lvl,lb);
    lb->ensure(8*sizeof(float) + sizeof(uint8_t));
    for (int x=0; x<8; x++) {
        lb->w_float(this->queue[x]);
    }
    lb->w_uint8(this->ptr);
}

void
fifo::read_state(lvlinfo *lvl, lvlbuf *lb)
{
    entity::read_state(lvl,lb);
    for (int x=0; x<8; x++) {
        this->queue[x] = lb->r_float();
    }
    this->ptr = lb->r_uint8();
}

void
fifo::setup()
{
    for (int x=0; x<8; x++)
        this->queue[x] = 0.f;
    this->ptr = 0;
}

edevice*
fifo::solve_electronics()
{
    if (!this->s_out[0].written()) {
        float out = queue[this->ptr];
        this->s_out[0].write(out);
    }

    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();

    float v = this->s_in[0].get_value();
    queue[this->ptr] = v;

    this->ptr ++;
    this->ptr = this->ptr & 7;

    return 0;
}

void
fifo::update_effects(void)
{
    float z = this->get_layer() * LAYER_DEPTH + LED_Z_OFFSET;
    for (int x=0; x<8; x++) {
        int y = (this->ptr+x) & 7;

        b2Vec2 p = this->local_to_world(b2Vec2(0, .25f-x*(.5f/8.f)), 0);
        ledbuffer::add(p.x, p.y, z, queue[y]);
    }
}

edevice*
cavg::solve_electronics()
{
    if (!this->s_in[0].is_ready())
        this->s_in[0].get_connected_edevice();

    float v = this->s_in[0].get_value();

    float f = this->properties[0].v.f;

    if (v <= 0.f) {
        this->value = 0.f;
    } else {
        this->value = f * this->value + (1.f - f)*v;
    }

    this->s_out[0].write(this->value);

    return 0;
}

void
mavg::update_effects(void)
{
    float z = this->get_layer() * LAYER_DEPTH + LED_Z_OFFSET;
    b2Vec2 p = this->get_position();
    ledbuffer::add(p.x, p.y, z, this->value);
}

mavg::mavg()
    : value(0.f)
{
    this->num_sliders = 1;
    this->menu_scale = 1.0f;
    this->set_flag(ENTITY_DO_UPDATE_EFFECTS, true);

    this->set_num_properties(1);
    this->properties[0].type = P_INT;
    this->properties[0].v.f = 1.f - (.5f / 8.f);
}

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

muladd::muladd()
{
    this->num_sliders = 2;

    this->set_num_properties(2);
    this->properties[0].type = P_FLT;
    this->properties[0].v.f = 1.f;
    this->properties[1].type = P_FLT;
    this->properties[1].v.f = 0.f;
}

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
clamp::on_slider_change(int s, float value)
{
    this->properties[s].v.f = value;
    G->show_numfeed(value);
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

edevice*
mavg::solve_electronics()
{
    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();

    float v = this->s_in[0].get_value();

    float f = this->properties[0].v.f;
    this->value = f * this->value + (1.f - f)*v;
    this->s_out[0].write(this->value);

    return 0;
}

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

rcactivator::rcactivator()
{
    this->set_flag(ENTITY_HAS_TRACKER, true);

    this->set_num_properties(1);
    this->properties[0].type = P_ID;
    this->properties[0].v.i = 0;
}

edevice*
rcactivator::solve_electronics()
{
    entity *e;
    uint32_t entity_id = this->properties[0].v.i;

    if (entity_id != 0 && !(e = W->get_entity_by_id(entity_id))) {
        goto invalid;
    }

    if (!this->s_out[0].written()) {
        if (entity_id == 0) {
            this->s_out[0].write(0.f);
        } else {
            this->s_out[0].write((G->current_panel == e ? 1.f : 0.f));
        }
    }

    if (!this->s_in[0].is_ready()) {
       return this->s_in[0].get_connected_edevice();
    }

    if (entity_id == 0 || entity_id == this->id) {
        if ((bool)roundf(this->s_in[0].get_value())) {
            entity *target = 0;
            if (W->is_adventure() && adventure::player) {
                target = adventure::player;
                adventure::player->detach();
            }

            G->set_control_panel(target);
        }
    } else if (!this->s_in[0].p || (bool)roundf(this->s_in[0].get_value())) {
        if (G->current_panel != e) {
            G->set_control_panel(e);
        }
    }

    return 0;

invalid:
    this->s_out[0].write(0);
    if (!this->s_in[0].p || (bool)roundf(this->s_in[0].get_value())) {
        G->add_error(this, ERROR_RC_ACTIVATOR_INVALID);
    }
    return 0;
}

player_activator::player_activator()
{
    this->set_flag(ENTITY_HAS_TRACKER, true);

    this->set_num_properties(1);
    this->properties[0].type = P_ID;
    this->properties[0].v.i = 0;
}

edevice*
player_activator::solve_electronics()
{
    entity *e = W->get_entity_by_id(this->properties[0].v.i);

    float v = 0.f;

    if (!this->s_out[0].written()) {
        this->s_out[0].write((G->state.adventure_id == this->properties[0].v.i ? 1.f : 0.f));
    }

    if (!this->s_in[0].is_ready()) {
        return this->s_in[0].get_connected_edevice();
    }

    if (!this->s_in[0].p || (bool)roundf(this->s_in[0].get_value())) {
        if (W->level.type == LCAT_ADVENTURE) {
            /* If we have received an input from an PA which was not just activated. */
            if (adventure::player != e) {

                /* If the target exists and is a creature */
                if (e && e->is_creature()) {
                    /* By default, we will also set the RC to the player. */
                    bool set_rc = true;

                    /* However, if there is a current panel, and that panel is an RC,
                     * we will not set the RC to the new player. */
                    if (G->current_panel && G->current_panel->is_control_panel()) {
                        set_rc = false;
                    }

                    adventure::set_player(static_cast<creature*>(e), true, set_rc);
                } else {
                    /* We either recived a null-target or a target which is not a creature.
                     * Either way, unset the current player */
                    adventure::set_player(0);
                }
            }
        }
    }

    return 0;
}

decay::decay()
    : value(0.f)
{
    this->set_mesh(mesh_factory::get_mesh(MODEL_I1O1_EMPTY));

    this->num_sliders = 1;

    this->set_num_properties(1);
    this->properties[0].type = P_FLT;
    this->properties[0].v.f = 0.95f; /* decay multiplier */
}

edevice*
decay::solve_electronics()
{
    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();

    float v = this->s_in->get_value();

    this->value += v;

    if (this->value > 1.f) this->value = 1.f;
    this->value *= this->properties[0].v.f;

    if (this->value < 0.000001f) this->value = 0.f;

    this->s_out[0].write(this->value);

    return 0;
}

float
decay::get_slider_value(int s)
{
    return (this->properties[0].v.f - 0.79f) * 5.f;
}

void
decay::on_slider_change(int s, float value)
{
    float v = value * 0.2f + 0.79f;
    this->properties[0].v.f = v;
    G->show_numfeed(v);
}

ldecay::ldecay()
{
    this->properties[0].v.f = 0.1f; /* decay rate */
}

edevice*
ldecay::solve_electronics()
{
    if (!this->s_in[0].is_ready()) {
        return this->s_in[0].get_connected_edevice();
    }

    float v = this->s_in->get_value();

    this->value += v;

    this->value = tclampf(this->value, 0.f, 1.f);
    this->value -= this->properties[0].v.f;

    // BUG: Linear decay used to not clamp, keep buggy behaviour for old levels.
    if (W->level.version >= LEVEL_VERSION_FUTURE)
        this->value = tclampf(this->value, 0.f, 1.f);

    this->s_out[0].write(this->value);

    return 0;
}

float
ldecay::get_slider_value(int s)
{
    return this->properties[0].v.f * 10.f;
}

void
ldecay::on_slider_change(int s, float value)
{
    float v = value / 10.f;
    this->properties[0].v.f = v;
    G->show_numfeed(v, 3);
}

boundary::boundary()
    : value(0.f)
{
    this->num_sliders = 1;

    this->set_num_properties(1);
    this->properties[0].type = P_FLT;
    this->properties[0].v.f = 0.f;
}

edevice*
boundary::solve_electronics()
{
    if (!this->s_out[0].written()) {
        this->s_out[0].write(this->value);
    }

    if (!this->s_in[0].is_ready()) {
        return this->s_in[0].get_connected_edevice();
    }

    this->value = this->s_in[0].get_value();

    return 0;
}

float
boundary::get_slider_value(int s)
{
    return this->properties[0].v.f;
}

void
boundary::on_slider_change(int s, float value)
{
    this->properties[0].v.f = value;
    G->show_numfeed(value);
}

elimit::elimit()
{
    this->counter = 0;
    this->num_sliders = 1;
    this->set_mesh(mesh_factory::get_mesh(MODEL_I1O1_EMPTY));

    this->set_num_properties(1);
    this->properties[0].type = P_INT;
    this->properties[0].v.i = 5;

    this->s_out[0].tag = SOCK_TAG_GENERIC_BOOL;
    this->s_in[0].tag = SOCK_TAG_GENERIC_BOOL;
}

edevice*
elimit::solve_electronics()
{
    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();

    bool v = ((bool)(int)roundf(this->s_in[0].get_value()) && this->counter < this->properties[0].v.i);

    if (v) this->counter ++;

    this->s_out[0].write(v);

    return 0;
}

float
elimit::get_slider_value(int s)
{
    return this->properties[0].v.i / 10.f;
}

void
elimit::on_slider_change(int s, float value)
{
    int v = roundf(value * 10.f);
    this->properties[0].v.i = v;
    G->show_numfeed(v);
}

snapgate::snapgate()
{
    this->num_sliders = 1;

    this->set_num_properties(1);
    this->properties[0].type = P_FLT;
    this->properties[0].v.f = 2.f;
}

edevice*
snapgate::solve_electronics()
{
    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();

    float v = this->s_in[0].get_value();
    float snap = 1.f / (this->properties[0].v.f);

    v /= snap;
    v = roundf(v);
    v *= snap;

    this->s_out[0].write(v);

    return 0;
}

void
snapgate::on_slider_change(int s, float value)
{
    value = 1.f + value * 20.f;
    this->properties[s].v.f = value;
    G->show_numfeed(value);
}

sequencer::sequencer()
{
    this->set_flag(ENTITY_DO_STEP,      true);
    this->set_flag(ENTITY_HAS_CONFIG,   true);

    this->dialog_id = DIALOG_SEQUENCER;

    this->set_num_properties(3);
    this->properties[0].type = P_STR;
    this->properties[1].type = P_INT;
    this->properties[2].type = P_INT8;

    this->properties[0].v.s.buf = 0; /* Sequence string */
    this->properties[0].v.s.len = 0;
    this->properties[1].v.i = 250; /* Time between changes */
    this->properties[2].v.i8 = 1; /* Wrap around */

    this->refresh_sequence();
}

void
sequencer::refresh_sequence()
{
    char *c = this->properties[0].v.s.buf;
    this->num_steps = 0;

    if (!c) {
        this->set_property(0, "10");
        c = this->properties[0].v.s.buf;
    }

    while (*c && this->num_steps < SEQUENCER_MAX_LENGTH) {
        if (*c == '1') {
            this->sequence[this->num_steps++] = 1;
        } else if (*c == '0') {
            this->sequence[this->num_steps++] = 0;
        }
        ++c;
    }

    if (this->num_steps == 0) {
        this->sequence[0] = 1;
        this->sequence[1] = 0;
        this->set_property(0, "10");
        this->num_steps = 2;
    }
}

void
sequencer::on_load(bool created, bool has_state)
{
    this->refresh_sequence();
}

void
sequencer::step()
{
    if (this->started) {
        if (this->cur_step < this->num_steps-1 || this->properties[2].v.i8 == 1) {
            this->time += G->timemul(WORLD_STEP);

            if (this->time >= this->properties[1].v.i*1000) {
                this->time -= this->properties[1].v.i*1000;
                this->cur_step = (this->cur_step + 1) % this->num_steps;
            }
        }
    } else {
        this->time = 0;
        this->cur_step = 0;
    }
}

edevice*
sequencer::solve_electronics()
{
    if (!this->s_out[0].written()) {
        this->s_out[0].write(this->started ? this->sequence[this->cur_step] : 0.f);
    }

    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();

    bool start = true;

    if (this->s_in[0].p)
        start = (bool)((int)roundf(this->s_in[0].get_value()));

    if (start) this->started = true;
    else       this->started = false;

    return 0;
}

