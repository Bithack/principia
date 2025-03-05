#include "eventlistener.hh"
#include "ui.hh"
#include "world.hh"

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
