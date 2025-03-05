#include "seesaw.hh"
#include "model.hh"
#include "material.hh"
#include "world.hh"
#include "game.hh"

seesaw::seesaw()
{
    this->width = 4.f;
    this->menu_scale = 1.f;

    this->set_mesh(mesh_factory::get_mesh(MODEL_SEESAW));
    this->set_material(&m_gen);

    this->c.init_owned(0, this);
    this->c.type = CONN_CUSTOM;

    this->c_floor.init_owned(1, this);
    this->c_floor.type = CONN_PLATE;

    tmat4_load_identity(this->M);
    tmat3_load_identity(this->N);
}

connection *
seesaw::load_connection(connection &conn)
{
    if (conn.o_index == 0) {
        this->c = conn;
        return &this->c;
    } else {
        this->c_floor = conn;
        return &this->c_floor;
    }
}

void
seesaw::connection_create_joint(connection *c)
{
    b2RevoluteJointDef rjd;
    rjd.collideConnected = true;
    rjd.bodyA = this->body;
    rjd.bodyB = c->o->get_body(c->f[1]);
    rjd.localAnchorA = b2Vec2(0.f, 0.f);
    rjd.localAnchorB = c->o->local_to_body(c->p_s, c->f[1]);
    b2World *w = this->body->GetWorld();
    c->j = w->CreateJoint(&rjd);
}

bool
seesaw::ReportFixture(b2Fixture *f)
{
    if (f->IsSensor()) {
        return true;
    }

    entity *e = (entity*)f->GetUserData();

    if (!f->IsSensor() && e && e!=this && f->TestPoint(this->query_point)
        && (e->type == ENTITY_PLANK
            || e->type == ENTITY_WHEEL
            ) && e->get_layer() == this->get_layer()) {
        query_result = e;
        query_result_fx = f;
        query_frame = VOID_TO_UINT8(f->GetBody()->GetUserData());
        return false;
    }

    return true;
}

float32
seesaw::ReportFixture(b2Fixture *f, const b2Vec2 &pt, const b2Vec2 &nor, float32 fraction)
{
    if (f->IsSensor()) {
        return -1.f;
    }

    b2Body *b = f->GetBody();
    entity *e = (entity*)f->GetUserData();

    if (e && e->allow_connections()) {
        this->query_result = e;
        this->query_frame = VOID_TO_UINT8(b->GetUserData());
        query_result_fx = f;
        this->query_fraction = fraction;
    }

    return -1;
}

void
seesaw::find_pairs()
{
    if (this->c.pending && this->body) {
        b2Vec2 p;
        this->query_point = p = this->local_to_world(b2Vec2(0.f, 0.f), 0);
        this->query_result = 0;
        this->query_frame = 0;

        b2AABB aabb;
        aabb.lowerBound.Set(p.x - .05f, p.y - .05f);
        aabb.upperBound.Set(p.x + .05f, p.y + .05f);

        W->b2->QueryAABB(this, aabb);

        if (this->query_result != 0) {
            this->c.o = this->query_result;
            this->c.f[0] = 0;
            this->c.f[1] = this->query_frame;
            this->c.p = p;
            this->c.o_data = this->query_result->get_fixture_connection_data(this->query_result_fx);
            G->add_pair(this, this->query_result, &this->c);
        }
    }

    if (this->c_floor.pending) {
        this->query_result = 0;
        this->query_frame = 0;

        W->b2->RayCast(this, this->local_to_world(b2Vec2(0.f, -1.f), 0), this->local_to_world(b2Vec2(0, -1.25f), 0));

        if (this->query_result) {
            this->c_floor.o = this->query_result;
            this->c_floor.f[0] = 0;
            this->c_floor.f[1] = this->query_frame;
            b2Vec2 vv = b2Vec2(0.f, -.25f);
            vv *= this->query_fraction;
            vv.y -= 1.f;
            //vv*=.5f;
            this->c_floor.p = this->local_to_world(vv, 0);
            this->c.o_data = this->query_result->get_fixture_connection_data(this->query_result_fx);
            G->add_pair(this, this->query_result, &this->c_floor);
        }
    }
}

void
seesaw::setup()
{
}

void
seesaw::add_to_world()
{
    b2BodyDef bd;
    bd.type = this->get_dynamic_type();
    bd.position = this->_pos;
    bd.angle = this->_angle;
    b2Body *b = W->b2->CreateBody(&bd);
    this->body = b;

    float width = .5f;
    float height = .125f;

    b2PolygonShape box;
    box.SetAsBox(width, height, b2Vec2(0.f, -1.f), 0.f);

    b2FixtureDef fd;

    this->width = width;
    this->height = height;

    fd.shape = &box;
    fd.density = this->get_material()->density;
    fd.friction = this->get_material()->friction;
    fd.restitution = this->get_material()->restitution;
    fd.filter = world::get_filter_for_layer(this->prio, this->layer_mask);

    (this->body->CreateFixture(&fd))->SetUserData(this);
}
