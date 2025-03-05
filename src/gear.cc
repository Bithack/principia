#include "gear.hh"
#include "model.hh"
#include "material.hh"
#include "world.hh"
#include "game.hh"
#include "rack.hh"

gear::gear()
{
    this->width = 1.f;
    this->type = ENTITY_GEAR;
    this->set_flag(ENTITY_IS_DEV, true);
    this->set_flag(ENTITY_DO_STEP, true);
    this->set_flag(ENTITY_DO_TICK, true);
    this->pending = 0;
    this->num_sliders = 1;
    this->menu_scale = .5f;

    this->joint = 0;

    this->c_back.init_owned(0, this);
    this->c_back.type = CONN_CUSTOM;
    this->c_back.typeselect = 1; /* allow the player to select between weld and pivot joints */

    this->c_front.init_owned(1, this);
    this->c_front.type = CONN_CUSTOM;
    this->c_front.typeselect = 1;

    this->set_mesh(mesh_factory::get_mesh(MODEL_GEAR2));
    this->set_material(&m_gear);

    tmat4_load_identity(this->M);
    tmat3_load_identity(this->N);

    this->set_num_properties(1);
    this->properties[0].v.i = 2;
}

void
gear::tick()
{
    if (!(this->joint = (b2RevoluteJoint*)this->find_pivot(0,true)))
        this->joint = (b2RevoluteJoint*)this->find_pivot(1,true);

    step();
}

void
gear::step()
{
    connection *c = this->conn_ll;
    if (c) {
        do {
            if (c->type == CONN_GEAR) {
                gear *g1 = (gear*)c->e;
                gear *g2 = (gear*)c->o;
                if ((c->e->get_position() - c->o->get_position()).Length()
                        > g1->get_ratio()*.8f+g2->get_ratio()*.8f+.2f) {
                    this->destroy_connection(c);
                    break;
                }
            }
        } while ((c = c->get_next(this)));
    }

    if (this->pending) {
        game *g = G;
        if (this->pending->type == ENTITY_GEAR &&
                ((gear*)this->pending)->joint && this->joint
                && !this->connected_to(this->pending)) {
            connection *c = G->get_tmp_conn();

            c->type = CONN_GEAR;
            c->e = this;
            c->o = this->pending;
            c->pending = true;

            if (!G->add_pair(this, this->pending, c))
                G->return_tmp_conn(c);
        } else if (this->pending->type == ENTITY_RACK) {
            rack *r = (rack*)this->pending;

            if (r->point_in_range(this->get_position())) {
                connection *c = G->get_tmp_conn();
                c->type = CONN_RACK;
                c->e = this;
                c->o = this->pending;
                c->pending = true;

                if (!G->add_pair(this, this->pending, c))
                    G->return_tmp_conn(c);
            }

        }

        this->pending = 0;
    }
}

float
gear::get_slider_value(int s)
{
    return this->properties[0].v.i / 3.f;
}

float
gear::get_slider_snap(int s)
{
    return 1.f/3.f;
}

int
gear::get_num_gear_conns()
{
    int num_gear_conns = 0;
    connection *c = this->conn_ll;
    if (c) {
        do {
            if (c->type == CONN_GEAR || c->type == CONN_RACK) {
                num_gear_conns ++;
                if (c->type == CONN_GEAR) {
                }
            }
        } while ((c = c->get_next(this)));
    }

    return num_gear_conns;
}

void
gear::on_load(bool created, bool has_state)
{
    uint32_t size = this->properties[0].v.i;
    if (size > 3) size = 3;

    this->set_mesh(mesh_factory::get_mesh(MODEL_GEAR0+size));
    this->set_material(&m_gear);

    this->width = this->get_ratio();
}

void
gear::connection_create_joint(connection *c)
{
    if (c->option == 1) {
        b2RevoluteJointDef rjd;
        rjd.bodyA = c->o->get_body(c->f[1]);
        rjd.bodyB = this->body;

        if (c->o->is_wheel()) {
            rjd.localAnchorA = c->o->local_to_body(b2Vec2(0.f, 0.f), c->f[1]);
        } else 
            rjd.localAnchorA = c->o->get_body(c->f[1])->GetLocalPoint(this->get_position());

        rjd.localAnchorB = b2Vec2(0,0);
        c->j = W->b2->CreateJoint(&rjd);
    } else {
        b2WeldJointDef wjd;
        wjd.localAnchorA = this->local_to_body(c->p, c->f[0]);
        wjd.bodyA = this->get_body(c->f[0]);
        if (c->o->is_wheel()) {
            wjd.localAnchorB = c->o->local_to_body(b2Vec2(0.f, 0.f), c->f[1]);
        } else 
            wjd.localAnchorB = c->o->get_body(c->f[1])->GetLocalPoint(this->get_position());
        wjd.bodyB = c->o->get_body(c->f[1]);
        wjd.referenceAngle = c->o->get_body(c->f[1])->GetAngle() - this->get_body(c->f[0])->GetAngle();
        wjd.collideConnected = false;
        wjd.frequencyHz = 0.f;
        c->j = W->b2->CreateJoint(&wjd);
    }
}

bool
gear::connection_destroy_joint(connection *c)
{
    if (c->j == this->joint) this->joint = 0;
    W->b2->DestroyJoint(c->j);
    return true;
}

void
gear::on_slider_change(int s, float value)
{
    uint32_t size = (uint32_t)roundf(value*3.f);
    this->set_property(0, size);
    this->disconnect_all();

    this->on_load(false, false);

    this->create_shape();

    //this->width = ((float)size+1.f)/2.f;
    //this->set_as_rect(((float)size+1.f)/2.f, .15f);
    //this->recreate_shape();

    //tms_infof("resize to value %f, size %u", value, size);
}

void
gear::set_angle(float a)
{
    entity::set_angle(a);
    connection *c = this->conn_ll;
    if (c) {
        do {
            if (c->type == CONN_GEAR || c->type == CONN_RACK)
                c->get_other(this)->get_body(c->f[1])->SetAwake(true);
        } while ((c = c->get_next(this)));
    }
}

void
gear::disconnect_gears()
{
    connection *c = this->conn_ll;
    connection **cc = &this->conn_ll;

    if (c) {
        do {
            connection **ccn = &c->next[(c->e == this) ? 0 : 1];
            if (c->type == CONN_GEAR) {
                *cc = *ccn;

                c->destroy_joint();
                W->erase_connection(c);
                entity *other = ((this == c->e) ? c->o : c->e);
                other->remove_connection(c);

                delete c;
                //break;
            }
            cc = ccn;
            c = *ccn;
        } while (c);
    }
}

void
gear::remove_connection(connection *c)
{
    if (c->type != CONN_GEAR)
        this->disconnect_gears();

    entity::remove_connection(c);
}

void
gear::destroy_connection(connection *c)
{
    if (c->type != CONN_GEAR)
        this->disconnect_gears();

    entity::destroy_connection(c);
}

void
gear::set_position(float x, float y, uint8_t frame/*=0*/)
{
    if (this->conn_ll) {

        int num_gear_conns = 0;
        connection *c = this->conn_ll;
        connection *first = 0;
        if (c) {
            do {
                if (c->type == CONN_GEAR || c->type == CONN_RACK) {
                    num_gear_conns ++;
                    if (c->type == CONN_GEAR) {
                        first = c;
                    }
                }
            } while ((c = c->get_next(this)));
        }

        if (num_gear_conns == 1) {
            /* we only have one gear connection */
                this->set_anchor_pos(x,y);
            /*
            if (first) {
                this->set_anchor_pos(x,y);
                this->fix_position(first->get_other(this));
                first->create_joint(false);
            }
            */
        } else if (num_gear_conns == 0) {
            this->set_anchor_pos(x,y);

        }

//        this->fix_position(this->conn_ll->e == this ? this->conn_ll->o : this->conn_ll->e);
        //this->fix_position(this->conn_ll->o == this ? this->conn_ll->e : this->conn_ll->o);
    } else

    /*if (this->connections.size() > 0) {
        tms_infof("moving and we are already stuck to shit ");

        for (std::set<gear *>::iterator i = this->connections.begin();
                i != this->connections.end();
                ) {
            gear *g = *i;

            if ((this->get_position() - g->get_position()).Length() > 2.25f) {
                //this->connections.erase(i++);
                //tms_infof("disconnect that shit ");
                i++;
            } else {
                this->set_anchor_pos(x,y);
                this->fix_position(g);
                i++;
            }
        }
    } else {*/
        this->set_anchor_pos(x,y);
    //}
}

void
gear::set_anchor_pos(float x, float y)
{
    entity::set_position(x,y);
    //if (this->joint)
        //this->joint->m_localAnchorA = b2Vec2(x,y);
}

float
gear::get_ratio()
{
    switch (this->properties[0].v.i) {
        case 0: return .25f;
        case 1: return .5f;
        case 2: return .75f;
        default: case 3: return 1.f;
    }
}

void
gear::fix_position(gear *other)
{
    b2Vec2 d = this->body->GetPosition() - other->body->GetPosition();
    float dist = d.Length();

    float rr = this->get_ratio()+other->get_ratio();
    float rr1 = rr-.0f;
    float rr2 = rr+.05f;

    d *= 1.f/(dist);

    tms_infof("dist %f", dist);
    tms_infof("rr1 %f", rr1);
    tms_infof("rr2 %f", rr2);

    dist = fmaxf(dist, rr1);
    dist = fminf(dist, rr2);

    tms_infof("final dist %f", dist);

    b2Vec2 dd = d;
    dd *= dist;
    dd += other->body->GetPosition();

    double diff = atan2(d.y, d.x);

    float ratio = this->get_ratio()/other->get_ratio();

    //ratio = ratio*2*M_PI;

    tms_infof("ratio %f", ratio);

    double na =
        diff * (1. + 1.f/ratio)
        - other->body->GetAngle() * 1.f/ratio
        + (M_PI/12.) / this->get_ratio();

    dist = tmath_adist(this->body->GetAngle(), na);

    //this->body->SetTransform(dd, this->body->GetAngle());
    this->body->SetTransform(this->body->GetPosition(), this->body->GetAngle()+dist);
    this->set_anchor_pos(dd.x, dd.y);
}

/*
void
gear::find_pairs()
{
    if (this->pending) {
        if (this->pending->type == ENTITY_GEAR) {
            connection *c = g->get_tmp_conn();
            c->type = CONN_GEAR;
            c->e = this;
            c->o = this->pending;
            c->pending = true;

            if (!g->add_pair(this, this->pending, c))
                g->return_tmp_conn(c);
        } else if (this->pending->type == ENTITY_RACK) {
            rack *r = this->pending;

            if (r->point_in_range(this->get_position())) {
                connection *c = g->get_tmp_conn();
                c->type = CONN_RACK;
                c->e = this;
                c->o = this->pending;
                c->pending = true;

                if (!g->add_pair(this, this->pending, c))
                    g->return_tmp_conn(c);
            }

        }

        this->pending = 0;
    }
}
*/

void
gear::on_touch(b2Fixture *a, b2Fixture *b)
{
    entity *e = (entity*)b->GetUserData();
    if (e) {
        if (e->type == ENTITY_GEAR) {
            gear *g = (gear*)e;

            if (g->joint) {
                if (!this->connected_to(g)) {
                    this->pending = g;
                }
            }
        } else if (e->type == ENTITY_RACK) {
            if (!this->connected_to(e))
                this->pending = e;
        }
    }
}

void
gear::create_shape()
{
    if (this->body) {
        for (b2Fixture *f = this->body->GetFixtureList(), *next = 0;
                f; f=next) {
            next = f->GetNext();
            this->body->DestroyFixture(f);
        }

        float r = this->get_ratio();
        r*=.8f;

        b2CircleShape circle;
        circle.m_radius = r+.1f;

        b2CircleShape sensor;
        sensor.m_radius = r;

        b2CircleShape inner;
        inner.m_radius = r-.1f;

        b2FixtureDef fd;
        fd.shape = &circle;
        fd.density = this->get_material()->density; /* XXX */
        fd.friction = this->get_material()->friction;
        fd.restitution = this->get_material()->restitution;
        //fd.filter.groupIndex = -10;

        b2FixtureDef fd_inner;
        fd_inner.shape = &inner;
        fd_inner.density = .1f; /* XXX */
        fd_inner.friction = FLT_EPSILON;
        fd_inner.restitution = .0f;
        fd_inner.filter.groupIndex = 0;
        
        b2FixtureDef fd_sensor;
        fd_sensor.shape = &sensor;
        fd_sensor.density = .1f; /* XXX */
        fd_sensor.friction = FLT_EPSILON;
        fd_sensor.restitution = .0f;
        fd_sensor.filter.groupIndex = 0;
        fd_sensor.isSensor = true;

        if (W->is_paused()) {
            (this->body->CreateFixture(&fd_sensor))->SetUserData(this);
        }
        (this->body->CreateFixture(&fd_inner))->SetUserData(this);
        (this->outer_fixture = this->body->CreateFixture(&fd))->SetUserData(this);

        this->set_layer(this->prio);
    }
}

connection *
gear::load_connection(connection &conn)
{
    if (conn.o_index == 0) {
        this->c_back = conn;
        return &this->c_back;
    } else {
        this->c_front = conn;
        return &this->c_front;
    }
}

bool
gear::ReportFixture(b2Fixture *f)
{
    entity *e = (entity*)f->GetUserData();
    uint8_t fr = (uint8_t)(uintptr_t)f->GetBody()->GetUserData();

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

    return true;
}

void
gear::find_pairs()
{
    if (this->c_back.pending|| this->c_front.pending) {
        b2Vec2 p = this->get_position();
        b2AABB aabb;
        aabb.lowerBound.Set(p.x - .05f, p.y - .05f);
        aabb.upperBound.Set(p.x + .05f, p.y + .05f);

        this->q_result = 0;
        this->q_point = p;

        W->b2->QueryAABB(this, aabb);

        if (q_result != 0) {
            connection *c;
            if (this->q_dir == 1)
                c = &this->c_back;
            else
                c = &this->c_front;

            if (c->pending) {
                c->type = CONN_CUSTOM;
                c->typeselect = this->find_pivot(0,false) == 0 && this->find_pivot(1, false) == 0;
                c->o = q_result;
                c->p = p;
                c->f[1] = q_frame;
                G->add_pair(this, q_result, c);
            }
        }
    }
}

void
gear::setup()
{
    /* find a revolute joint for this->joint */
    if (!(this->joint = (b2RevoluteJoint*)this->find_pivot(0,true)))
        this->joint = (b2RevoluteJoint*)this->find_pivot(1,true);

    this->pending = 0;
}

void
gear::on_pause()
{
    this->joint = 0;
    this->pending = 0;
}

void
gear::add_to_world()
{
    b2BodyDef bd;
    bd.type = this->get_dynamic_type();
    bd.position = this->_pos;
    bd.angle = this->_angle;

    this->body = W->b2->CreateBody(&bd);
    //this->body->SetAngularDamping(1.f);
    this->create_shape();
    this->pending = 0;
}

