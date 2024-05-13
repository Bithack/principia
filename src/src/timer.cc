#include "timer.hh"
#include "game.hh"
#include "ui.hh"

#include <sys/time.h>

/**
 * Sockets:
 * IN0 = Start
 * IN1 = Stop
 *
 * OUT0 = Tick
 * OUT1 = Status
 *
 * Properties:
 * 0 = Time between ticks in milliseconds
 * 1 = Number of ticks (0=infinite)
 * 2 = Use system time
 **/
timer::timer()
{
    this->set_flag(ENTITY_DO_STEP,      true);
    this->set_flag(ENTITY_HAS_CONFIG,   true);

    this->dialog_id = DIALOG_TIMER;

    this->time = 0;
    this->tick = false;
    this->started = false;
    this->ticks_left = 0;

    this->menu_scale = 1.5;

    this->s_in[0].tag = SOCK_TAG_ONOFF;
    this->s_in[1].tag = SOCK_TAG_RESET;

    this->s_out[0].tag = SOCK_TAG_TICK;
    this->s_out[1].tag = SOCK_TAG_STATUS;

    this->set_num_properties(3);
    this->properties[0].type = P_INT;
    this->properties[0].v.i = 2500;
    this->properties[1].type = P_INT8;
    this->properties[1].v.i8 = 0;
    this->properties[2].type = P_INT;
    this->properties[2].v.i = 0;
}

void
timer::on_pause()
{
    this->setup();
}

void
timer::setup()
{
    this->time = 0;
    this->tick = false;
    this->started = false;
    this->ticks_left = this->properties[1].v.i8;
    this->last_tick = _tms.last_time;
}

uint64_t
timer::refresh_time()
{
    uint64_t curr_time, delta;

#ifdef TMS_BACKEND_IOS
    curr_time = tms_IOS_get_time();
#else
    struct timeval t;
    gettimeofday(&t, 0);
    curr_time = t.tv_usec + t.tv_sec * 1000000ull;
#endif

    delta = curr_time - this->last_tick;
    this->last_tick = curr_time;

    return delta;
}

void
timer::step()
{
    if (this->started) {
        if (this->properties[2].v.i) {
            this->time += refresh_time();
        } else {
            this->time += G->timemul(WORLD_STEP);
        }

        if (this->time >= this->properties[0].v.i*1000 && (this->ticks_left > 0 || this->properties[1].v.i8 == 0)) {
            this->time -= this->properties[0].v.i*1000;

            this->tick = true;
            this->ticks_left--;
        }
    } else {
        this->time = 0;
        this->tick = false;
        this->ticks_left = this->properties[1].v.i8;
        /* Started has been reset to 0, a timer with a finite amount of ticks can now tick again! */
    }
}

edevice*
timer::solve_electronics()
{
    /* Stop always takes priority over start */
    bool start = true;
    bool stop = false;

    if (!this->s_out[0].written()) {
        if (this->tick) {
            this->s_out[0].write(1.f);
            this->tick = false;
            //tms_infof("-------- %p TICK", this);
        } else {
            this->s_out[0].write(0.f);
        }
    }
    if (!this->s_out[1].written()) {
        this->s_out[1].write(this->started ? 1.f : 0.f);
    }

    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();
    if (!this->s_in[1].is_ready())
        return this->s_in[1].get_connected_edevice();

    if (this->s_in[0].p)
        start = (bool)((int)roundf(this->s_in[0].get_value()));

    stop  = (bool)((int)roundf(this->s_in[1].get_value()));

    if (start) this->started = true;
    if (stop)  this->started = false;

    return 0;
}

/**
 * Sockets:
 * IN0 = Value
 * IN1 = Conditional
 *
 * OUT0 = IN0 if IN1=true else 0
 * OUT1 = IN0 if IN1=false else 0
 **/
ifelse::ifelse()
{
    this->menu_scale = 1.5f;

    this->s_in[0].tag = SOCK_TAG_VALUE;
    this->s_in[1].tag = SOCK_TAG_GENERIC_BOOL;

    this->s_out[0].tag = SOCK_TAG_VALUE;
    this->s_out[1].tag = SOCK_TAG_VALUE;
}

edevice*
ifelse::solve_electronics()
{
    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();
    if (!this->s_in[1].is_ready())
        return this->s_in[1].get_connected_edevice();

    float value = this->s_in[0].get_value();
    int gate = (int)((bool)(roundf(this->s_in[1].get_value())));

    this->s_out[gate].write(value);
    this->s_out[!gate].write(0.f);

    return 0;
}
