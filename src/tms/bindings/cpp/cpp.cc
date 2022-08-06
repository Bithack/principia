#include "cpp.hh"

extern "C" {

static int _oopassthrough_handle_input(struct tms_screen *s, struct tms_event *ev, int action);
static int _oopassthrough_render(struct tms_screen *s);
static int _oopassthrough_post_render(struct tms_screen *s);
static int _oopassthrough_begin_frame(struct tms_screen *s);
static int _oopassthrough_end_frame(struct tms_screen *s);
static int _oopassthrough_pause(struct tms_screen *s);
static int _oopassthrough_resume(struct tms_screen *s);
static int _oopassthrough_step(struct tms_screen *s, double dt);

tms_screen_spec tms::_oopassthrough = {
    0,
    _oopassthrough_pause,
    _oopassthrough_resume,
    _oopassthrough_render,
    _oopassthrough_post_render,
    _oopassthrough_begin_frame,
    _oopassthrough_end_frame,
    _oopassthrough_step,
    _oopassthrough_handle_input,
    0
};


void
_oopassthrough_entity_update(struct tms_entity *e)
{
    tms::entity *ent = static_cast<tms::entity *>(e);
    ent->update();
}

static int _oopassthrough_pause(struct tms_screen *s)
{
    tms::screen *ss = reinterpret_cast<tms::screen*>(s->data);
    return ss->pause();

}

static int _oopassthrough_resume(struct tms_screen *s)
{
    tms::screen *ss = reinterpret_cast<tms::screen*>(s->data);
    return ss->resume();
}

static int _oopassthrough_handle_input(struct tms_screen *s, struct tms_event *ev, int action)
{
    tms::screen *ss = reinterpret_cast<tms::screen*>(s->data);
    return ss->handle_input(ev, action);
}

static int _oopassthrough_render(struct tms_screen *s)
{
    tms::screen *ss = reinterpret_cast<tms::screen*>(s->data);
    return ss->render();
}

static int _oopassthrough_post_render(struct tms_screen *s)
{
    tms::screen *ss = reinterpret_cast<tms::screen*>(s->data);
    return ss->post_render();
}

static int _oopassthrough_begin_frame(struct tms_screen *s)
{
    tms::screen *ss = reinterpret_cast<tms::screen*>(s->data);
    return ss->begin_frame();
}

static int _oopassthrough_end_frame(struct tms_screen *s)
{
    tms::screen *ss = reinterpret_cast<tms::screen*>(s->data);
    return ss->end_frame();
}


static int _oopassthrough_step(struct tms_screen *s, double dt)
{
    tms::screen *ss = (tms::screen *)s->data;
    return ss->step(dt);
}

}
