#include "lmotor.hh"
#include "game.hh"
#include "world.hh"
#include "model.hh"
#include "material.hh"

#define SPEED 2.f
#define FORCE 250.f
#define FRICTION 3.f

static void jent_update_ml(struct tms_entity *e);
static void jent_update_sl(struct tms_entity *e);

bool lmotor::ReportFixture(b2Fixture *f)
{
    if (f->IsSensor()) return true;

    entity *e = static_cast<entity*>(f->GetUserData());

    if (this->flag_active(ENTITY_AXIS_ROT)) {
        if (e && e->get_layer() == this->get_layer()
            /*&& (e->type == ENTITY_PLANK
                || e->type == ENTITY_WHEEL
                )*/
            && f->TestPoint(this->query_point)) {
            this->query_result = e;
            this->query_result_fx = f;
            return false;
        }
    } else {
        if (!f->IsSensor() && e && f->TestPoint(this->query_point)
            /*&& (e->type == ENTITY_PLANK
                || e->type == ENTITY_WHEEL
                )*/
            && abs(e->get_layer() - this->get_layer()) == 1) {
            this->query_result = e;
            this->query_result_fx = f;
            return false;
        }
    }

    return true;
}

void
lmotor::construct()
{
    if (this->properties[1].v.i > 2) {
        this->properties[1].v.i = 2;
    }

    this->recreate_shape();

    if (!this->jent) {
        this->jent = tms_entity_alloc();
        tms_entity_init(this->jent);
        tms_entity_set_mesh(this->jent, const_cast<tms_mesh*>(tms_meshfactory_get_cylinder()));
        tms_entity_set_material(this->jent, (struct tms_material *)&m_colored);
        tms_entity_set_uniform4f(this->jent, "~color", .3f, .3f, .3f, 1.f);
        tms_entity_set_update_fn(this->jent, (void*)jent_update_ml);
        tms_entity_add_child(static_cast<struct tms_entity*>(this), this->jent);
    }
}

bool
lmotor::allow_connection(entity *asker, uint8_t frame, b2Vec2 p)
{
    /* do not allow connections if we are axis toggled and
     * something is attempting to connect along the joint path */

    b2Vec2 l = this->world_to_local(p, 0);

    if (l.y > 0.1f) {
        return false;
    }

    return true;
}

void
lmotor::on_load(bool created, bool has_state)
{
    if (this->properties[1].v.i > 3) {
        this->properties[1].v.i = 3;
    }

    this->recreate_shape();

    if (!this->jent) {
        this->jent = tms_entity_alloc();
        tms_entity_init(this->jent);
        tms_entity_set_mesh(this->jent, const_cast<tms_mesh*>(tms_meshfactory_get_cylinder()));
        tms_entity_set_material(this->jent, (struct tms_material *)&m_colored);
        tms_entity_set_uniform4f(this->jent, "~color", .3f, .3f, .3f, 1.f);
        tms_entity_set_update_fn(this->jent, (void*)jent_update_ml);
        tms_entity_add_child(static_cast<struct tms_entity*>(this), this->jent);
    }

    this->init_socks();

}

float
lmotor::get_slider_snap(int s)
{
    return s==1 ? (1.f/3.f) : .1f;
}

float
lmotor::get_slider_value(int s)
{
    if (s == 0) {
        return this->properties[0].v.f;
    } else
        return ((float)this->properties[1].v.i)/3.f;
}

void
lmotor::on_slider_change(int s, float value)
{
    if (s == 0) {
        this->properties[0].v.f = value;
        G->show_numfeed(value - 0.5f);
    } else {
        this->properties[1].v.i = (uint32_t)roundf(value * 3.f);
        this->disconnect_all();
        this->recreate_shape();
    }
}

static void
jent_update_ml(struct tms_entity *e)
{
    lmotor *lm = (lmotor*)e->parent;

    tmat4_load_identity(e->M);
    b2Vec2 p = lm->get_joint_pos();
    tmat4_translate(e->M, p.x, p.y, lm->get_layer()*LAYER_DEPTH+LAYER_DEPTH*.5f);
    tmat3_copy_mat4_sub3x3(e->N, e->M);
    tmat4_scale(e->M, .10f, .10f, 1.0f);
}

static void
jent_update_sl(struct tms_entity *e)
{
    lmotor *lm = (lmotor*)e->parent;

    tmat4_load_identity(e->M);
    b2Vec2 p = lm->get_joint_pos();
    tmat4_translate(e->M, p.x, p.y, lm->get_layer()*LAYER_DEPTH);
    tmat4_rotate(e->M, lm->get_angle()*(180.f/M_PI) + 90.f, 0.f, 0.f, -1.f);
    tmat4_translate(e->M, .4f, .0f, 0.f);
    tmat4_rotate(e->M, 90.f, 0.f, 1.f, 0.f);
    tmat3_copy_mat4_sub3x3(e->N, e->M);
    tmat4_scale(e->M, .10f, .10f, .5f);
}

lmotor::lmotor(bool is_servo)
{
    this->jent = 0;
    this->is_servo = is_servo;
    this->set_mesh(mesh_factory::get_mesh(MODEL_LMOTOR0));
    this->set_material(&m_edev_dark);
    this->curr_update_method = this->update_method = ENTITY_UPDATE_CUSTOM;

    tmat4_load_identity(this->M);
    tmat3_load_identity(this->N);
    this->set_num_properties(3);
    this->properties[0].v.f = .5f; /* SPEED/FORCE */
    this->properties[0].type = P_FLT;
    this->properties[1].v.i = 1; /* SIZE */
    this->properties[1].type = P_INT;
    this->properties[2].v.i = 0; /* RECOIL */
    this->properties[2].type = P_INT;

    this->num_sliders = 2;

    this->set_flag(ENTITY_ALLOW_AXIS_ROT, true);

    /* XXX */
    //((struct tms_entity*)this)->update = 0;
    //
    this->layer_mask = 15;

    this->num_s_in = 1;
    this->num_s_out = 0;

    this->init_socks();

    this->c.init_owned(0, this);
    this->c_side[0].init_owned(1, this); this->c_side[0].type = CONN_GROUP;
    this->c_side[1].init_owned(2, this); this->c_side[1].type = CONN_GROUP;
    this->c_side[2].init_owned(3, this); this->c_side[2].type = CONN_GROUP;
    this->c_side[3].init_owned(4, this); this->c_side[3].type = CONN_GROUP;

    this->set_as_rect(this->get_size(), .275f);

    //this->set_uniform("~color", .1f, 0.1f, 0.1f, 1.f);
    //
}

void
lmotor::init_socks()
{
    if (this->flag_active(ENTITY_AXIS_ROT)) {
        this->s_in[0].lpos = b2Vec2(0.f,-.085f);
        this->s_in[0].ctype = CABLE_BLUE;
        this->s_in[0].angle = M_PI/2.f;

        /*
        this->s_in[1].lpos = b2Vec2(.0f,.35f);
        this->s_in[1].ctype = CABLE_RED;
        this->s_in[1].angle = M_PI/2.f;

        this->s_in[2].lpos = b2Vec2(.4f,.35f);
        this->s_in[2].ctype = CABLE_RED;
        this->s_in[2].angle = M_PI/2.f;
        */
    } else {
        this->s_in[0].lpos = b2Vec2(0.f,-.185f);
        this->s_in[0].ctype = CABLE_BLUE;
        this->s_in[0].angle = -M_PI/2.f;

        /*
        this->s_in[1].lpos = b2Vec2(.0f,-.35f);
        this->s_in[1].ctype = CABLE_RED;
        this->s_in[1].angle = -M_PI/2.f;

        this->s_in[2].lpos = b2Vec2(.4f,-.35f);
        this->s_in[2].ctype = CABLE_RED;
        this->s_in[2].angle = -M_PI/2.f;
        */
    }

    if (this->jent) {
        if (!this->flag_active(ENTITY_AXIS_ROT)) {
            tms_entity_set_update_fn(this->jent, (void*)jent_update_ml);
        } else {
            tms_entity_set_update_fn(this->jent, (void*)jent_update_sl);
        }
    }

}

b2Vec2
lmotor::get_joint_pos()
{
    if (!this->c.pending && this->c.j) {
        /* this->c.j can be 0 in cases where the linear motor has been
         * captured into a composable group */
        return this->local_to_world(b2Vec2(((b2PrismaticJoint*)this->c.j)->GetJointTranslation(), 0.f), 0);
    } else {
        return this->get_position();
    }
}

void
lmotor::ghost_update()
{
    update();
}

void
lmotor::update()
{
    b2Vec2 p = this->get_position();

    tmat4_load_identity(this->M);
    tmat4_translate(this->M, p.x, p.y, this->prio*LAYER_DEPTH);
    tmat4_rotate(this->M, this->get_angle()*(180.f/M_PI), 0.f, 0.f, -1.f);

    tmat3_copy_mat4_sub3x3(this->N, this->M);

    if (this->jent)
        tms_entity_update(this->jent);
}

void
lmotor::toggle_axis_rot()
{
    this->set_flag(ENTITY_AXIS_ROT, !this->flag_active(ENTITY_AXIS_ROT));
    this->disconnect_all();
    this->init_socks();
    this->recreate_shape();
}

void
lmotor::recreate_shape()
{
    if (this->properties[1].v.i > 3) this->properties[1].v.i = 3;

    uint32_t size = this->properties[1].v.i;

    this->set_mesh(
            mesh_factory::get_mesh(
                (this->flag_active(ENTITY_AXIS_ROT) ? MODEL_LMOTOR0_R : MODEL_LMOTOR0)
                + size
            )
        );

    this->set_as_rect(this->get_size(), .275f);

    composable::recreate_shape();

    float qw = this->width/2.f + 0.15f;
    float qh = this->height/2.f + 0.15f;
    if (this->flag_active(ENTITY_AXIS_ROT))
        this->query_sides[0].Set(0.f,  0.f);
    else
        this->query_sides[0].Set(0.f,  qh); /* up */
    this->query_sides[1].Set(-qw, 0.f); /* left */
    this->query_sides[2].Set(0.f, -qh); /* down */
    this->query_sides[3].Set( qw, 0.f); /* right */
}

/*
void
lmotor::set_position(float x, float y)
{
    entity::set_position(x,y);
    this->update();
}
*/

void
lmotor::find_pairs()
{
    if (this->c.pending) {
        b2Vec2 p;

        if (this->flag_active(ENTITY_AXIS_ROT)) {
            p = this->local_to_world(b2Vec2(0.f, .5f), 0);
        } else
            p = this->local_to_world(b2Vec2(0.f, 0.f), 0);

        b2AABB aabb;

        this->query_result = 0;
        this->query_point = p;

        aabb.lowerBound.Set(p.x - .05f, p.y - .05f);
        aabb.upperBound.Set(p.x + .05f, p.y + .05f);
        W->b2->QueryAABB(this, aabb);

        if (this->query_result != 0) {
            /* We need to make sure we don't try to connect to ourselves,
             * or something that belongs to our group */
            if (this->get_body(0) != this->query_result->get_body(0)) {
                this->c.o = this->query_result;
                this->c.p = p;
                this->c.o_data = this->query_result->get_fixture_connection_data(this->query_result_fx);
                G->add_pair(this, this->query_result, &this->c);
            }
        }
    }

    this->sidecheck4(this->c_side);
}

void
lmotor::connection_create_joint(connection *c)
{
    b2World *w = this->get_body(0)->GetWorld();

    b2PrismaticJointDef rjd;
    if (W->level.version >= 26 && !W->is_paused())
        rjd.tolerance = W->level.prismatic_tolerance;
    rjd.maxMotorForce = 0.f;
    rjd.motorSpeed = 0.f;
    rjd.enableLimit = true;
    rjd.upperTranslation = this->get_size() - .25;
    rjd.lowerTranslation = -this->get_size() + .25;
    rjd.collideConnected = true;

    if (this->flag_active(ENTITY_AXIS_ROT))
        rjd.localAnchorA = this->local_to_body(b2Vec2(0,.5f), 0);
    else
        rjd.localAnchorA = this->local_to_body(b2Vec2(0,0), 0);

    rjd.localAnchorB = c->o->local_to_body(c->p_s, 0);
    rjd.bodyA = this->get_body(0);
    rjd.bodyB = c->o->get_body(c->f[1]);

    float a = this->get_angle();
    b2Vec2 va = this->get_body(0)->GetLocalVector(b2Vec2(cosf(a), sinf(a)));
    rjd.localAxisA = va;

    //rjd.referenceAngle = c->o->get_angle() - c->e->get_angle();
    rjd.referenceAngle = c->o->get_body(c->f[1])->GetAngle() - c->e->get_body(0)->GetAngle();

    //if (!this->w->is_paused())
        //rjd.enableMotor = true;

    c->j = (b2PrismaticJoint*)w->CreateJoint(&rjd);
    ((b2PrismaticJoint*)c->j)->EnableMotor(true);

    if (c->multilayer)
        tms_entity_set_update_fn(this->jent, (void*)jent_update_ml);
    else
        tms_entity_set_update_fn(this->jent, (void*)jent_update_sl);
}

connection *
lmotor::load_connection(connection &conn)
{
    if (conn.o_index == 0) {
        this->c = conn;
        return &this->c;
    } else {
        this->c_side[conn.o_index-1] = conn;
        return &this->c_side[conn.o_index-1];
    }
}

void
lmotor::ifstep(float v, float ctl_speed, float ctl_angle,float ctl_tradeoff, bool enable_angle, bool enable_tradeoff)
{
    b2PrismaticJoint *j = (b2PrismaticJoint*)this->c.j;

    if (j) {
        float tr = this->properties[0].v.f;
        float dd = 1.f;
        float speed, force;

        if (enable_tradeoff)
            tr = ctl_tradeoff;

        if (enable_angle) {
            float target = ctl_angle * (this->get_size()-.25f) * 2.f - (this->get_size()-.25f);
            dd = target - j->GetJointTranslation();
        } else {
            bool recoil = (bool)this->properties[2].v.i;

            if (recoil && ctl_speed <= 0.f) {
                dd = -1.f;
                ctl_speed = 1.f;
            }
        }

        float max_speed = (1.f - tr) * ctl_speed * v * SPEED;
        speed = max_speed * dd;

        if (this->is_servo)  {
            force = tr * v * FORCE;
            force *= 1.5f;
            speed *= 1.f/1.5f;
        } else {
            force = tr * v * FORCE;

            if (W->level.version >= LEVEL_VERSION_1_3_0_3) {
                if (std::abs(speed) <= 0.f) {
                    force = 0.f;
                }
            } else {
                if (speed <= 0.f) {
                    force = 0.f;
                }
            }
        }

        if (force <= 0.f) force = FRICTION;

        j->SetMotorSpeed(speed);
        j->SetMaxMotorForce(force);
        this->_speed = speed;
        this->_force = force;
    }
}

void
lmotor::ifget(iffeed *feed)
{
    b2PrismaticJoint *j = (b2PrismaticJoint*)this->c.j;

    if (j) {
        float js = j->GetJointSpeed(), ms = j->GetMotorSpeed();
        float s;
        if (fabsf(ms) < 0.00001f) s = 1.f;
        else s = js/ms;
        feed->speed = tclampf(s, 0.f, 1.f);
        if (W->level.version >= LEVEL_VERSION_1_3_0_3) {
            float inv_dt = 1. / ((double)((WORLD_STEP+WORLD_STEP_SPEEDUP)/1000000.) * G->get_time_mul());
            float abs_motor_force = std::abs(j->GetMotorForce(inv_dt));
            float max_motor_force = j->GetMaxMotorForce();

            if (max_motor_force < 0.0000001f) {
                feed->torque = 0.f;
            } else {
                feed->torque = tclampf(abs_motor_force / max_motor_force, 0.f, 1.f);
            }
        } else {
            float mt = j->GetMaxMotorForce();
            if (mt < .0000001f)
                feed->torque = 0.f;
            else
                feed->torque = tclampf(j->GetMotorForce(1. / .012) / mt, 0.f, 1.f);
        }
        feed->error = ((js >= 0.f) == (ms >= 0.f));
        feed->angle = j->GetJointTranslation() + (this->get_size()-.25f);
        feed->angle /= (this->get_size()-.25f) * 2.f;
        feed->angle = tclampf(feed->angle, 0.f, 1.f);
    }
}


void
lmotor::read_state(lvlinfo *lvl, lvlbuf *lb)
{
    entity::read_state(lvl, lb);

    this->_speed = lb->r_float();
    this->_force = lb->r_float();

}

void
lmotor::restore()
{
    entity::restore();

    b2PrismaticJoint *j = (b2PrismaticJoint*)this->c.j;

    if (j) {
        j->SetMotorSpeed(this->_speed);
        j->SetMaxMotorForce(this->_force);
    }
}
