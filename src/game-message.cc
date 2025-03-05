#include "game-message.hh"
#include "text.hh"
#include "pscreen.hh"

#include <tms/core/glob.h>

static double DEFAULT_ALPHA_MULTIPLIER = 4.0;

enum {
    STATE_IDLE,
    STATE_IN,
    STATE_WAIT,
    STATE_OUT
};

game_message::game_message()
{
    this->text = new p_text(font::medium, ALIGN_CENTER, ALIGN_CENTER);
    this->alpha = &this->text->color.a;
    this->oalpha = &this->text->outline_color.a;
    this->set_alpha(0.f);

    this->state = STATE_IDLE;
}

void
game_message::set_position(float x, float y)
{
    this->text->set_position(x, y);
}

void
game_message::render(pscreen *ps)
{
    switch (this->state) {
        case STATE_IDLE:
            this->set_alpha(0.f);
            return;

        case STATE_IN:
            if (this->in_duration >= this->max_in_duration) {
                this->state = STATE_WAIT;
                this->set_alpha(1.f);
            } else {
                this->set_alpha((this->in_duration) / this->max_in_duration);

                this->in_duration += _tms.dt;
            }
            break;

        case STATE_WAIT:
            if (this->duration <= 0) {
                this->state = STATE_OUT;
            } else {
                this->duration -= _tms.dt;
            }
            break;

        case STATE_OUT:
            if (this->out_duration >= this->max_out_duration) {
                this->state = STATE_IDLE;
                this->set_alpha(0.f);
            } else {
                this->set_alpha((this->max_out_duration-this->out_duration) / this->max_out_duration);

                this->out_duration += _tms.dt;
            }
            break;
    }

    ps->add_rounded_square(
            this->text->get_x(),
            this->text->get_y(),
            this->text->get_width(),
            this->text->get_height(),
            tvec4f(.2f, .2f, .2f, (*this->alpha)*.5f),
            4.f
            );
    ps->add_text(this->text);
}

void
game_message::show(const char *text, double duration/*=2.5*/, double in_speed/*=1.0*/, double out_speed/*=1.0*/)
{
    this->text->set_text(text);

    this->duration = duration;

    this->out_duration = 0.0;

    this->max_in_duration = in_speed;
    this->max_out_duration = out_speed;

    switch (this->state) {
        case STATE_IDLE:
            this->in_duration = 0.0;
            this->state = STATE_IN;
            break;

        case STATE_OUT:
            this->out_duration = 0.0;
            this->state = STATE_IN;
            break;
    }
}
