#include "wmotor.hh"
#include "game.hh"
#include "world.hh"
#include "model.hh"
#include "material.hh"

#define SPEED 20.f
#define TORQUE 1000.f

class wmotor_query_cb : public b2QueryCallback
{
  public:
    entity *result;
    b2Fixture *result_fx;
    uint8_t result_frame;
    b2Vec2 point;

    wmotor_query_cb(b2Vec2 point)
    {
        this->result = 0;
        this->point = point;
    }

    bool ReportFixture(b2Fixture *f)
    {
        entity *e = static_cast<entity*>(f->GetUserData());

        if (!f->IsSensor() && e && f->TestPoint(this->point)
            && (e->type == ENTITY_PLANK
                || e->type == ENTITY_WHEEL
                )) {
            result = e;
            result_fx = f;
            result_frame = (uint8_t)(uintptr_t)f->GetBody()->GetUserData();
            return false;
        }

        return true;
    }
};

wmotor::wmotor()
{
    this->set_flag(ENTITY_IS_STATIC,            true);
    this->set_flag(ENTITY_ALLOW_ROTATION,       false);
    this->set_flag(ENTITY_ALLOW_CONNECTIONS,    false);
    this->set_flag(ENTITY_DISABLE_LAYERS,       true);

    this->set_mesh(mesh_factory::get_mesh(MODEL_WMOTOR));
    this->set_material(&m_colored);

    tmat4_load_identity(this->M);
    tmat3_load_identity(this->N);

    this->set_num_properties(0);
    //this->properties[0].v.f = .5f;

    this->update_method = ENTITY_UPDATE_STATIC;

    this->c.init_owned(0, this);

    this->set_uniform("~color", .1f, 0.1f, 0.1f, 1.f);
}

void
wmotor::update()
{
    b2Vec2 p;

    if (this->body) {
        p = this->body->GetPosition();
    } else {
        p = this->_pos;
    }

    tmat4_load_identity(this->M);
    tmat4_translate(this->M, p.x, p.y, -.5f);
}

void
wmotor::add_to_world()
{
    this->set_flag(ENTITY_IS_STATIC, !(W->level.type == LCAT_PUZZLE));
    this->create_circle(b2_staticBody, .375f, this->material);
    this->body->GetFixtureList()[0].SetSensor(true);
}

void
wmotor::find_pairs()
{
    if (this->c.pending) {
        b2Vec2 p = this->local_to_world(b2Vec2(0,0), 0);
        b2AABB aabb;
        wmotor_query_cb cb(p);
        aabb.lowerBound.Set(p.x - .05f, p.y - .05f);
        aabb.upperBound.Set(p.x + .05f, p.y + .05f);
        W->b2->QueryAABB(&cb, aabb);

        if (cb.result != 0) {
            this->c.o = cb.result;
            this->c.p = p;
            this->c.f[0] = 0;
            this->c.f[1] = cb.result_frame;
            this->c.o_data = cb.result->get_fixture_connection_data(cb.result_fx);
            G->add_pair(this, cb.result, &this->c);
        }
    }
}

void
wmotor::connection_create_joint(connection *c)
{
    b2World *w = this->body->GetWorld();

    b2RevoluteJointDef rjd;
    rjd.collideConnected = true;
    rjd.maxMotorTorque = 0.f;
    rjd.motorSpeed = 0.f;
    rjd.enableMotor = false;
    rjd.localAnchorA = this->local_to_body(b2Vec2(0.f, 0.f), 0);
    rjd.referenceAngle = (c->o->get_body(c->f[1])->GetAngle()- this->get_body(0)->GetAngle());

    if (c->o->type == ENTITY_PLANK && !c->o->gr) {
        rjd.localAnchorB = c->o->world_to_body(this->get_position(), c->f[1]);
        rjd.localAnchorB.y = 0.f; /* XXX */
    } else if (c->o->type == ENTITY_WHEEL || c->o->g_id == O_GEARBOX) {
        /* get the centroid of the object */
        rjd.localAnchorB = c->o->local_to_body(b2Vec2(0.f,0.f), c->f[1]);
    } else {
        rjd.localAnchorB = c->o->local_to_body(c->p_s, c->f[1]);
    }
    rjd.bodyA = this->get_body(0);
    rjd.bodyB = c->o->get_body(c->f[1]);

    if (W->level.version >= LEVEL_VERSION_1_5 && W->level.joint_friction > 0.f) {
        rjd.enableMotor = true;
        rjd.maxMotorTorque = W->level.joint_friction;
        rjd.motorSpeed = 0.f;
    }

    c->j = w->CreateJoint(&rjd);
}

connection *
wmotor::load_connection(connection &conn)
{
    this->c = conn;
    return &this->c;
}

/*
bool
wmotor::solve_electronics()
{
    if (!this->s_in[0].is_ready())
        return false;
    if (!this->s_in[1].is_ready())
        return false;

    float v = this->s_in[0].get_value();
    float tradeoff = this->s_in[1].p ? this->s_in[1].get_value() : this->properties[0].v.f;

    tradeoff = .02f + tradeoff*.96f;

    float voltage = tradeoff * v * SPEED;
    float current = (1.f-tradeoff) * v * TORQUE;

    b2RevoluteJoint *j = this->c.j;

    if (j) {
        if (voltage <= 0.f || current <= 0.f) {
            j->EnableMotor(false);
        } else {
            j->SetMotorSpeed(voltage);
            j->SetMaxMotorTorque(current);
            j->EnableMotor(true);
        }
    }
    return true;
}
*/
