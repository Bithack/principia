#include "transmitter.hh"
#include "material.hh"
#include "model.hh"
#include "world.hh"
#include "game.hh"
#include "receiver.hh"
#include "ui.hh"

#define FORCE_NEW_SOLVER

/* Old frequency method, this gave a variable amount of ticks
 * per frequency assuming IN1 was used */
uint32_t
frequency_solver_old(uint32_t base, float extra)
{
    return (base + (uint32_t)roundf(extra * 10.f));
}

/* The new frequency method, extra verbose for now and probably more "complex" than it
 * needs to be. */
uint32_t
frequency_solver_new(uint32_t base, float extra)
{
    float v = extra;
    if (v == 1.f) v -= 0.001f;
    double offset_d = (double)v * 10.0;
    double offset_dr = floorf(offset_d);
    uint32_t offset_i = (uint32_t)offset_dr;

    return base + offset_i;
}

transmitter::transmitter(int _is_broadcaster)
    : pending_value(0.f)
    , is_broadcaster(_is_broadcaster)
{
    this->set_flag(ENTITY_HAS_CONFIG, true);

    this->set_mesh(mesh_factory::get_mesh(MODEL_TRANSMITTER));
    this->set_material(&m_edev);

    this->set_num_properties(this->is_broadcaster?2:1);
    this->properties[0].type = P_INT; /* Transmit frequency, range_min for broadcaster */
    this->properties[0].v.i = 1;

    if (this->is_broadcaster) {
        this->properties[1].type = P_INT; /* range_max */
        this->properties[1].v.i = 10;

        this->dialog_id = DIALOG_SET_FREQ_RANGE;
    } else {
        this->dialog_id = DIALOG_SET_FREQUENCY;
    }

    this->num_s_in = 2;
    this->s_in[0].lpos = b2Vec2(-.125f, -.25f);
    this->s_in[1].lpos = b2Vec2(.125f, -.25f);

    this->s_in[0].tag = SOCK_TAG_VALUE;

    this->set_as_rect(.25f, .375f);

    if (W->level.version >= LEVEL_VERSION_1_1_7) {
        this->frequency_solver = frequency_solver_new;
    } else {
#ifdef FORCE_NEW_SOLVER
        this->frequency_solver = frequency_solver_new;
#else
        this->frequency_solver = frequency_solver_old;
#endif
    }
}

void
transmitter::construct()
{
    this->pending_value = 0.f;
}

void
transmitter::on_pause()
{
    this->pending_value = 0.f;
}

void
transmitter::setup()
{
}

edevice*
transmitter::solve_electronics()
{
    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();
    if (!this->s_in[1].is_ready())
        return this->s_in[1].get_connected_edevice();

    this->pending_value = this->s_in[0].get_value();

    uint32_t freq = this->frequency_solver(this->properties[0].v.i, this->s_in[1].get_value());

    if (this->pending_value > 0.f) {
        if (this->is_broadcaster) {
            uint32_t freq_max = freq + this->properties[1].v.i;

            for (std::multimap<uint32_t, receiver_base*>::iterator
                    i = W->receivers.begin();
                    i != W->receivers.end()
                    ;
                    ) {

                if (i->first >= freq && i->first <= freq_max) {
                    if (!i->second->no_broadcast)
                        i->second->pending_value = this->pending_value;
                    i++;
                } else
                    i = W->receivers.upper_bound(i->first);
            }
        } else {
            std::pair<std::multimap<uint32_t, receiver_base*>::iterator, std::multimap<uint32_t, receiver_base*>::iterator> range = W->receivers.equal_range(freq);
            for (std::multimap<uint32_t, receiver_base*>::iterator
                    i = range.first;
                    i != range.second && i != W->receivers.end();
                    i++) {
                i->second->pending_value = this->pending_value;
                i->second->no_broadcast = true;
            }
        }
    }

    return 0;
}

void
transmitter::write_quickinfo(char *out)
{
    sprintf(out, "%s (f:%u)", this->get_name(), this->properties[0].v.i);
}
