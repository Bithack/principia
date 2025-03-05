#include "trampoline.hh"
#include "model.hh"
#include "world.hh"
#include "material.hh"
#include "settings.hh"

#define SPEED 50.f

trampoline::trampoline()
{
    this->set_flag(ENTITY_IS_STATIC,            true);
    this->set_flag(ENTITY_ALLOW_CONNECTIONS,    false);
    this->set_flag(ENTITY_DISABLE_LAYERS,       true);
    this->set_flag(ENTITY_CUSTOM_GHOST_UPDATE,  true);
    this->set_flag(ENTITY_DO_STEP,              true);

    this->set_mesh(mesh_factory::get_mesh(MODEL_TRAMPOLINE_BASE));
    this->set_material(&m_pv_colored);

    this->update_method = ENTITY_UPDATE_CUSTOM;

    if (settings["gamma_correct"]->v.b) {
        this->set_uniform("~color", .1f, .1f, .1f, 1.f);
    } else {
        this->set_uniform("~color", sqrtf(.1f), sqrtf(.1f), sqrtf(.1f), 1.f);
    }

    this->k = 700.f;

    this->width = 2.75f;
    this->pad_body = 0;

    this->pad = new tpad(this);
    this->add_child(this->pad);

    this->scaleselect = true;
    this->scalemodifier = 3.f;

    this->menu_scale = .5f;
    this->menu_pos = b2Vec2(0.f, -.5f);

    this->set_num_properties(2);
    this->properties[0].type = P_FLT;
    this->properties[0].v.f = 0.f;
    this->properties[1].type = P_FLT;
    this->properties[1].v.f = 0.f;

    this->num_s_in = 2;
    this->num_s_out = 1;

    this->s_in[0].ctype = CABLE_BLACK;
    this->s_in[0].lpos = b2Vec2(-.25f, 0.f);
    this->s_in[1].ctype = CABLE_RED;
    this->s_in[1].lpos = b2Vec2(0.f, 0.f);
    this->s_out[0].ctype = CABLE_RED;
    this->s_out[0].lpos = b2Vec2(.25f, 0.f);

    tmat4_load_identity(this->M);
    tmat3_load_identity(this->N);
}

bool
trampoline::allow_connection(entity *asker, uint8_t frame, b2Vec2 p)
{
    if (frame == 0) return false;
    return this->world_to_local(p, 1).y > -0.2f;
}

edevice*
trampoline::solve_electronics()
{
    if (!this->s_out[0].written()) {
        float displ = this->joint->GetJointTranslation();
        displ = 1.f - (displ-.25f)/(1.f-.125f-.25f+.5f);
        displ = fmaxf(0.0f, displ);
        displ = fminf(1.f, displ);
        this->s_out[0].write(displ);
    }

    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();
    if (!this->s_in[1].is_ready())
        return this->s_in[1].get_connected_edevice();

    float voltage = this->s_in[0].get_value();
    bool active = (bool)roundf(this->s_in[1].get_value());

    if (this->s_in[1].p == 0 || active) {
        this->joint->EnableMotor(true);
        this->input_voltage = voltage;
    } else {
        this->joint->EnableMotor(false);
    }

    return 0;
}

void
trampoline::step()
{
    float displ = this->joint->GetJointTranslation();

    displ = 1.f - (displ-.25f)/(1.f-.125f-.25f+.5f);

    //tms_infof("real dipls: %f", real_displ);

    displ = fmaxf(0.1f, displ);
    displ = fminf(1.f, displ);

    float factor = displ;

    displ *= this->k;

    b2Vec2 force = b2Vec2(0.f,1.f);
    force *= displ;

    if (this->input_voltage > 0.f) {
        if (this->status == 0) {
            if (displ*this->input_voltage < this->last_force)
                displ = this->last_force;
            else {
                displ *= this->input_voltage;
                this->last_force = displ;
            }

            if (factor <= 0.1f) {
                this->status = 1;
                this->last_factor = 0.1f;
                this->last_force = 0.f;
            }
        }

        if (this->status == 1) {
            if (factor < this->last_factor) {
                this->status = 0;
                this->last_force = displ*this->input_voltage;
            }
            this->last_factor = factor;
        }
    }

    this->joint->SetMaxMotorForce(displ);
    this->joint->SetMotorSpeed(SPEED);
}

void
trampoline::set_angle(float a)
{
    if (this->pad_body) {
        this->pad_body->SetAwake(true);
    }

    entity::set_angle(a);
}

void
trampoline::set_position(float x, float y, uint8_t frame/*=0*/)
{
    if (this->pad_body) {
        b2Vec2 diff = this->pad_body->GetPosition() - this->get_position();

        diff += b2Vec2(x,y);
        this->pad_body->SetTransform(diff, this->pad_body->GetAngle());
        //this->pad_body->SetAwake(true);
    }

    entity::set_position(x, y, frame);
}

void
trampoline::pre_write(void)
{
    this->properties[0].v.f = this->pad->get_position().x;
    this->properties[1].v.f = this->pad->get_position().y;
    entity::pre_write();
}

uint32_t
trampoline::get_num_bodies()
{
    return 2;
}

b2Body*
trampoline::get_body(uint8_t body)
{
    if (body == 0) return this->body;
    else return this->pad_body;
}

void
trampoline::add_to_world()
{
    this->status = 0;
    this->last_force = 0.f;
    b2Fixture *f;
    this->create_rect(b2_staticBody, 1.25f, .15f, this->get_material(), &f);

    b2BodyDef bd;
    bd.type = b2_dynamicBody;
    bd.position = b2Vec2(this->properties[0].v.f, this->properties[1].v.f);
    bd.angle = _angle;

    b2PolygonShape shape;
    shape.SetAsBox(1.5f, .375f, b2Vec2(0,-.25f), 0);

    b2FixtureDef fd;
    fd.restitution = 0.f;
    fd.friction = 0.5f;
    fd.density = 1.f;
    fd.shape = &shape;
    fd.filter = world::get_filter_for_layer(0);

    this->pad_body = W->b2->CreateBody(&bd);
    this->pad_body->CreateFixture(&fd)->SetUserData(this);
    this->pad_body->SetUserData((void*)1);

    b2PrismaticJointDef pjd;
    pjd.localAnchorA = b2Vec2(0.f, 0.f);
    pjd.localAnchorB = b2Vec2(0.f, 0.f);
    pjd.bodyA = this->body;
    pjd.bodyB = this->pad_body;
    pjd.localAxisA = b2Vec2(0.f, 1.f);
    pjd.lowerTranslation = .25f;
    pjd.upperTranslation = 1.f-.125+.5f;
    pjd.enableMotor = true;
    pjd.enableLimit = true;
    pjd.maxMotorForce = 0.f;
    pjd.motorSpeed = 0.f;
    pjd.referenceAngle = 0.f;

    this->joint = static_cast<b2PrismaticJoint*>(W->b2->CreateJoint(&pjd));
}

void
trampoline::remove_from_world()
{
    if (this->pad_body) W->b2->DestroyBody(this->pad_body);
    this->pad_body = 0;
    entity::remove_from_world();
}

void
trampoline::ghost_update(void)
{
    this->pad->update();
    entity_fast_update(this);
}

void
trampoline::update(void)
{
    this->pad->update();
    entity_fast_update(this);
    //entity::update();
}

/////////////////////

trampoline::tpad::tpad(trampoline *parent)
{
    this->parent = parent;
    this->set_mesh(mesh_factory::get_mesh(MODEL_TRAMPOLINE_PAD));
    this->set_material(&m_pv_colored);

    if (settings["gamma_correct"]->v.b)
        this->set_uniform("~color", .5f, .5f, .5f, 1.f);
    else
        this->set_uniform("~color", sqrtf(.5f), sqrtf(.5f), sqrtf(.5f), 1.f);
}

b2Vec2
trampoline::tpad::get_position()
{
    if (this->parent->pad_body)
        return this->parent->pad_body->GetPosition();
    return this->parent->_pos + b2Vec2(0, .5f);
}

float
trampoline::tpad::get_angle()
{
    return this->parent->get_angle();
}

void
trampoline::tpad::update()
{
    b2Vec2 p = this->get_position();
    float a = this->parent->get_angle();

    float s,c;
    tmath_sincos(a, &s, &c);

    this->M[0] = c;
    this->M[1] = s;
    this->M[4] = -s;
    this->M[5] = c;
    this->M[12] = p.x;
    this->M[13] = p.y;
    this->M[14] = this->prio * LAYER_DEPTH;

    tmat3_copy_mat4_sub3x3(this->N, this->M);
}

