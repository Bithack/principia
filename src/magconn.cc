#include "magconn.hh"
#include "world.hh"
#include "model.hh"
#include "material.hh"
#include "game.hh"

magsock::magsock()
    : plug(0)
    , connected(0)
    , sensor(0)
{
    this->scaleselect = true;
    this->scalemodifier = 3.f;

    this->set_mesh(mesh_factory::get_mesh(MODEL_MAGSOCK));
    this->set_material(&m_edev);
    //this->set_uniform("~color", .25f, .25f, .25f, 1.f);

    this->set_flag(ENTITY_DO_STEP, true);

    this->num_s_out = 2;

    /* connection status */
    this->s_out[0].lpos = b2Vec2(-.125f, -.05f);
    this->s_out[0].tag = SOCK_TAG_STATUS;
    /* data output */
    this->s_out[1].lpos = b2Vec2(.125f, -.05f);
    this->s_out[1].tag = SOCK_TAG_VALUE;

    this->width = .375f;
    this->height = .375f/2.f;
    float qw = this->width/2.f + 0.15f;
    float qh = this->height/2.f + 0.15f;
    this->query_sides[0].Set(0.f, 0.f); /* up */
    this->query_sides[1].Set(-qw, 0.f); /* left */
    this->query_sides[2].Set(0.f, -qh); /* down */
    this->query_sides[3].Set( qw, 0.f); /* right */
}

void
magsock::add_to_world()
{
    b2PolygonShape shape;
    b2PolygonShape sensor;

    shape.SetAsBox(.375f, .375f/2.f);
    sensor.SetAsBox(.375f, .1f, b2Vec2(0,.25f), 0);

    b2FixtureDef fd;
    fd.shape = &shape;
    fd.density = this->get_material()->density;
    fd.friction = this->get_material()->friction;
    fd.restitution = this->get_material()->restitution;
    fd.filter = world::get_filter_for_layer(this->prio);

    b2FixtureDef fd_sensor;
    fd_sensor.shape = &sensor;
    fd_sensor.density = .1f;
    fd_sensor.friction = FLT_EPSILON;
    fd_sensor.restitution = .0f;
    fd_sensor.filter.groupIndex = 0;
    fd_sensor.isSensor = true;
    fd_sensor.filter = world::get_filter_for_layer(this->prio);

    b2BodyDef bd;
    bd.type = this->get_dynamic_type();
    bd.position = this->_pos;
    bd.angle = this->_angle;

    this->body = W->b2->CreateBody(&bd);
    this->body->CreateFixture(&fd)->SetUserData(this);
    (this->sensor = this->body->CreateFixture(&fd_sensor))->SetUserData(this);
}

void
magsock::set_layer(int z)
{
    entity::set_layer(z);
    if (this->sensor) {
        this->sensor->SetFilterData(world::get_filter_for_layer(z));
    }
}

edevice*
magsock::solve_electronics()
{
    if (this->plug) {
        if (!this->plug->s_in[0].is_ready())
            return this->plug->s_in[0].get_connected_edevice();

        this->s_out[0].write(1.f);
        this->s_out[1].write(this->plug->s_in[0].get_value());
    } else {
        this->s_out[0].write(0.f);
        this->s_out[1].write(0.f);
    }

    return 0;
}

void
magsock::step()
{
    if (this->connected) {
        float adiff = tmath_adist(this->plug->get_angle(), this->get_angle());

        b2Vec2 pd = this->plug->get_position() - this->local_to_world(b2Vec2(0,.24f), 0);

        float dist = pd.Length();
        pd *= 1.f/dist;

        dist = fmaxf(dist, 0.1f);
        dist = fminf(dist, 0.9f);

        dist = powf(dist, 4.f);
        dist = 1.f-dist;

        pd *= -dist * .08 ;//*10.f;

        adiff = adiff * .05f * .1f;

        this->plug->get_body(0)->ApplyAngularImpulse(adiff);
        this->plug->get_body(0)->ApplyLinearImpulse(pd, this->plug->get_body(0)->GetWorldCenter());

        this->body->ApplyAngularImpulse(-adiff);

        pd *= -1.f;
        this->body->ApplyLinearImpulse(pd, this->body->GetWorldCenter());
    }
}

void
magsock::on_touch(b2Fixture *a, b2Fixture *b)
{
    if (this->connected)
        return;

    if (a == this->sensor) {
        entity *e = (entity*)b->GetUserData();

        if (e && e->g_id == O_MAGNETIC_PLUG) {
            this->connected = b;
            this->plug = static_cast<magplug*>(e);
            this->plug->sock = this;
        }
    }
}

void
magsock::on_untouch(b2Fixture *a, b2Fixture *b)
{
    if (a == this->sensor && b == this->connected) {
        this->plug->sock = 0;
        this->plug = 0;
        this->connected = 0;
    }
}

magplug::magplug()
    : sock(0)
{
    this->scaleselect = true;
    this->scalemodifier = 3.f;

    this->set_mesh(mesh_factory::get_mesh(MODEL_MAGPLUG));
    this->set_material(&m_colored);
    this->set_uniform("~color", .25f, .25f, .25f, 1.f);

    this->num_s_in = 1;
    this->num_s_out = 2;

    /* data input */
    this->s_in[0].lpos = b2Vec2(-.25f, .05f);
    this->s_in[0].tag = SOCK_TAG_VALUE;
    /* connection status */
    this->s_out[0].lpos = b2Vec2(0.f,  .05f);
    this->s_out[0].tag = SOCK_TAG_STATUS;
    /* fallback output */
    this->s_out[1].lpos = b2Vec2(.25f, .05f);
    this->s_out[1].tag = SOCK_TAG_VALUE;

    this->set_as_rect(.375f, .375f/2.f);

    this->query_sides[2].SetZero(); /* down */
}

edevice*
magplug::solve_electronics()
{
    if (this->sock) {
        this->s_out[0].write(1.f);
        this->s_out[1].write(0.f);
    } else {
        /* no socket attached */
        if (!this->s_in[0].is_ready())
            return this->s_in[0].get_connected_edevice();

        this->s_out[0].write(0.f);
        this->s_out[1].write(this->s_in[0].get_value());
    }

    return 0;
}

