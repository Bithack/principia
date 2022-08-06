#include "button.hh"
#include "model.hh"
#include "material.hh"
#include "world.hh"
#include "game.hh"

#define SWITCH_OFFS .1f

enum {
    BUTTON_NORMAL,
    BUTTON_TOGGLE,
};

enum {
    STEP_NONE,
    STEP_PRESS_DOWN,
    STEP_UP,
};

button::button(int _button_type)
    : num_blocking(0)
    , step_action(STEP_NONE)
    , button_type(_button_type)
    , down_time(0.f)
    , pressed(false)
    , switch_sensor_status(false)
{
    this->set_flag(ENTITY_DO_STEP, true);
    this->set_flag(ENTITY_CUSTOM_GHOST_UPDATE, true);

    this->set_mesh(mesh_factory::get_mesh(MODEL_BUTTON));
    this->set_material(&m_edev_dark);

    this->update_method = ENTITY_UPDATE_CUSTOM;

    this->mswitch = new bswitch(this);
    this->add_child(this->mswitch);

    this->num_s_in = 0;
    this->num_s_out = 1;

    this->s_out[0].lpos = b2Vec2(.125f, -.1f);
    this->s_out[0].ctype = CABLE_RED;
    this->s_out[0].tag = SOCK_TAG_GENERIC_BOOL;

    this->layer_mask = 8;

    this->width = .4f;
    this->height = .25f;

    float qw = (this->width)/2.f+0.15f;
    float qh = (this->height)/2.f+0.15f;
    this->query_sides[0].Set(0.f,  0.f); /* up */
    this->query_sides[1].Set(-qw, 0.f); /* left */
    this->query_sides[2].Set(0.f, -qh); /* down */
    this->query_sides[3].Set( qw, 0.f); /* right */
}

void
button::add_to_world()
{
    tms_debugf("add to world");
    b2BodyDef bd;
    bd.type = this->get_dynamic_type();
    bd.position = _pos;
    bd.angle = _angle;

    b2Body *b = W->b2->CreateBody(&bd);
    this->body = b;

    b2PolygonShape box;
    box.SetAsBox(.375f, .25f);

    b2FixtureDef fd;
    fd.shape = &box;
    fd.density = 1.f;
    fd.friction = .5f;
    fd.restitution = .3f;
    fd.filter = world::get_filter_for_layer(this->get_layer(), 6);

    (b->CreateFixture(&fd))->SetUserData(this);

    b2PolygonShape switch_box;
    switch_box.SetAsBox(.35f, .2f, b2Vec2(.0f, SWITCH_OFFS), 0);

    b2FixtureDef switch_fd;
    switch_fd.shape = &switch_box;
    switch_fd.density = 1.0f;
    switch_fd.friction = .1f;
    switch_fd.restitution = .1f;
    switch_fd.isSensor = false;
    if (W->is_paused()) {
        switch_fd.filter = world::get_filter_for_layer(this->get_layer(), 0);
    } else {
        switch_fd.filter = world::get_filter_for_layer(this->get_layer(), 15);
    }

    (this->switch_fx = this->body->CreateFixture(&switch_fd))->SetUserData(this);
}

void
button::on_touch(b2Fixture *my, b2Fixture *other)
{
    this->num_blocking ++;
}

void
button::on_untouch(b2Fixture *my, b2Fixture *other)
{
    this->num_blocking --;

    if (this->num_blocking <= 0 && this->button_type == BUTTON_TOGGLE) {
        this->step_action = STEP_UP;
    }
}

void button::step()
{
    if (this->down_time >= 0.f) {
        this->down_time -= 0.1f * G->get_time_mul();
        return;
    }

    switch (this->step_action) {
        case STEP_PRESS_DOWN:
            this->num_blocking = 0;
            if (this->button_type == BUTTON_TOGGLE) {
                this->pressed = !this->pressed;
            } else {
                this->pressed = true;
            }

            this->switch_fx->SetSensor(true);

            this->down_time = 5.f;
            break;

        case STEP_UP:
            this->num_blocking = 0;
            this->switch_fx->SetSensor(false);
            break;
    }

    this->step_action = STEP_NONE;
}

void
button::update(void)
{
    this->mswitch->update();
    entity_fast_update(this);
}

void
button::ghost_update(void)
{
    this->mswitch->update();
    entity_fast_update(this);
}

void
button::restore()
{
    this->switch_fx->SetSensor(this->switch_sensor_status);
}

void
button::set_layer(int z)
{
    entity::set_layer(z);
    this->mswitch->set_layer(z);
}

edevice*
button::solve_electronics()
{
    this->s_out[0].write(this->pressed ? 1.f : 0.f);

    /* always done immediately */
    return 0;
}

void
button::press()
{
    if (!this->switch_fx->IsSensor()) {
        this->step_action = STEP_PRESS_DOWN;
    }
}

/////////////////////

button::bswitch::bswitch(button *parent)
{
    this->parent = parent;
    this->set_mesh(mesh_factory::get_mesh(MODEL_BUTTON_SWITCH));
    this->set_material(&m_pv_colored);

    this->set_uniform("~color", .7f, .7f, .7f, 1.f);
}

b2Vec2
button::bswitch::get_position()
{
    return this->parent->local_to_world(b2Vec2(0.f, (this->parent->body && this->parent->switch_fx->IsSensor())?0.f:SWITCH_OFFS), 0);
}

float
button::bswitch::get_angle()
{
    return this->parent->get_angle();
}

void
button::bswitch::update()
{
    b2Vec2 p = this->get_position();
    float a = this->get_angle();

    float c = cosf(a);
    float s = sinf(a);

    this->M[0] = c;
    this->M[1] = s;
    this->M[4] = -s;
    this->M[5] = c;
    this->M[12] = p.x;
    this->M[13] = p.y;
    this->M[14] = this->prio * LAYER_DEPTH;

    tmat3_copy_mat4_sub3x3(this->N, this->M);
}

