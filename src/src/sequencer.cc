#include "sequencer.hh"
#include "game.hh"
#include "ui.hh"
#include "world.hh"

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

