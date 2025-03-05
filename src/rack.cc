#include "rack.hh"
#include "model.hh"
#include "material.hh"
#include "world.hh"
#include "gear.hh"

rack::rack()
{
    this->width = 2.f;

    this->rackbody = 0;
    this->joint = 0;
    this->type = ENTITY_RACK;
    this->menu_scale = 1.f/6.f;

    this->limits[0] = -4.f;
    this->limits[1] = 4.f;

    this->set_mesh(mesh_factory::get_mesh(MODEL_BOX0)); /* XXX */
    this->set_material(&m_rackhouse);
    //this->set_uniform("~color", 1.f, 1.f, 1.f, 1.f);

    tmat4_load_identity(this->M);
    tmat3_load_identity(this->N);

    this->rackent = tms_entity_alloc();
    tms_entity_init(this->rackent);
    tms_entity_set_mesh(this->rackent, mesh_factory::get_mesh(MODEL_BOX0)); /* XXX */
    tms_entity_set_material(this->rackent, &m_rack);
    tms_entity_set_uniform4f(this->rackent, "~color", 0.5f, .5f, .5f, 1.f);
    tmat4_load_identity(this->rackent->M);
    tmat3_load_identity(this->rackent->N);
    tms_entity_add_child(static_cast<struct tms_entity*>(this), this->rackent);
    this->set_num_properties(2);

    this->properties[0].v.f = 0.f;
    this->properties[1].v.f = 0.f;
}

void
rack::update(void)
{
    entity::update();
    tmat4_copy(this->rackent->M, this->M);

    if (this->joint) {
        float p = this->joint->GetJointTranslation();

        /* even if the shit goes outside, make sure it doesnt visually */
        if (p < this->limits[0]) p = this->limits[0];
        else if (p > this->limits[1]) p = this->limits[1];

        tmat4_translate(this->rackent->M, p, 0, 0);
        tmat3_copy_mat4_sub3x3(this->rackent->N, this->rackent->M);
    }
}

void
rack::update_limits(void)
{
    connection *c = this->conn_ll;

    float min = INFINITY;
    float max = -INFINITY;

    if (c) {
        do {
            if (c->type == CONN_RACK) {
                gear *g = static_cast<gear*>(c->e);
                float proj = this->get_projection(g->get_position());

                if (proj > 0.f) {
                    if (proj < min) min = proj;
                } else if (proj > max)
                    max = proj;
            }
        } while ((c = c->next[this == c->e ? 0 : 1]));
    }

    this->limits[0] = (min == INFINITY) ? -4.f : (min - 3.375f);
    this->limits[1] = (max == -INFINITY) ? 4.f : (max + 3.375f);

    tms_infof("limits %f %f", this->limits[0], this->limits[1]);

    this->joint->SetLimits(this->limits[0], this->limits[1]);
}

float
rack::get_projection(b2Vec2 p)
{
    p -= this->get_position();

    float angle = this->body->GetAngle();
    b2Vec2 axis = b2Vec2(cosf(angle), sinf(angle));

    return axis.x*p.x + axis.y*p.y;
}

bool
rack::point_in_range(b2Vec2 p)
{
    float projection = this->get_projection(p);
    return projection > this->limits[0]-4.f && projection < this->limits[1]+4.f;
}

void
rack::create_shape()
{
    if (this->rackbody) {
        for (b2Fixture *f = this->rackbody->GetFixtureList(), *next = 0;
                f; f=next) {
            next = f->GetNext();
            this->rackbody->DestroyFixture(f);
        }

        b2PolygonShape shape;
        shape.SetAsBox(4.f, 0.25f);

        b2FixtureDef fd;
        fd.shape = &shape;
        fd.density = 1.0f; /* XXX */
        fd.friction = .1f;
        fd.restitution = .1f;

        //(this->rackbody->CreateFixture(&fd))->SetUserData(this);
        (this->rackbody->CreateFixture(&fd))->SetUserData(this);
        this->set_layer(this->prio);
    }
}

void
rack::remove_from_world()
{
    W->b2->DestroyBody(this->rackbody);
    W->b2->DestroyBody(this->body);
    this->body = 0;
    this->rackbody = 0;
    this->joint = 0; /* automatically freed when the bodies are destroyed */
}

void
rack::set_angle(float a)
{
    if (this->rackbody)
        this->rackbody->SetAwake(true);

    entity::set_angle(a);
}

void
rack::set_position(float x, float y, uint8_t frame/*=0*/)
{
    if (this->rackbody) {
        this->rackbody->SetAwake(true);
    }

    entity::set_position(x, y, frame);
}

void
rack::pre_write(void)
{
    if (this->rackbody) {
        b2Vec2 rp = this->rackbody->GetPosition();
        rp -= this->get_position();
        this->properties[0].v.f = rp.x;
        this->properties[1].v.f = rp.y;
    }
    entity::pre_write();
}

void
rack::add_to_world()
{
    b2BodyDef bd;
    bd.type = b2_staticBody;
    bd.position = this->_pos;
    bd.angle = this->_angle;

    b2BodyDef bdr;
    bdr.type = b2_dynamicBody;
    bdr.position = this->_pos + b2Vec2(this->properties[0].v.f, this->properties[1].v.f);
    bdr.angle = this->_angle;

    b2PolygonShape shape;
    shape.SetAsBox(2.f, .5f);
    b2FixtureDef fd;
    fd.shape = &shape;
    fd.friction = .2f;
    fd.restitution = .1f;
    fd.filter = world::get_filter_for_layer(this->prio);

    this->body = W->b2->CreateBody(&bd);
    (this->body->CreateFixture(&fd))->SetUserData(this);

    this->rackbody = W->b2->CreateBody(&bdr);
    this->create_shape();

    b2PrismaticJointDef pjd;
    pjd.bodyA = this->body;
    pjd.bodyB = this->rackbody;
    pjd.localAnchorA = b2Vec2(0,0);
    pjd.localAnchorB = b2Vec2(0,0);
    pjd.collideConnected = false;
    pjd.upperTranslation = 4.f;
    pjd.lowerTranslation = -4.f;
    pjd.enableLimit = true;
    pjd.localAxisA = b2Vec2(1.f, 0.f);
    this->joint = static_cast<b2PrismaticJoint*>(W->b2->CreateJoint(&pjd));
}
