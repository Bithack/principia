#include "i0o1gate.hh"
#include "model.hh"
#include "world.hh"
#include "game.hh"
#include "object_factory.hh"
#include "ui.hh"

#define STEPS_PER_FREQUENCY 125

i0o1gate::i0o1gate()
{
    this->set_mesh(mesh_factory::get_mesh(MODEL_I0O1));
    this->set_material(&m_iomisc);

    this->menu_scale = 1.5;

    this->num_s_out = 1;
    this->s_out[0].lpos = b2Vec2(0.f, .0625f);
    this->set_as_rect(.15f, .2f);
}

edevice*
erandom::solve_electronics(void)
{
    this->s_out[0].write((float)rand()/(float)RAND_MAX);
    return 0;
}

sawtooth::sawtooth()
{
#ifdef PRECISE_SAWTOOTH
    this->elapsed = 0;
#else
    this->elapsed = 0.f;
#endif

    this->num_sliders = 2;
    this->set_num_properties(2);

    this->properties[0].type = P_FLT; /* Frequency */
    this->properties[0].v.f = 1.0f;

    this->properties[1].type = P_FLT; /* Phase */
    this->properties[1].v.f = 0.f;
}

void
sawtooth::setup()
{
#ifdef PRECISE_SAWTOOTH
    double x = roundf(this->properties[1].v.f * (STEPS_PER_FREQUENCY / this->properties[0].v.f));
    this->elapsed = (int)x;
#else
    this->elapsed = this->properties[1].v.f;
#endif
}

edevice*
sawtooth::solve_electronics(void)
{
    float v;

#ifdef PRECISE_SAWTOOTH
    int step = ((int)roundf(STEPS_PER_FREQUENCY / this->properties[0].v.f));
    int a = this->elapsed % step;
    v = (float)a / (float)step;

    this->elapsed += 1;
#else
    float inv = 1.f/this->properties[0].v.f;
    v = fmod(this->elapsed, inv) / inv;

    this->elapsed += WORLD_STEP/1000000.;
#endif

    this->s_out[0].write(v);

    return 0;
}

void
sawtooth::on_slider_change(int s, float value)
{
    if (s == 0) {
        //if (value == 0) value = 0.1f/4.f;
        //this->properties[s].v.f = tmath_logstep(value, 0.01, 10.01);
        this->properties[s].v.f = value * 4.f;
    } else
        this->properties[s].v.f = value;

    G->show_numfeed(this->properties[s].v.f);
}

sinewave::sinewave()
{
    this->elapsed = 0.f;
    this->num_sliders = 2;
    this->set_num_properties(2);

    this->properties[0].type = P_FLT; /* Frequency */
    this->properties[0].v.f = 1.0f;

    this->properties[1].type = P_FLT; /* Phase */
    this->properties[1].v.f = 0.f;
}

void
sinewave::setup()
{
    this->elapsed = this->properties[1].v.f*M_PI;
}

edevice*
sinewave::solve_electronics(void)
{
    float v = sin((float)(this->elapsed*this->properties[0].v.f * M_PI * 2.)) * .5f + .5f;

    this->s_out[0].write(v);

    this->elapsed += WORLD_STEP/1000000.;
    return 0;
}

void
sinewave::on_slider_change(int s, float value)
{
    if (s == 0) {
        this->properties[s].v.f = value*4.f;
    } else
        this->properties[s].v.f = value;

    G->show_numfeed(this->properties[s].v.f);
}

eventlistener::eventlistener()
{
    this->set_flag(ENTITY_HAS_CONFIG, true);

    this->dialog_id = DIALOG_EVENTLISTENER;

    this->triggered = 0;

    this->set_num_properties(1);
    this->properties[0].type = P_INT;
    this->properties[0].v.i = 0;
}

void
eventlistener::setup()
{
    this->triggered = 0;
    this->event_id = this->properties[0].v.i;
    if (this->event_id < 0) this->event_id = 0;
    else if (this->event_id >= WORLD_EVENT__NUM) this->event_id = WORLD_EVENT__NUM-1;
}

void
eventlistener::restore()
{
    entity::restore();

    this->event_id = this->properties[0].v.i;
    if (this->event_id < 0) this->event_id = 0;
    else if (this->event_id >= WORLD_EVENT__NUM) this->event_id = WORLD_EVENT__NUM-1;
}

edevice*
eventlistener::solve_electronics(void)
{
    this->s_out[0].write(this->triggered > 0 ? 1.f : 0.f);
    this->triggered --;

    if (this->triggered < 0) this->triggered = 0;
    return 0;
}

var_getter::var_getter()
{
    this->set_flag(ENTITY_HAS_CONFIG, true);

    this->dialog_id = DIALOG_VARIABLE;


    this->set_num_properties(1);
    this->properties[0].type = P_STR;
    this->set_property(0, "x");
}

/* TODO: on play, save an iterator to the correct variable */

edevice*
var_getter::solve_electronics(void)
{
    std::map<std::string, double>::iterator i = W->level_variables.find(this->properties[0].v.s.buf);
    if (i != W->level_variables.end()) {
        this->s_out[0].write(tclampf(i->second, 0.f, 1.f));
    } else {
        this->s_out[0].write(0.f);
    }

    return 0;
}

bool
var_getter::compatible_with(entity *o)
{
    return (this->num_properties == o->num_properties && 
            (o->g_id == O_VAR_GETTER || o->g_id == O_VAR_SETTER));
}

void
var_getter::write_quickinfo(char *out)
{
    snprintf(out, 255, "%s (%s)", this->get_name(), this->properties[0].v.s.buf);
}

key_listener::key_listener()
{
    this->set_flag(ENTITY_HAS_CONFIG, true);

    this->dialog_id = DIALOG_KEY_LISTENER;

    this->set_num_properties(1);
    this->properties[0].type = P_INT; /* key to listen to */
    this->properties[0].v.i = TMS_KEY_SPACE;

    this->active = false;
}

void
key_listener::setup()
{
    this->active = false;
}

edevice*
key_listener::solve_electronics()
{
    this->s_out[0].write(this->active ? 1.f : 0.f);

    return 0;
}
