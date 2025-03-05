#include "rubberband.hh"
#include "material.hh"
#include "model.hh"
#include "world.hh"
#include "game.hh"
#include "linebuffer.hh"
#include "object_factory.hh"

#define WIDTH .075f
#define MIN_WIDTH 0.035f

#define RUBBER_Z_OFFSET .5f

rubberband::rubberband()
{
    this->width = .5f;
    this->height = .5f;

    float qw = this->width/2.f + 0.15f;
    float qh = this->height/2.f + 0.15f;
    this->query_sides[0].Set(0.f,  qh); /* up */
    this->query_sides[1].Set(-qw, 0.f); /* left */
    this->query_sides[2].Set(0.f, -qh); /* down */
    this->query_sides[3].Set( qw, 0.f); /* right */

    /*
    this->c_back.init_owned(0, this);
    this->c_back.type = CONN_GROUP;
    this->c_back.typeselect = 1; // allow the player to select between weld and pivot joints
    */

    this->c_side[0].init_owned(0, this);
    this->c_side[0].type = CONN_GROUP;
    this->c_side[1].init_owned(1, this);
    this->c_side[1].type = CONN_GROUP;
    this->c_side[2].init_owned(2, this);
    this->c_side[2].type = CONN_GROUP;
    this->c_side[3].init_owned(3, this);
    this->c_side[3].type = CONN_GROUP;

    this->c_front.init_owned(4, this);
    this->c_front.type = CONN_GROUP;
    this->c_front.typeselect = 1;
}

void rubberband::on_grab(game *g){};
void rubberband::on_release(game *g){};

connection *
rubberband::load_connection(connection &conn)
{
        /*
    if (conn.o_index == 0) {
        this->c_back = conn;
        return &this->c_back;
    } else */if (conn.o_index == 4) {
        this->c_front = conn;
        return &this->c_front;
    } else if (conn.o_index >= 0 && conn.o_index <= 3) {
        this->c_side[conn.o_index] = conn;
        return &this->c_side[conn.o_index];
    }

    return 0;
}

bool
rubberband::ReportFixture(b2Fixture *f)
{
    entity *e = (entity*)f->GetUserData();
    uint8_t fr = (uint8_t)(uintptr_t)f->GetBody()->GetUserData();

    /*
    int open = this->c_back.pending | (this->c_front.pending <<1);

    if (!f->IsSensor() && e && e!=this && f->TestPoint(this->q_point)
        && e->allow_connections() && e->allow_connection(this,fr,this->q_point)) {
        int dist = e->get_layer() - this->get_layer();
        if (abs(dist) == 1) {

            if (dist < 0) dist = 0;
            dist++;

            if (open & dist) {
                q_result = e;
                q_frame = fr;
                q_dir = dist;
                return false;
            }
        }
    }
    */
    bool front_open = this->c_front.pending;

    if (!f->IsSensor() && e && e!=this && f->TestPoint(this->q_point)
        && e->allow_connections() && e->allow_connection(this,fr,this->q_point)) {
        int dist = e->get_layer() - this->get_layer();
        if (dist == -1 && front_open) {
            q_result = e;
            q_fx = f;
            q_frame = fr;
            q_dir = -1;
            return false;
        }
    }

    return true;
}

void
rubberband::find_pairs()
{
    if (/*this->c_back.pending || */this->c_front.pending) {
        b2Vec2 p = this->get_position();
        b2AABB aabb;
        aabb.lowerBound.Set(p.x - .05f, p.y - .05f);
        aabb.upperBound.Set(p.x + .05f, p.y + .05f);

        this->q_result = 0;
        this->q_point = p;

        W->b2->QueryAABB(this, aabb);

        if (q_result != 0) {
            connection *c;
            if (this->q_dir == -1) {
                c = &this->c_front;

                if (c->pending) {
                    c->type = CONN_GROUP;
                    c->typeselect = this->find_pivot(0,false) == 0 && this->find_pivot(1, false) == 0;
                    c->o = q_result;
                    c->p = p;
                    c->f[1] = q_frame;
                    c->o_data = q_result->get_fixture_connection_data(q_fx);
                    G->add_pair(this, q_result, c);
                }
            }
        }
    }

    this->sidecheck4(this->c_side);
}

rubberband_1::rubberband_1()
{
    this->set_flag(ENTITY_DO_STEP, true);
    this->set_flag(ENTITY_DO_UPDATE_EFFECTS, true);

    this->menu_scale = .5f;
    this->menu_pos = b2Vec2(0.f, .5f);

    this->num_sliders = 2;

    this->set_mesh(mesh_factory::get_mesh(MODEL_RUBBEREND));
    this->set_material(&m_rocket);

    this->dconn.init_owned(5, this);
    this->dconn.fixed = true;
    this->dconn.type = CONN_CUSTOM;

    this->set_num_properties(2);

    this->properties[0].type = P_FLT; /* length */
    this->set_property(0, 2.f);

    this->properties[1].type = P_FLT; /* coefficient */
    this->set_property(1, 200.f);

    /* translation of the second body */
    /*
    this->properties[0].type = P_FLT;
    this->set_property(0, 0.f);

    this->properties[1].type = P_FLT;
    this->set_property(1, 10.f);

    this->properties[2].type = P_FLT;
    this->set_property(2, 2.f);
    */

    this->set_as_rect(.25f, .25f);
}

void
rubberband_1::update_effects()
{
    if (this->get_body(0)) {
        entity *o = this->dconn.o;

        float z1 = this->get_layer() * LAYER_DEPTH + RUBBER_Z_OFFSET;
        float z2 = o->get_layer() * LAYER_DEPTH + RUBBER_Z_OFFSET;
        b2Vec2 p1 = this->get_position();
        b2Vec2 p2 = o->get_position();

        float dist = b2Distance(p1, p2);
        float dist_norm = 1.f - tclampf((dist/(this->properties[0].v.f+5.f)), 0.f, 1.f);

        float w = (WIDTH * dist_norm) + MIN_WIDTH;

        linebuffer::add(
                p1.x, p1.y, z1,
                p2.x, p2.y, z2,
                0.3f, 0.3f, 0.3f, 6.f,
                0.3f, 0.3f, 0.3f, 6.f,
                w, w);
    }
}

rubberband_2::rubberband_2()
{
    this->d1 = 0;
    this->set_mesh(mesh_factory::get_mesh(MODEL_RUBBEREND));
    this->set_material(&m_rocket);
    //this->set_uniform("~color", .18f, .18f, .18f, 1.f);

    this->set_as_rect(.25f, .25f);
}

void
rubberband_1::update_frame(bool hard)
{
    if (hard) this->dconn.j = 0;
    if (this->dconn.o) this->dconn.create_joint(0);
}

void
rubberband_2::update_frame(bool hard)
{
    if (this->d1) {
        this->d1->update_frame(hard);
    }
}

connection*
rubberband_1::load_connection(connection &conn)
{
    if (conn.o_index == 5) {
        this->dconn = conn;
        this->dconn.fixed = true;
        return &this->dconn;
    }

    return rubberband::load_connection(conn);
}

/* create the prismatic connection */
void
rubberband_1::connection_create_joint(connection *c)
{
    if (c == &this->dconn) {
        ((rubberband_2*)c->o)->d1 = this;

        b2RopeJointDef rjd;
        rjd.collideConnected = true;
        rjd.bodyA = c->e->get_body(0);
        rjd.bodyB = c->o->get_body(0);
        rjd.maxLength = this->properties[0].v.f+5.f;
        //rjd.localAxisA = b2Vec2(0.f, -1.f);
        rjd.localAnchorA = c->e->local_to_body(b2Vec2(0.f, 0.f), 0);
        rjd.localAnchorB = c->o->local_to_body(b2Vec2(0.f, 0.f), 0);

        c->j = W->b2->CreateJoint(&rjd);
    }
}

void
rubberband::setup()
{
    this->get_body(0)->SetLinearDamping(.25f);
    this->get_body(0)->SetAngularDamping(.25f);
}

void
rubberband_1::construct()
{
    rubberband_2 *d2 = static_cast<rubberband_2*>(of::create(96));

    d2->_pos = this->_pos;
    d2->_pos -= b2Vec2(0.f, 1.f);
    W->add(d2);
    G->add_entity(d2);

    this->dconn.o = d2;
    this->dconn.fixed = true;

    G->apply_connection(&this->dconn, -1);
}

void
rubberband_1::set_layer(int z)
{
    if (this->body) {
        if (this->dconn.o) this->dconn.o->entity::set_layer(z);
    }
    entity::set_layer(z);
}

void
rubberband_2::set_layer(int z)
{
    if (this->body) {
        if (this->d1) this->d1->set_layer(z);
    } else
        entity::set_layer(z);
}

void
rubberband_1::on_slider_change(int s, float value)
{
    float v;
    if (s == 0) { /* length */
        v = 1.f + value * 5.f;
    } else { /* Coefficient */
        v = .5f + value * 400.f;
    }
    this->properties[s].v.f = v;
    G->show_numfeed(v);
}

void
rubberband_1::step(void)
{
    if (this->dconn.j) {
        b2Vec2 v = this->get_position() - dconn.o->get_position();
        float dist = v.Length();
        v *= 1.f/dist;

        if (dist > this->properties[0].v.f) {
            dist -= this->properties[0].v.f;

            v.x*=dist*this->properties[1].v.f;
            v.y*=dist*this->properties[1].v.f;

            this->get_body(0)->ApplyForce(-v, this->get_position());
            this->dconn.o->get_body(0)->ApplyForce(v, dconn.o->get_position());
        }
    }
}
