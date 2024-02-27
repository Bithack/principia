#include "edevice.hh"
#include "world.hh"
#include "game.hh"
#include "mini_transmitter.hh"

uint64_t edev_step_count = 0;

bool
socket_out::written_mt()
{
    mini_transmitter *mt = static_cast<mini_transmitter*>(this->p);

    return (mt->edev_step == edev_step_count);
}

void
socket_out::write_mt(float v)
{
    mini_transmitter *mt = static_cast<mini_transmitter*>(this->p);
    mt->edev_step = edev_step_count;
    mt->set_value(v);
}

uint8_t
edevice::get_socket_index(isocket *s)
{
    if (s >= &s_in[0] && s < &s_in[this->num_s_in])
        return (uint8_t)(((uintptr_t)s - (uintptr_t)&s_in[0])/sizeof(socket_in));
    if (s >= &s_out[0] && s < &s_out[this->num_s_out])
        return (uint8_t)((((uintptr_t)s - (uintptr_t)&s_out[0])/sizeof(socket_out)) | 0x80);

    return 0;
}

void
socket_out::write(float v)
{
    //tms_infof("Writing %.2f to %p", v, this->p);
    if (p) {
        plug_base *o;
        if ((o = p->get_other())) {
            /* we have a plug to output to */
            socket_in *s = (socket_in*)o->s;

            if (o->s) {
                s->step_count = edev_step_count;
                s->value = v;
            }
        } else if (this->p->plug_type == PLUG_MINI_TRANSMITTER) {
            this->write_mt(v);
        }
    }
}

void
edevice::recreate_all_cable_joints()
{
    for (int x=0; x<num_s_in; x++) {
        if (this->s_in[x].p && this->s_in[x].p->c)
            this->s_in[x].p->c->create_joint();
    }
    for (int x=0; x<num_s_out; x++) {
        if (this->s_out[x].p && this->s_out[x].p->c)
            this->s_out[x].p->c->create_joint();
    }
}

void
brcomp::set_layer(int z)
{
    for (int x=0; x<num_s_in; x++) {
        if (s_in[x].p) {
            s_in[x].p->set_layer(z);
        }
    }
    for (int x=0; x<num_s_out; x++) {
        if (s_out[x].p) {
            s_out[x].p->set_layer(z);
        }
    }
    entity::set_layer(z);
}

void
ecomp::set_layer(int z)
{
    for (int x=0; x<num_s_in; x++) {
        if (s_in[x].p) {
            s_in[x].p->set_layer(z);
        }
    }
    for (int x=0; x<num_s_out; x++) {
        if (s_out[x].p) {
            s_out[x].p->set_layer(z);
        }
    }
    entity::set_layer(z);
}

connection *
ecomp_multiconnect::load_connection(connection &conn)
{
    this->c_side[conn.o_index] = conn;
    return &this->c_side[conn.o_index];
}

void
ecomp_multiconnect::find_pairs()
{
    this->sidecheck4(this->c_side);
}

void
ecomp_multiconnect::set_as_rect(float width, float height)
{
    composable::set_as_rect(width, height);

    const float qw = width/2.f+0.15f;
    const float qh = height/2.f+0.15f;
    this->query_sides[0].Set(0.f,  qh); /* up */
    this->query_sides[1].Set(-qw, 0.f); /* left */
    this->query_sides[2].Set(0.f, -qh); /* down */
    this->query_sides[3].Set( qw, 0.f); /* right */
}

connection *
edev_multiconnect::load_connection(connection &conn)
{
    this->c_side[conn.o_index] = conn;
    return &this->c_side[conn.o_index];
}

void
edev_multiconnect::find_pairs()
{
    this->sidecheck4(this->c_side);
}

void
edev::set_layer(int z)
{
    for (int x=0; x<num_s_in; x++) {
        if (s_in[x].p) {
            s_in[x].p->set_layer(z);
        }
    }
    for (int x=0; x<num_s_out; x++) {
        if (s_out[x].p) {
            s_out[x].p->set_layer(z);
        }
    }
    entity::set_layer(z);
}

connection *
brcomp_multiconnect::load_connection(connection &conn)
{
    this->c_side[conn.o_index] = conn;
    return &this->c_side[conn.o_index];
}

void
brcomp_multiconnect::find_pairs()
{
    this->sidecheck4(this->c_side);
}

void
brcomp_multiconnect::set_as_rect(float width, float height)
{
    composable::set_as_rect(width, height);

    const float qw = width/2.f+0.15f;
    const float qh = height/2.f+0.15f;
    this->query_sides[0].Set(0.f,  qh); /* up */
    this->query_sides[1].Set(-qw, 0.f); /* left */
    this->query_sides[2].Set(0.f, -qh); /* down */
    this->query_sides[3].Set( qw, 0.f); /* right */
}

void
ecomp_simpleconnect::find_pairs()
{
    if (this->query_vec.Length() > 0.f && this->c.pending) {
        this->query_result = 0;

        W->b2->RayCast(this, this->local_to_world(this->query_pt, 0), this->local_to_world(this->query_vec+this->query_pt, 0));

        if (this->query_result) {
            this->c.o = this->query_result;
            this->c.f[1] = this->query_frame;
            this->c.angle = atan2f(this->query_vec.y, this->query_vec.x);
            this->c.o_data = this->query_result->get_fixture_connection_data(this->query_result_fx);
            b2Vec2 vv = this->query_vec;
            vv*=.5f;
            vv += this->query_pt;
            this->c.p = this->local_to_world(vv, 0);
            G->add_pair(this, this->query_result, &this->c);
        }
    }
}

float32
ecomp_simpleconnect::ReportFixture(b2Fixture *f, const b2Vec2 &pt, const b2Vec2 &nor, float32 fraction)
{
    if (f->IsSensor()) {
        return -1.f;
    }

    b2Body *b = f->GetBody();
    entity *e = (entity*)f->GetUserData();

    if (e && e != this && e->allow_connections() && e->get_layer() == this->get_layer() && (e->layer_mask & 6) != 0) {
        this->query_result = e;
        this->query_result_fx = f;
        this->query_frame = (uint8_t)(uintptr_t)b->GetUserData();
    }

    return -1;
}

void
edev_simpleconnect::find_pairs()
{
    if (this->query_vec.Length() > 0.f && this->c.pending) {
        this->query_result = 0;

        W->b2->RayCast(this, this->local_to_world(this->query_pt, 0), this->local_to_world(this->query_vec+this->query_pt, 0));

        if (this->query_result) {
            this->c.o = this->query_result;
            this->c.f[1] = this->query_frame;
            this->c.angle = atan2f(this->query_vec.y, this->query_vec.x);
            this->c.o_data = this->query_result->get_fixture_connection_data(this->query_result_fx);
            b2Vec2 vv = this->query_vec;
            vv*=.5f;
            vv += this->query_pt;
            this->c.p = this->local_to_world(vv, 0);
            G->add_pair(this, this->query_result, &this->c);
        }
    }
}

float32
edev_simpleconnect::ReportFixture(b2Fixture *f, const b2Vec2 &pt, const b2Vec2 &nor, float32 fraction)
{
    if (f->IsSensor()) {
        return -1.f;
    }

    b2Body *b = f->GetBody();
    entity *e = (entity*)f->GetUserData();

    if (e && e != this && e->allow_connections() && e->get_layer() == this->get_layer() && (e->layer_mask & 6) != 0) {
        this->query_result = e;
        this->query_result_fx = f;
        this->query_frame = (uint8_t)(uintptr_t)b->GetUserData();
    }

    return -1;
}
