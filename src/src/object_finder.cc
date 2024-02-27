#include "object_finder.hh"
#include "model.hh"
#include "game.hh"

object_finder::object_finder()
{
    this->set_flag(ENTITY_HAS_TRACKER, true);

    this->set_material(&m_edev_dark);

    this->num_sliders = 1;

    this->set_num_properties(2);
    this->properties[0].type = P_INT;
    this->properties[0].v.i = 0;

    this->properties[1].type = P_FLT;
    this->properties[1].v.f = .5f;

    this->s_out[0].tag = SOCK_TAG_ANGLE;
    this->s_out[1].tag = SOCK_TAG_DISTANCE;
}

float
object_finder::get_slider_snap(int s)
{
    return .05f;
}

float
object_finder::get_slider_value(int s)
{
    return this->properties[1].v.f;
}

void
object_finder::on_slider_change(int s, float value)
{
    this->set_property(1, value);

    G->show_numfeed(value);
}

edevice*
object_finder::solve_electronics()
{
    entity *e;
    if ((e = W->get_entity_by_id(this->properties[0].v.i))) {
        b2Vec2 dist = e->get_position() - this->get_position();
        float dlen = dist.Length();

        b2Vec2 local_vec = this->world_to_local(e->get_position(), 0);
        float il = 1.f/local_vec.Length();
        local_vec.y *= il;
        local_vec.x *= il;

        float angle = tmath_atan2add(local_vec.y, local_vec.x);

        dlen -= .5f;

        float a = fabsf(fmodf(angle, M_PI*2.f)) / (M_PI*2.f);
        float s = tclampf(dlen * this->properties[1].v.f, 0.f, 1.f);

        this->s_out[0].write(a);
        this->s_out[1].write(s);

        //tms_infof("dist: %f", s);
    } else {
        this->s_out[0].write(0.f);
        this->s_out[1].write(0.f);
    }

    return 0;
}

/** CURSOR FINDER **/

cursor_finder::cursor_finder()
{
    this->num_sliders = 1;

    this->set_num_properties(1);
    this->properties[0].type = P_FLT;
    this->properties[0].v.f = 5.f;

    this->s_out[0].tag = SOCK_TAG_ANGLE;
    this->s_out[1].tag = SOCK_TAG_DISTANCE;
}

float
cursor_finder::get_slider_snap(int s)
{
    return .05f;
}

float
cursor_finder::get_slider_value(int s)
{
    return this->properties[0].v.f / 20.f;
}

void
cursor_finder::on_slider_change(int s, float value)
{
    this->set_property(0, value*20.f);

    G->show_numfeed(value*20.f);
}

edevice*
cursor_finder::solve_electronics()
{
    b2Vec2 cp = G->get_last_cursor_pos(this->get_layer());

    b2Vec2 dist = cp - this->get_position();
    float dlen = dist.Length();

    b2Vec2 local_vec = this->world_to_local(cp, 0);
    float il = 1.f/local_vec.Length();
    local_vec.y *= il;
    local_vec.x *= il;

    float angle = tmath_atan2add(local_vec.y, local_vec.x);

    float a = fabsf(fmodf(angle, M_PI*2.f)) / (M_PI*2.f);
    float s = tclampf(dlen / (this->properties[0].v.f>0.f ? this->properties[0].v.f : 1.f), 0.f, 1.f);

    this->s_out[0].write(a);
    this->s_out[1].write(s);

    return 0;
}

