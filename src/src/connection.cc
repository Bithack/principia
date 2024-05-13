#include "connection.hh"
#include "model.hh"
#include "material.hh"
#include "world.hh"
#include "object_factory.hh"
#include "gear.hh"
#include "rack.hh"
#include "game.hh"

void
connection::update(void)
{
    this->layer = this->e->get_layer();
    this->layer_mask = this->e->layer_mask;

    if (this->layer > this->o->get_layer()) {
        this->layer = this->o->get_layer();
        this->layer_mask = this->o->layer_mask;
    }

    this->multilayer = (this->e->get_layer() != this->o->get_layer());
    this->sublayer_dist = this->e->sublayer_dist(this->o);
}

/**
 * Updates the relative angle between the bodies
 **/
void
connection::update_relative_angle(bool force)
{
    if (!force) {
        b2Joint *j = this->j;
        if (j) {
            b2JointType t = j->GetType();

            if (t != e_revoluteJoint && t != e_pivotJoint) {
                return;
            }
        }
    }

    this->relative_angle = -(this->o->get_angle(this->f[1]) - this->e->get_angle(this->f[0]));
}

/**
 * Apply is called the first time the connection is created
 **/
void
connection::apply(void)
{
    //tms_infof("apply joint : %f %f, %p", this->p.x, this->p.y, this);
    this->update_relative_angle(true);

    if (this->type != CONN_GEAR) {
        this->p_s = this->o->world_to_local(this->p, this->f[1]);
        this->p = this->e->world_to_local(this->p, this->f[0]);
    }

    this->layer = this->e->get_layer();
    this->layer_mask = this->e->layer_mask;

    if (this->layer > this->o->get_layer()) {
        this->layer = this->o->get_layer();
        this->layer_mask = this->o->layer_mask;
    }

    this->multilayer = (this->e->get_layer() != this->o->get_layer());
    this->sublayer_dist = this->e->sublayer_dist(this->o);

    if (!this->fixed) {
        //tms_infof("adding to bodies");
        this->e->add_connection(this);
        this->o->add_connection(this);
    }

    this->pending = false;
}

void
connection::destroy_joint()
{
    if (this->j) {
        if (!this->owned || this->e->connection_destroy_joint(this) == false)
            this->e->get_body(this->f[0])->GetWorld()->DestroyJoint(this->j);

        this->j = 0;

    }

    this->remove_self_ent();
}

void
connection::remove_self_ent()
{
    if (this->self_ent) {
        if (this->self_ent->scene) {
            G->remove_entity(this->self_ent);
        }

        delete this->self_ent;
        this->self_ent = 0;
    }
}

void
connection::create_self_ent(bool add)
{
    if (this->render_type == CONN_RENDER_HIDE) {
        return;
    }

    switch (this->type) {
        case CONN_PIVOT:
        case CONN_WELD:
        case CONN_PLATE:
            this->self_ent = new connection_entity(this, this->type);
            break;
    }

    if (this->self_ent && add) {
        if (this->self_ent->scene == 0) {
            G->add_entity(this->self_ent);
        }
    }
}

void
connection::update_render_type()
{
    tms_debugf("Update render type. Self ent: %p", this->self_ent);
    bool add = (this->self_ent && this->self_ent->scene);

    this->remove_self_ent();

    switch (this->render_type) {
        case CONN_RENDER_HIDE:
            break;

        default:
            if (!this->self_ent) {
                this->create_self_ent(add);
            }
            break;
    }
}

void
connection::create_joint(bool add)
{
    if (this->type == CONN_GROUP) {
        return;
    }

    if (!this->self_ent) {
        this->create_self_ent(add);
    } else {
        if (this->self_ent->mesh == mesh_factory::get_mesh(MODEL_PLATEJOINT_DAMAGED)) {
            this->self_ent->set_mesh(mesh_factory::get_mesh(MODEL_PLATEJOINT));
        }
    }

    //tms_infof("group: %p, body: %p, %d", this->e->group, this->e->body, this->f[0]);
    //b2World *b2 = this->e->get_body(this->f[0])->GetWorld();
    b2World *b2 = W->b2;
    //b2RevoluteJointDef rjd;
    b2PivotJointDef rjd;
    b2WeldJointDef wjd;
    b2GearJointDef gjd;
    b2PrismaticJointDef pjd;

    bool recreate = false;

    if (this->j) {
        recreate = true;
        b2->DestroyJoint(j);
        j = 0;
    }

    if (this->e->get_body(this->f[0]) != this->o->get_body(this->f[1])) {
        switch (this->type) {
            case CONN_CUSTOM: this->e->connection_create_joint(this); break;

            case CONN_RACK:{
                gear *g = static_cast<gear*>(this->e); /* always this order */
                rack *r = static_cast<rack*>(this->o);

                gjd.bodyA = this->e->get_body(this->f[0]);
                gjd.bodyB = this->o->get_body(this->f[1]);
                gjd.joint1 = g->joint;
                gjd.joint2 = r->joint;
                gjd.ratio = 1.f/g->get_ratio();
                this->j = b2->CreateJoint(&gjd);

                r->update_limits();
                }
                break;

            case CONN_GEAR:{
                gear *g0 = static_cast<gear*>(this->e);
                gear *g1 = static_cast<gear*>(this->o);
                float ratio = 1.f;
                if (this->e->get_layer() == this->o->get_layer()) {
                    if (g0->properties[0].v.i != g1->properties[0].v.i) {
                        //ratio = g1->get_ratio()/g0->get_ratio();
                        ratio = g1->get_ratio()/g0->get_ratio();
                    }
                }

                /*
                if (((gear*)this->e)->get_num_gear_conns() < ((gear*)this->o)->get_num_gear_conns()) {
                    ((gear*)this->e)->fix_position(this->o);
                } else
                    ((gear*)this->o)->fix_position(this->e);
                    */

                gjd.bodyA = this->e->get_body(this->f[0]);
                gjd.bodyB = this->o->get_body(this->f[1]);
                gjd.joint1 = ((gear*)this->e)->joint;
                gjd.joint2 = ((gear*)this->o)->joint;

                tms_infof("types %p %p: %p %p %d %d", this->e, this->o, gjd.joint1, gjd.joint2, gjd.joint1->GetType(), gjd.joint2->GetType());
                gjd.ratio = ratio;
                gjd.collideConnected = true;
                this->j = b2->CreateJoint(&gjd);
                }
                break;

            case CONN_PIVOT:
                if (this->e->is_wheel() && this->o->is_wheel() && this->e->get_layer() != this->o->get_layer()) {
                    rjd.localAnchorA = this->e->local_to_body(b2Vec2(0.f, 0.f), this->f[0]);
                    rjd.localAnchorB = this->o->local_to_body(b2Vec2(0.f, 0.f), this->f[1]);
                } else {
                    rjd.localAnchorA = this->e->local_to_body(this->p, this->f[0]);
                    rjd.localAnchorB = this->o->local_to_body(this->p_s, this->f[1]);
                }
                rjd.bodyA = this->e->get_body(this->f[0]);
                rjd.bodyB = this->o->get_body(this->f[1]);

                if (this->damping > 0.f) {
                    rjd.enableMotor = true;
                    rjd.maxMotorTorque = this->damping;
                    rjd.motorSpeed = 0.f;
                } else if (W->level.version >= LEVEL_VERSION_1_5 && W->level.joint_friction > 0.f) {
                    rjd.enableMotor = true;
                    rjd.maxMotorTorque = W->level.joint_friction;
                    rjd.motorSpeed = 0.f;
                }

                rjd.collideConnected = true;

                if (W->level.version >= 26 && !W->is_paused())
                    rjd.tolerance = W->level.pivot_tolerance;

                this->j = b2->CreateJoint(&rjd);
                break;

            case CONN_PLATE:
            case CONN_WELD:
                {
                    if (this->e->is_wheel() && this->o->is_wheel() && this->e->get_layer() != this->o->get_layer()) {
                        wjd.localAnchorA = this->e->local_to_body(b2Vec2(0.f, 0.f), this->f[0]);
                        wjd.localAnchorB = this->o->local_to_body(b2Vec2(0.f, 0.f), this->f[1]);
                    } else {
                        wjd.localAnchorA = this->e->local_to_body(this->p, this->f[0]);
                        wjd.localAnchorB = this->o->local_to_body(this->p_s, this->f[1]);
                    }

                    wjd.bodyA = this->e->get_body(this->f[0]);
                    wjd.bodyB = this->o->get_body(this->f[1]);

                    if (wjd.bodyA->GetType() == b2_staticBody && wjd.bodyB->GetType() == b2_staticBody) {
                        this->j = 0;
                        tms_debugf("skipping joint between static bodies");
                        break;
                    }

                    wjd.referenceAngle = this->o->get_body(this->f[1])->GetAngle() - this->e->get_body(this->f[0])->GetAngle()
                        - (this->o->get_angle(this->f[1]) - this->e->get_angle(this->f[0])) - this->relative_angle;

                    //wjd.referenceAngle = this->o->get_angle(this->f[1]) - this->e->get_angle(this->f[0]);
                    wjd.collideConnected = false;
                    wjd.frequencyHz = 0.f;

                    if (false && this->tolerant && !W->is_paused())
                        wjd.frequencyHz = 40.f;

                    this->j = b2->CreateJoint(&wjd);

                    b2Vec2 pp = this->e->local_to_world(wjd.localAnchorA, this->f[0]);
                    tms_debugf("applied plate, to world = %f %f, f = %u, id <%u,%u>", pp.x, pp.y, this->f[0], this->e->id, this->o->id);
                }
                break;

            default:
                {
                    tms_errorf("invalid joint type %d", this->type);
                }
                break;
        }

        if (this->max_force < INFINITY) {
            //tms_infof("joint is destructable");
            G->add_destructable_joint(this->j, this->max_force);
        }
    } else
        this->j = 0;

    if (this->j && this->j != (void*)1) {
        // small memory leak =(
        this->ji = new joint_info(JOINT_TYPE_CONN, this);

        this->j->SetUserData(this->ji);
    }
}

b2Vec2 connection::get_position()
{
    return this->e->local_to_world(this->p, this->f[0]);
}

connection_entity::connection_entity(connection *c, int type)
{
    this->conn = c;
    this->curr_update_method = this->update_method = ENTITY_UPDATE_JOINT;

    //((struct tms_entity*)this)->update = 0;
    //
    this->conn->type = type;

    if ((c->e->layer_mask & c->o->layer_mask) == 0 && c->o->get_layer() == c->e->get_layer()) {
        /* breadboard, sublayer plank */
        type = CONN_PIVOT;
    }

    this->prio = c->layer+(int)c->multilayer;
    this->prio_bias = 1;

    //tms_infof("layer of joint: %d", this->get_layer());

    tmat4_load_identity(this->M);
    tmat3_load_identity(this->N);


    switch (type) {
        case CONN_WELD: case CONN_PLATE:
            if (c->render_type == CONN_RENDER_NAIL || (c->multilayer)) {
                this->set_mesh(mesh_factory::get_mesh(MODEL_PIVOTJOINT));
            } else {
                this->set_mesh(mesh_factory::get_mesh(MODEL_PLATEJOINT));
            }
            break;
   /*     case CONN_WELD:
             if (c->multilayer)
                 this->set_mesh(tms_meshfactory_get_cylinder());
             else
                 this->set_mesh(mesh_factory::weldjoint);
             break;*/
        case CONN_PIVOT:
            this->set_mesh(mesh_factory::get_mesh(MODEL_PIVOTJOINT));
             this->curr_update_method = this->update_method = ENTITY_UPDATE_JOINT_PIVOT;
             break;
    }

    /*
    if (c->render_type == CONN_RENDER_NAIL)
        this->set_material(&m_conn);
    else
    */
        this->set_material(&m_conn_no_ao);
}
