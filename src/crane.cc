#include "crane.hh"
#include "game.hh"
#include "model.hh"
#include "linebuffer.hh"
#include "ledbuffer.hh"

crane::crane()
{
    this->dragging = false;
    this->pc.init_owned(0, this); this->pc.type = CONN_CUSTOM;
    this->rc.init_owned(1, this); this->rc.type = CONN_CUSTOM;
    this->c_side[0].init_owned(2, this); this->c_side[0].type = CONN_GROUP;
    this->c_side[1].init_owned(3, this); this->c_side[1].type = CONN_GROUP;
    this->c_side[2].init_owned(4, this); this->c_side[2].type = CONN_GROUP;
    this->c_side[3].init_owned(5, this); this->c_side[3].type = CONN_GROUP;

    this->update_method = ENTITY_UPDATE_CUSTOM;

    this->set_flag(ENTITY_IS_BETA, true);
    this->set_flag(ENTITY_DO_STEP, true);
    this->set_flag(ENTITY_DO_UPDATE_EFFECTS, true);

    this->set_mesh(mesh_factory::get_mesh(MODEL_CRANE));
    this->set_material(&m_edev_dark);

    this->width = .475f;
    this->height = .175f;
    float qw = this->width/2.f + 0.15f;
    float qh = this->height/2.f + 0.15f;
    this->query_sides[0].Set(0.f,  qh); /* up */
    this->query_sides[1].Set(-qw, 0.f); /* left */
    this->query_sides[2].Set(0.f, 0.f); /* down */
    this->query_sides[3].Set( qw, 0.f); /* right */

    this->pulley = 0;
    this->pjoint = 0;
    this->rjoint = 0;
    this->desired_pos = -CRANE_MIN_LENGTH;

    this->scaleselect = true;

    this->num_s_in = 1;
    this->num_s_out = 1;

    this->s_in[0].lpos = b2Vec2(.125f, -.025f); // rope length fraction
    this->s_out[0].lpos = b2Vec2(.125f+.25f, -.025f); // rope pos
    this->hit = false;
    this->go_up = false;
    this->rev = 0;
}

void
crane::add_to_world()
{
    b2PolygonShape shape_base;
    b2CircleShape shape_rev;
    shape_base.SetAsBox(.475f, .175f);
    shape_rev.m_radius = .15f;

    b2FixtureDef fd;
    fd.shape = &shape_base;
    fd.density = this->get_material()->density;
    fd.friction = this->get_material()->friction;
    fd.restitution = this->get_material()->restitution;
    fd.filter = world::get_filter_for_layer(this->prio);

    b2FixtureDef fd_rev;
    fd_rev.shape = &shape_rev;
    fd_rev.density = 2.f;
    fd_rev.friction = this->get_material()->friction;
    fd_rev.restitution = this->get_material()->restitution;
    //fd_rev.filter = world::get_filter_for_layer(this->prio);
    fd_rev.filter = world::get_filter_for_layer(-1);

    b2BodyDef bd;
    bd.type = this->get_dynamic_type();
    bd.position = this->_pos;
    bd.angle = this->_angle;

    b2BodyDef bd_rev;
    bd_rev.type = this->get_dynamic_type();
    bd_rev.position = this->_pos;
    bd_rev.angle = this->_angle;

    this->body = W->b2->CreateBody(&bd);
    this->body->CreateFixture(&fd)->SetUserData(this);

    if (!W->is_paused() && W->level.type == LCAT_ADVENTURE) {
        this->rev = W->b2->CreateBody(&bd_rev);
        this->rev->CreateFixture(&fd_rev)->SetUserData(this);
    }
}

void
crane::remove_from_world()
{
    tms_infof("removing from world");
    if (this->body) {
        W->b2->DestroyBody(this->body);
    }

    if (this->rev) {
        W->b2->DestroyBody(this->rev);
    }
    if (W->is_playing() && this->pulley) {
        //this->pulley->remove_from_world();
    }

    this->body = 0;
    this->rev = 0;
    this->fx = 0;
}

bool
crane::ReportFixture(b2Fixture *f)
{
    entity *e = static_cast<entity*>(f->GetUserData());

    if (e && e != this && e->get_layer() == this->get_layer()
        && f->TestPoint(this->q_point)) {
        this->q_result = e;
        return false;
    }

    return true;
}

void
crane::find_pairs()
{
    this->sidecheck4(this->c_side);
}

void
crane::setup()
{
    if (!W->is_adventure()) {
        return;
    }

    this->hit = false;
    this->dragging = false;
    this->pulley = new crane_pulley(this);
    b2Vec2 p = this->get_position();
    p.y -= 1.f;

    this->pulley->set_position(p.x, p.y);

    W->add(this->pulley);
    G->add_entity(this->pulley);
    this->pc.o = this->pulley;
    this->pc.fixed = true;

    this->rc.o = this;
    this->rc.fixed = true;

    G->apply_connection(&this->pc, -1);
    G->apply_connection(&this->rc, -1);
    this->desired_pos = -CRANE_MIN_LENGTH;

    this->pulley->set_layer(this->get_layer());
    this->rev->SetTransform(b2Vec2(this->_pos.x, this->_pos.y), this->rev->GetAngle());
}

void
crane::on_pause()
{
    if (!W->is_adventure()) {
        return;
    }

    if (this->pulley) {
        W->remove(this->pulley);
        G->remove_entity(this->pulley);
        this->pulley = 0;
    }

    this->rev = 0;
}

connection *
crane::load_connection(connection &conn)
{
    if (conn.o_index == 0) {
        this->pc = conn;
        return &this->pc;
    } else if (conn.o_index == 1) {
        this->rc = conn;
        return &this->rc;
    } else {
        this->c_side[conn.o_index-2] = conn;
        return &this->c_side[conn.o_index-2];
    }
}

crane_pulley::crane_pulley(crane *parent)
{
    this->set_flag(ENTITY_DISABLE_LAYERS,   true);
    this->set_flag(ENTITY_ALLOW_ROTATION,   false);
    this->set_flag(ENTITY_IS_CRANE_PULLEY,  true);

    this->set_mesh(mesh_factory::get_mesh(MODEL_BULLET));
    this->set_material(&m_edev_dark);
    this->parent = parent;

    tmat4_load_identity(this->M);
    tmat3_load_identity(this->N);

    this->width = .1f;
    this->height = .1f;

    float qw = this->width/2.f + 0.15f;
    float qh = this->height/2.f + 0.15f;
    this->query_sides[0].Set(0.f,  qh); /* up */
    this->query_sides[1].Set(-qw, 0.f); /* left */
    this->query_sides[2].Set(0.f, -qh); /* down */
    this->query_sides[3].Set( qw, 0.f); /* right */

    this->c_side[0].init_owned(0, this); this->c_side[0].type = CONN_CUSTOM;
    this->c_side[1].init_owned(1, this); this->c_side[1].type = CONN_CUSTOM;
    this->c_side[2].init_owned(2, this); this->c_side[2].type = CONN_CUSTOM;
    this->c_side[3].init_owned(3, this); this->c_side[3].type = CONN_CUSTOM;
}

void
crane_pulley::add_to_world()
{
    this->create_circle(b2_dynamicBody, .1, this->material);
    this->body->SetLinearDamping(5.f);
    this->body->SetAngularDamping(5.f);
}

void
crane::connection_create_joint(connection *c)
{
    if (c == &this->pc) {
        b2PrismaticJointDef pjd;
        pjd.collideConnected = true;
        pjd.bodyA = this->pulley->get_body(0);
        pjd.bodyB = this->rev;
        pjd.maxMotorForce = 0.f;
        pjd.motorSpeed = 0.f;
        pjd.enableLimit = true;
        pjd.localAxisA.Set(0.f, -1.f); // TODO: Base this on gravity or crane angle?
        pjd.localAnchorA = this->pulley->local_to_body(b2Vec2(0,0),0);
        pjd.localAnchorB = this->local_to_body(b2Vec2(0,0),0);
        pjd.upperTranslation = -CRANE_MIN_LENGTH;
        pjd.lowerTranslation = -CRANE_MAX_LENGTH;

        this->pjoint = static_cast<b2PrismaticJoint*>(c->j = W->b2->CreateJoint(&pjd));
    } else if (c == &this->rc) {
        b2RevoluteJointDef rjd;
        rjd.enableMotor = true;
        rjd.maxMotorTorque = 10.f;
        rjd.motorSpeed = 0.f;
        rjd.collideConnected = false;
        rjd.bodyA = this->get_body(0);
        rjd.bodyB = this->rev;
        rjd.localAnchorA = c->e->local_to_body(b2Vec2(0,0),0);
        rjd.localAnchorB = c->o->local_to_body(b2Vec2(0,0),0);
        rjd.referenceAngle = 0.f;

        this->rjoint = static_cast<b2RevoluteJoint*>(c->j = W->b2->CreateJoint(&rjd));
    }
}

void
crane::set_position(float x, float y, uint8_t frame/*=0*/)
{
    if (!this->flag_active(ENTITY_CAN_MOVE)) {
        return;
    }

    if (this->body) {
        this->body->SetTransform(b2Vec2(x,y), this->body->GetAngle());

        for (b2Fixture *f = this->body->GetFixtureList(); f; f=f->GetNext()) {
            f->Refilter();
        }
    } else {
        this->_pos = b2Vec2(x,y);
    }

    if (this->rev) {
        this->rev->SetTransform(b2Vec2(x,y), this->rev->GetAngle());

        for (b2Fixture *f = this->rev->GetFixtureList(); f; f=f->GetNext()) {
            f->Refilter();
        }
    }
}

edevice*
crane::solve_electronics()
{
    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();

    if (this->s_in[0].p) {
        float v = this->s_in[0].get_value();
        this->desired_pos = -((v * (CRANE_MAX_LENGTH - CRANE_MIN_LENGTH)));
    }

    //float rf = (-this->pjoint->GetJointTranslation()-CRANE_MIN_LENGTH) / (CRANE_MAX_LENGTH-CRANE_MIN_LENGTH);
    float rf = -this->desired_pos / CRANE_MAX_LENGTH;
    this->s_out[0].write(tclampf(rf, 0.f, 1.f));

    return 0;
}

connection*
crane_pulley::load_connection(connection &conn)
{
    this->c_side[conn.o_index] = conn;
    return &this->c_side[conn.o_index];
}

void
crane_pulley::find_pairs()
{
    this->sidecheck4(this->c_side);
}

void
crane::step()
{
    if (!W->is_adventure()) {
        return;
    }

    bool lifting = false;

    for (int x=0; x<4; x++) {
        if (this->pulley && !this->pulley->c_side[x].pending) {
            lifting = true;
            break;
        }
    }

    if (lifting) {
        ((b2RevoluteJoint*)this->rc.j)->EnableMotor(true);
    } else  {
        ((b2RevoluteJoint*)this->rc.j)->EnableMotor(false);
    }

    if (!dragging) {
        if (this->go_up) {
            if (this->pjoint->GetJointTranslation()>-1.2f) {
                this->go_up = false;
            } else {
                this->desired_pos = -CRANE_MIN_LENGTH;
            }
        }
        float target = this->desired_pos;
        float dd = target - this->pjoint->GetJointTranslation() - CRANE_MIN_LENGTH;
        float speed = (dd * CRANE_MAX_SPEED) * 1.f/1.5f;

        float f = 1.f;
        if (speed < 0.f) {
            f = .1f;
        }

        if (!lifting && speed < 0.f) {
            f = 0.01f;
        }

        this->pjoint->SetMotorSpeed(speed);
        this->pjoint->SetMaxMotorForce(CRANE_MAX_FORCE*f);
        this->pjoint->EnableMotor(true);
    } else {
        this->pjoint->EnableMotor(false);
    }

    if (this->hit) {
        this->go_up = !this->go_up;
        if (this->go_up) {
        } else {
            this->desired_pos = -tclampf(b2Distance(this->pulley->get_position(), this->get_position())-0.5f, CRANE_MIN_LENGTH, CRANE_MAX_LENGTH);
        }
        this->hit = false;
    }
}

void
crane::on_absorb()
{
    if (this->pulley) {
        this->pulley->disconnect_all();
        this->pulley->remove_from_world();
        G->remove_entity(this->pulley);
        W->remove(this->pulley);

        connection *c = this->pulley->conn_ll;
        while (c) {
            connection *next = c->get_next(this->pulley);

            if (c->owned && c->e == this->pulley) {
                W->erase_connection(c);
                c->o->remove_connection(c);
            } else if (c->owned) {
                /* owned but not by us */
                W->erase_connection(c);
                c->e->remove_connection(c);
            }
            c = next;
        }

        delete this->pulley;
        this->pulley = 0;
    }
}

void
crane::update_effects()
{
    if (!W->is_adventure()) {
        return;
    }

    if (this->get_body(0) && !W->is_paused()) {
        entity *o = this->pc.o;

        float z1 = this->get_layer() * LAYER_DEPTH;
        float z2 = o->get_layer() * LAYER_DEPTH;
        b2Vec2 p1 = this->get_position();
        b2Vec2 p2 = o->get_position();

        //float w = .135f - (.075f * (b2Distance(p1, p2)/CRANE_MAX_LENGTH));
        float w = .05f;

        linebuffer::add(
                p1.x, p1.y, z2,
                p2.x, p2.y, z1,
                0.3f, 0.3f, 0.3f, 6.f,
                0.3f, 0.3f, 0.3f, 6.f,
                w, w);

        float z = this->get_layer() * LAYER_DEPTH + LED_Z_OFFSET;

        b2Vec2 p = this->local_to_world(b2Vec2(-.325f, 0.f), 0);
        ledbuffer::add(p.x, p.y, z, this->go_up ? 1.f : 0.f);
    }
}

void
crane::set_layer(int z)
{
    entity::set_layer(z);
    if (this->pulley) {
        this->pulley->set_layer(z);
    }
}

void
crane_pulley::on_grab_playing()
{
    if (!W->is_adventure()) {
        return;
    }

    crane *c = (crane*)this->parent;
    c->dragging = true;
}

void
crane_pulley::on_release_playing()
{
    if (!W->is_adventure()) {
        return;
    }

    crane *c = (crane*)this->parent;
    c->dragging = false;
    float length = tclampf(b2Distance(this->get_position(), c->get_position())-0.5f, CRANE_MIN_LENGTH, CRANE_MAX_LENGTH);
    c->desired_pos = -length;
}

void
crane_pulley::set_layer(int z)
{
    tms_entity_set_prio_all((struct tms_entity*)this, z);

    if (this->body) {
        b2Filter filter = world::get_filter_for_layer(z, this->layer_mask);

        if (this->body == W->ground) {
            b2Filter curr = this->fx->GetFilterData();
            filter.groupIndex = curr.groupIndex;
            this->fx->SetFilterData(filter);
        } else {
            for (b2Fixture *f = this->body->GetFixtureList(); f; f=f->GetNext()) {
                b2Filter curr = f->GetFilterData();
                filter.groupIndex = curr.groupIndex;
                f->SetFilterData(filter);
            }
        }
    }
}

void
crane_pulley::connection_create_joint(connection *c)
{
    for (int x=0; x<4; ++x) {
        if (c == &this->c_side[x]) {
            b2RevoluteJointDef rjd;
            rjd.maxMotorTorque = 10.f;
            rjd.motorSpeed = 0.f;
            rjd.enableMotor = true;
            rjd.collideConnected = false;
            rjd.bodyA = c->e->get_body(c->f[0]);
            rjd.bodyB = c->o->get_body(c->f[1]);
            rjd.localAnchorA = c->e->local_to_body(c->p, c->f[0]);
            rjd.localAnchorB = c->o->local_to_body(c->p_s, c->f[1]);
            rjd.referenceAngle = rjd.bodyB->GetAngle()-rjd.bodyA->GetAngle();

            c->j = W->b2->CreateJoint(&rjd);
            break;
        }
    }
}
