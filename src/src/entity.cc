#include "entity.hh"
#include "world.hh"
#include "group.hh"
#include "gear.hh"
#include "game.hh"
#include "rubberband.hh"
#include "pivot.hh"
#include "damper.hh"
#include "model.hh"
#include "world.hh"
#include "ui.hh"

entity::entity()
    : m_scale(1.0f)
    , properties(0)
    , entity_health(ENTITY_MAX_HEALTH)
{
    this->protection_status = ENTITY_NOT_SEARCHED;
    this->flags = 0;
    this->state[0] = 0.f;
    this->state[1] = 0.f;
    this->state[2] = 0.f;
    this->state_size = 0;
    this->state_ptr = 0;
    this->write_ptr = 0;
    this->write_size = 0;
    this->num_chunk_intersections = 0;

    // Default flag states
    this->set_flag(ENTITY_ALLOW_CONNECTIONS, true);
    this->set_flag(ENTITY_ALLOW_ROTATION, true);
    this->set_flag(ENTITY_DO_UPDATE, true);
    this->set_flag(ENTITY_FADE_ON_ABSORB, true);
    this->set_flag(ENTITY_CAN_MOVE, true);
    this->set_flag(ENTITY_CAN_BE_GRABBED, true);

    this->dialog_id = -1;

    this->in_dragfield = false;
    this->interacted_with = false;
    this->emit_step = 0;
    this->emitted_by = 0;
    this->update_method = ENTITY_UPDATE_FASTBODY;
    this->curr_update_method = ENTITY_UPDATE_GHOST;
    //this->owner = 0;
    this->g_id = 0;
    this->id = 0;
    this->menu_pos.Set(0,0);
    this->old_pos.Set(0,0);
    this->layer_mask = 15;
    this->body = 0;
    this->conn_ll = 0;
    this->type = ENTITY_DEFAULT;
    this->_pos = b2Vec2(0,0);
    this->_angle = 0.f;
    this->num_properties = 0;
    this->width = 1.f;
    this->height = 1.f;
    this->menu_scale = 1.f;

    this->gr = 0;
    this->fx = 0;

    this->query_sides[0].SetZero();
    this->query_sides[1].SetZero();
    this->query_sides[2].SetZero();
    this->query_sides[3].SetZero();

    this->cull_effects_method = CULL_EFFECTS_DEFAULT;
    this->num_sliders = 0;

    this->query_len = .2f;
}

entity::~entity()
{
    if (this->properties) {
        delete []this->properties;
    }

    /*
    if (!W->is_paused()) {
        for (std::set<entity*>::iterator it = this->subscriptions.begin();
                it != this->subscriptions.end(); ++it) {
            entity *e = *it;
            std::multimap<int, entity_listener>::iterator jt = e->listeners.begin();
            while (jt != e->listeners.end()) {
                if (jt->second.self == this) {
                    tms_debugf("remove this from that %p - %p", this, e);
                    e->listeners.erase(jt++);
                } else {
                    ++jt;
                }
            }
        }
    }
    */
}

b2BodyType
entity::get_dynamic_type()
{
    if (G->state.sandbox && W->is_paused() && this->is_locked()) {
        tms_debugf("entity locked, forcing to static");
        return b2_staticBody;
    } else if (!this->flag_active(ENTITY_MUST_BE_DYNAMIC)
            && !this->is_moveable()
            && !G->state.sandbox && W->is_paused()) {
        tms_debugf("other thing, forcing to static %u", this->is_moveable());
        return b2_staticBody;
    } else {
        return b2_dynamicBody;
    }
}

class sidecheck_cb : public b2RayCastCallback
{
  public:
    entity *q_result;
    b2Fixture *q_result_fx;
    entity *ignore;
    uint8_t q_frame;
    float q_fraction;

    sidecheck_cb(entity *ignore)
    {
        this->ignore = ignore;
        this->q_result = 0;
    }

    float32 ReportFixture(b2Fixture *f, const b2Vec2 &pt, const b2Vec2 &nor, float32 fraction)
    {
        if (f->IsSensor()) {
            return -1.f;
        }

        b2Body *b = f->GetBody();
        entity *e = static_cast<entity*>(f->GetUserData());

        if (e && e != this->ignore && e->get_layer() == ignore->get_layer() && e->allow_connections()) {
            if ((e->g_id == O_RUBBERBAND || e->g_id == O_RUBBERBAND_2) &&
                (this->ignore->g_id == O_RUBBERBAND || this->ignore->g_id == O_RUBBERBAND_2)) {
                entity *other = NULL;

                if (e->g_id == O_RUBBERBAND) {
                    rubberband_1 *r1 = static_cast<rubberband_1*>(e);
                    other = r1->dconn.get_other(r1);
                } else {
                    rubberband_2 *r2 = static_cast<rubberband_2*>(e);
                    other = r2->get_property_entity();
                }

                /* Ignore this result if we're attempting to connect to our counterpart */
                if (other == this->ignore) {
                    return -1;
                }
            }

            this->q_result = e;
            this->q_result_fx = f;
            this->q_frame = VOID_TO_UINT8(b->GetUserData());
            this->q_fraction = fraction;
            /* TODO: only return nearest */
        }

        return -1;
    }
};

void
entity::sidecheck(connection *c)
{
    if (c->pending) {
        b2Vec2 ray_p[4] = {
            b2Vec2(0.f, this->height/2.f),
            b2Vec2(-this->width/2.f, 0.f),
            b2Vec2(0.f, -this->height/2.f),
            b2Vec2(this->width/2.f, 0.f)
        };
        b2Vec2 ray_v[4] = {
            b2Vec2(0.f, .5f),
            b2Vec2(-.5f, 0.f),
            b2Vec2(0.f, -.5f),
            b2Vec2(.5f, 0.f),
        };

        sidecheck_cb sc(this);

        for (int x=0; x<4; x++) {
            sc.q_result = 0;

            W->raycast(&sc,
                    this->local_to_world(ray_p[x], 0),
                    this->local_to_world(ray_p[x]+ray_v[x], 0));

            if (sc.q_result) {
                c->o = sc.q_result;
                c->f[0] = 0;
                c->f[1] = sc.q_frame;
                c->p = ray_v[x];
                c->p *= sc.q_fraction;
                c->p += ray_p[x];
                c->p = this->local_to_world(c->p, 0);
                c->o_data = sc.q_result->get_fixture_connection_data(sc.q_result_fx);
                G->add_pair(this, sc.q_result, c);
                break;
            }
        }
    }
}

void
entity::sidecheck4(connection *cc)
{
    connection *c;
    b2Vec2 ray_p[4] = {
        b2Vec2(0.f, this->height),
        b2Vec2(-this->width, 0.f),
        b2Vec2(0.f, -this->height),
        b2Vec2(this->width, 0.f)
    };

    b2Vec2 ray_v[4] = {
        this->query_sides[0],
        this->query_sides[1],
        this->query_sides[2],
        this->query_sides[3],
    };

    sidecheck_cb sc(this);

    for (int x=0; x<4; ++x) {
        c = &cc[x];

        if (ray_v[x].Length() > 0.01f && c->pending) {
            ray_v[x] *= 1.f/ray_v[x].Length() * this->query_len;
            sc.q_result = 0;

            W->raycast(&sc,
                    this->local_to_world(ray_p[x], 0),
                    this->local_to_world(ray_p[x]+ray_v[x], 0));

            if (sc.q_result) {
                c->o = sc.q_result;
                c->angle = atan2f(ray_v[x].y, ray_v[x].x);
                c->render_type = CONN_RENDER_SMALL;
                c->f[0] = 0;
                c->f[1] = sc.q_frame;
                c->p = ray_v[x];
                c->p *= sc.q_fraction * .5;
                c->p += ray_p[x];
                c->p = this->local_to_world(c->p, 0);
                c->o_data = sc.q_result->get_fixture_connection_data(sc.q_result_fx);
                G->add_pair(this, sc.q_result, c);
            }
        }
    }
}

void
entity::easy_update()
{
    tmat4_load_identity(this->M);
    tmat4_translate(this->M, this->get_position().x, this->get_position().y, this->get_layer() * LAYER_DEPTH);
    tmat4_rotate(this->M, this->get_angle() * (180.f/M_PI), 0, 0, -1);
    tmat3_copy_mat4_sub3x3(this->N, this->M);
}

void
entity_fast_update(struct tms_entity *t)
{
    entity *ths = static_cast<entity*>(t);

    if (ths->body) {
        const b2Transform &t = ths->body->GetTransform();

        tmat4_load_identity(ths->M);
        //ths->M[0] = t.q.c;
        ths->M[0] = t.q.c * ths->get_scale();
        ths->M[1] =  t.q.s * ths->get_scale();
        ths->M[4] = -t.q.s * ths->get_scale();
        //ths->M[1] =  t.q.s;
        //ths->M[4] = -t.q.s;
        //ths->M[5] = t.q.c;
        ths->M[5] = t.q.c * ths->get_scale();
        ths->M[10] = ths->get_scale();
        ths->M[12] = t.p.x;
        ths->M[13] = t.p.y;
        ths->M[14] = ths->prio * LAYER_DEPTH;

        tmat3_copy_mat4_sub3x3(ths->N, ths->M);
    } else {
        tmat4_load_identity(ths->M);
        tmat4_translate(ths->M, ths->_pos.x, ths->_pos.y, 0);
        tmat4_rotate(ths->M, ths->_angle * (180.f/M_PI), 0, 0, -1);
        tmat3_copy_mat4_sub3x3(ths->N, ths->M);
        /* XXX rotate */
    }
}

void
entity::update()
{
    //static_cast<struct tms_entity*>(this)->update = entity_fast_update;
    entity_fast_update(static_cast<struct tms_entity*>(this));
}

/**
 * Get all chunks that this entity is intersecting with
 * We can get that info by looping through all bodies and checking their
 * contacts.
 *
 * Static bodies don't collide with the sensors so for static bodies
 * we instead calculate chunk intersection using width and height
 **/
void
entity::get_chunk_intersections(std::set<chunk_pos> *chunks)
{
    for (int x=0; x<this->get_num_bodies(); x++) {
        b2Body *b = this->get_body(x);

        if (b) {
            b2ContactEdge *c;
            for (c=b->GetContactList(); c; c = c->next) {
                b2Fixture *f = 0, *my;
                if (c->contact->GetFixtureA()->GetBody() == b) {
                    f = c->contact->GetFixtureB();
                    my = c->contact->GetFixtureA();
                } else {
                    f = c->contact->GetFixtureA();
                    my = c->contact->GetFixtureB();
                }

                if (this->fx && my != this->fx)
                    continue;

                entity *o;
                if (f->IsSensor() && c->contact->IsTouching() && (o = (entity*)f->GetUserData()) && o->g_id == O_CHUNK && f->GetUserData2() == 0) {
                    chunks->insert(((level_chunk*)o)->get_chunk_pos());
                }
            }
        }
    }
}

void
entity::set_locked(bool locked, bool immediate/*=true*/)
{
    this->set_flag(ENTITY_IS_LOCKED, locked);

    if (immediate) {
        if (G && (W->is_paused() && G->state.sandbox)) {
            for (uint32_t x=0; x<this->get_num_bodies(); ++x) {
                b2Body *b = this->get_body(x);

                if (b) {
                    b->SetType(this->gr ? this->gr->get_dynamic_type() : this->get_dynamic_type());
                }
            }
        }
    }
}

void
entity::load_flags(uint64_t f)
{
    if (f & ES_LOCKED) {
        this->set_locked(true, false);
    }

    this->set_moveable(f & ES_MOVEABLE);

    this->set_flag(ENTITY_AXIS_ROT, f & ES_AXIS_ROT);
    this->set_flag(ENTITY_STATE_SLEEPING, (f & ES_SLEEPING));
    if (f & ES_DISABLE_UNLOADING) {
        this->set_flag(ENTITY_DISABLE_UNLOADING, true);
    }
}

uint64_t
entity::save_flags()
{
    uint64_t f = 0;

    if (this->is_locked()) {
        f |= ES_LOCKED;
    }

    if (this->is_moveable()) {
        f |= ES_MOVEABLE;
    }

    if (this->flag_active(ENTITY_AXIS_ROT)) {
        f |= ES_AXIS_ROT;
    }

    if (W->is_playing() && this->get_body(0) && !this->get_body(0)->IsAwake()) {
        f |= ES_SLEEPING;
    }

    if (this->flag_active(ENTITY_DISABLE_UNLOADING)) {
        f |= ES_DISABLE_UNLOADING;
    }

    return f;
}

void
entity::gather_connections(std::set<connection*> *connections, std::set<entity*> *entities)
{
    connection *c = this->conn_ll;

    bool fr = false;
    if (!entities) {
        entities = new std::set<entity*>();
        fr = true;
    }

    entities->insert(this);

    if (c) {
        do {
            connection *next = c->get_next(c->e);
            entity *other = c->get_other(c->e);

            if (!other->is_static()) {
                if (entities->find(other) == entities->end()) {
                    other->gather_connections(connections, entities);
                }
            }

            connections->insert(c);
            c = next;
        } while (c);
    }

    if (fr) {
        delete entities;
    }
}

void
entity::gather_connected_entities(std::set<entity*> *entities, bool include_cables/*=false*/, bool include_custom_conns/*=true*/, bool include_static/*=false*/, bool select_through_layers/*=true*/, int layer/*=-1*/)
{
    connection *c = this->conn_ll;

    if (!this->get_body(0)) {
        return;
    }

    if (this->g_id == O_CHUNK) {
        return;
    }

    if (layer >= 0 && this->get_layer() != layer) {
        return;
    } else if (!select_through_layers && layer < 0) {
        layer = this->get_layer();
    }

    if (this->flag_active(ENTITY_IS_STATIC)) {
        if (include_static) {
            entities->insert(this);
        }
        return;
    }

    entities->insert(this);

    if (c) {
        do {
            entity *other = c->get_other(this);

            if (include_custom_conns || c->type != CONN_CUSTOM) {
                if (!c->destroyed && entities->find(other) == entities->end()) {
                    other->gather_connected_entities(entities, include_cables, include_custom_conns, include_static, select_through_layers, layer);
                }
            }

            c = c->get_next(this);
        } while (c);
    }

    if (include_custom_conns) {
        entity *other = 0;

        if (this->g_id == O_OPEN_PIVOT) other = ((pivot_1*)this)->dconn.o;
        else if (this->g_id == O_OPEN_PIVOT_2) other = ((pivot_2*)this)->p1;
        else if (this->g_id == O_DAMPER) other = ((damper_1*)this)->dconn.o;
        else if (this->g_id == O_DAMPER_2) other = ((damper_2*)this)->d1;
        else if (this->g_id == O_RUBBERBAND) other = ((rubberband_1*)this)->dconn.o;
        else if (this->g_id == O_RUBBERBAND_2) other = ((rubberband_2*)this)->d1;

        if (other && entities->find(other) == entities->end()) {
            other->gather_connected_entities(entities, include_cables, include_custom_conns, include_static, select_through_layers, layer);
        }
    }

    if (include_cables) {
        entity *f = 0;
        edevice *e = this->get_edevice();
        if (e) {
            for (int x=0; x<e->num_s_in; x++) {
                if (e->s_in[x].p) {
                    if (e->s_in[x].p->get_other() && e->s_in[x].p->get_other()->plugged_edev) {
                        f = e->s_in[x].p->get_other()->plugged_edev->get_entity();

                        if (f) {
                            if (entities->find(f) == entities->end()) {
                                f->gather_connected_entities(entities, include_cables, include_custom_conns, include_static, select_through_layers, layer);
                            }
                        }
                    } else if (!e->s_in[x].p->c) {
                        /* the plug doesnt have a cable, it is probably a jumper or receiver or similar */
                        entities->insert(e->s_in[x].p);
                    }
                }
            }
            for (int x=0; x<e->num_s_out; x++) {
                if (e->s_out[x].p) {
                    if (e->s_out[x].p->get_other() && e->s_out[x].p->get_other()->plugged_edev) {
                        f = e->s_out[x].p->get_other()->plugged_edev->get_entity();

                        if (f) {
                            if (entities->find(f) == entities->end()) {
                                f->gather_connected_entities(entities, include_cables, include_custom_conns, include_static, select_through_layers, layer);
                            }
                        }
                    } else if (!e->s_out[x].p->c) {
                        entities->insert(e->s_out[x].p);
                    }
                }

            }
        }
    }
}

void
entity::remove_from_world()
{
    if (this->body && this->body != W->ground) {
        W->b2->DestroyBody(this->body);
    } else if (this->body && this->body == W->ground && this->fx) {
        W->ground->DestroyFixture(this->fx);
    } else if (this->fx && this->gr && this->gr->body) {
        this->gr->body->DestroyFixture(this->fx);
    }

    this->fx = 0;
    this->body = 0;
}

/**
 *
 * ENTITY STATE HANDLING
 *
 * When a level is saved, write_state is called from world. The entity can fill however
 * much data in the buffer as it wishes.
 *
 * When the level is loaded again, the following occurs:
 * 1. Entity allocates
 * 2. on_load(bool created, bool has_state)
 * 3. read_state()
 * 4. add_to_world()
 * 5. init()
 * 6. restore()
 *
 * When a level is loaded without a state buffer, the follow occurs:
 * 1. Entity allocates
 * 2. on_load(bool created, bool has_state)
 * 3. add_to_world()
 * 4. init()
 * 5. setup()
 *
 * Therefore, it is important that we ALWAYS make sure the data changed/set in
 * setup is also modified by restore() or read_state().
 *
 * read_state() is used to restore the state of an entity that is not yet added to
 * the world, we need to keep important data in temporary variables until restore()
 * is called.
 *
 * If an entity leaves and enters the chunk window, only restore() is re-called.
 * read_state() loads the state from a buffer, restore() applies the state
 *
 * read_state() must make sure the lvlbuf skips the whole state
 **/

void
entity::restore()
{
    if (this->flag_active(ENTITY_IS_STATIC)) {
        return;
    }

#ifdef DEBUG
    if (this->get_num_bodies() > 1) {
        tms_warnf("!!! entity::restore() called on %s with more than 1 body, verify this is correct", of::get_object_name_by_gid(this->g_id));
    }

    tms_debugf("applying state for entity %u, vel %f %f, avel %f", this->id, this->state[0], this->state[1], this->state[2]);
#endif
    if (!this->gr && this->get_body(0)) {
        this->get_body(0)->SetLinearVelocity(b2Vec2(this->state[0], this->state[1]));
        this->get_body(0)->SetAngularVelocity(this->state[2]);
    }
}

/* default state read/write saves the current velocity of the body */
void
entity::write_state(lvlinfo *lvl, lvlbuf *lb)
{
    if (this->flag_active(ENTITY_IS_STATIC)) {
        return;
    }

#ifdef DEBUG
    if (this->get_num_bodies() > 1) {
        tms_warnf("!!! entity::write_state() called on %s with more than 1 body, verify this is correct", of::get_object_name_by_gid(this->g_id));
    }

    tms_debugf("writing state for entity %u", this->id);
#endif

    b2Vec2 velocity = this->get_body(0) ? this->get_body(0)->GetLinearVelocity() : b2Vec2(0.f, 0.f);
    float avel = this->get_body(0) ? this->get_body(0)->GetAngularVelocity() : 0.f;

    lb->ensure(3*sizeof(float));
    lb->w_float(velocity.x);
    lb->w_float(velocity.y);
    lb->w_float(avel);
}

void
entity::read_state(lvlinfo *lvl, lvlbuf *lb)
{
    if (this->flag_active(ENTITY_IS_STATIC))
        return;

#ifdef DEBUG
    if (this->get_num_bodies() > 1) {
        tms_warnf("!!! entity::read_state() called on %s with more than 1 body, verify this is correct", of::get_object_name_by_gid(this->g_id));
    }

    tms_debugf("reading state for %u", this->id);
#endif

    this->state[0] = lb->r_float();
    this->state[1] = lb->r_float();
    this->state[2] = lb->r_float();
}

void entity::set_prio(int z)
{
    struct tms_scene *scene = this->scene;

    if (scene) {
        tms_scene_remove_entity(scene, this);
    }

    this->prio = z;

    if (scene) {
        tms_scene_add_entity(scene, this);
    }
}

void entity::set_layer(int z)
{
    if (this->flag_active(ENTITY_DISABLE_LAYERS)) return;

    tms_entity_set_prio_all((struct tms_entity*)this, z);

    if (this->body) {
        b2Filter filter = world::get_filter_for_layer(z, this->layer_mask);

        if (this->body == W->ground) {
            b2Filter curr = this->fx->GetFilterData();

            if (curr.groupIndex < 0) filter.groupIndex = -(1+z);
            else if (curr.groupIndex > 0) filter.groupIndex = 1+z;
            else filter.groupIndex = 0;

            this->fx->SetFilterData(filter);
        } else {
            for (b2Fixture *f = this->body->GetFixtureList(); f; f=f->GetNext()) {
                b2Filter curr = f->GetFilterData();
                if (curr.groupIndex < 0) filter.groupIndex = -(1+z);
                else if (curr.groupIndex > 0) filter.groupIndex = 1+z;
                else filter.groupIndex = 0;
                f->SetFilterData(filter);
            }
        }
    }
}

void entity::create_circle(b2BodyType type,
        float radius, m *m)
{
    tms_assertf(!this->flag_active(ENTITY_IS_COMPOSABLE), "error: create_circle() called on composable object (g_id: %u)", this->g_id);

    b2BodyDef bd;
    bd.type = type;
    bd.position = this->_pos;
    bd.angle = this->_angle;

    b2CircleShape circle;
    circle.m_radius = radius * this->get_scale();

    b2FixtureDef fd;

    fd.shape = &circle;
    fd.density = m->density;
    fd.friction = m->friction;
    fd.restitution = m->restitution;
    fd.filter = world::get_filter_for_layer(this->prio, this->layer_mask);

    b2Body *b = W->b2->CreateBody(&bd);
    (this->fx = b->CreateFixture(&fd))->SetUserData(this);

    this->body = b;
    this->set_layer(this->prio);

    this->width = radius*2.f;
    this->height = this->width;
}

void entity::create_rect(b2BodyType type,
        float width, float height, m *m, b2Fixture **fixture_out /* = NULL */)
{
    tms_assertf(!this->is_composable(), "error: create_rect() called on composable object (g_id: %u)", this->g_id);

    if (!this->body) {
        if (type == b2_staticBody && fixture_out == 0) {
            this->body = W->ground;
        } else {
            b2BodyDef bd;
            bd.type = type;
            bd.position = this->_pos;
            bd.angle = this->_angle;
            b2Body *b = W->b2->CreateBody(&bd);
            this->body = b;
        }
    } else {
        if (this->body != W->ground) {
            this->body->SetType(type);
        } else if (type == b2_dynamicBody) {
            b2BodyDef bd;
            bd.type = type;
            bd.position = this->_pos;
            bd.angle = this->_angle;
            b2Body *b = W->b2->CreateBody(&bd);
            this->body = b;
        }
    }

    b2PolygonShape box;

    this->width = width;
    this->height = height;

    if (this->body == W->ground) {
        box.SetAsBox(this->width, this->height, this->_pos, this->_angle);
    } else {
        box.SetAsBox(width, height);
    }

    box.Scale(this->get_scale());

    b2FixtureDef fd;

    fd.shape = &box;
    fd.density = m->density;
    fd.friction = m->friction;
    fd.restitution = m->restitution;
    fd.filter = world::get_filter_for_layer(this->prio, this->layer_mask);

    b2Fixture *fixture;

    if (type == b2_staticBody) {
        fd.filter.groupIndex = -(1+this->get_layer());
    }

    fixture = this->body->CreateFixture(&fd);
    fixture->SetUserData(this);

    if (fixture_out != NULL) {
        (*fixture_out) = fixture;
    } else {
        this->fx = fixture;
    }

    this->set_layer(this->prio);
}

b2Vec2
entity::get_position()
{
    if (this->body) {
        if (this->body == W->ground) {
            return _pos;
        } else {
            return this->body->GetPosition();
        }
    } else {
        return this->_pos;
    }
}

float
entity::get_angle()
{
    if (this->body) {
        if (this->body == W->ground) {
            return _angle;
        } else {
            return this->body->GetAngle();
        }
    } else {
        return this->_angle;
    }
}

void entity::set_angle(float a)
{
    if (this->body) {
        if (this->body == W->ground) {
            this->_angle = a;
            if (this->fx->GetShape()->m_type == b2Shape::e_polygon) {
                b2PolygonShape *p = ((b2PolygonShape*)this->fx->GetShape());
                p->SetAsBox(this->width, this->height, this->_pos, this->_angle);
                this->fx->Refilter();
            }

            this->recreate_all_connections();
        } else
            this->body->SetTransform(this->body->GetPosition(), a);
    }

    /*
    b2JointEdge *je = 0;
    if (this->get_body(0)) {
        for (je = this->get_body(0)->GetJointList(); je != NULL; je = je->next) {
            je->joint->GetBodyA()->SetAwake(true);
            je->joint->GetBodyB()->SetAwake(true);
        }
    }
    */
}

void
entity::set_position(float x, float y, uint8_t frame/*=0*/)
{
    //tms_debugf("entity::set_position(%.2f, %.2f)", x, y);

    if (!this->flag_active(ENTITY_CAN_MOVE)) {
        return;
    }

    /*
    b2JointEdge *je = 0;
    if (this->get_body(0)) {
        for (je = this->get_body(0)->GetJointList(); je != NULL; je = je->next) {
            je->joint->GetBodyA()->SetAwake(true);
            je->joint->GetBodyB()->SetAwake(true);
        }
    }
    */

    if (this->body) {
        if (this->body == W->ground) {
            this->_pos = b2Vec2(x,y);
            if (this->fx->GetShape()->m_type == b2Shape::e_polygon) {
                b2PolygonShape *p = ((b2PolygonShape*)this->fx->GetShape());
                //this->body->SetActive(false);
                p->SetAsBox(this->width, this->height, this->_pos, this->_angle);
                this->fx->Refilter();
                //this->body->SetActive(true);
            }
            this->recreate_all_connections();
        } else {
            this->body->SetTransform(b2Vec2(x,y), this->body->GetAngle());

            for (b2Fixture *f = this->body->GetFixtureList(); f; f=f->GetNext()) {
                f->Refilter();
            }
        }
    } else {
        this->_pos = b2Vec2(x,y);
    }
}

void
entity::recreate_all_connections()
{
    connection *cc = this->conn_ll;
    if (cc) {
        do {
            cc->create_joint(false);
        } while ((cc = cc->next[cc->e == this ? 0 : 1]));
    }
}

uint32_t
entity::get_num_bodies()
{
    return 1;
}

b2Body*
entity::get_body(uint8_t frame)
{
    return this->body;
}

b2Vec2 entity::local_to_world(b2Vec2 p, uint8_t frame)
{
    b2Body *b = this->get_body(frame);
    if (b) {
        if (b == W->ground) {
            b2Vec2 pos = b->GetWorldPoint(this->_pos);

            float c,s;
            tmath_sincos(this->_angle, &s, &c);

            pos.x += p.x*c - p.y*s;
            pos.y += p.x*s + p.y*c;

            return pos;
        } else
            return b->GetWorldPoint(p);
    } else {
        float c,s;
        tmath_sincos(this->get_angle(), &s, &c);

        b2Vec2 p2;
        p2.x = p.x*c - p.y*s;
        p2.y = p.x*s + p.y*c;

        p2 += this->get_position();

        return p2;
    }

    return b2Vec2(0,0);
}

b2Vec2 entity::local_to_body(b2Vec2 p, uint8_t frame)
{
    if (this->get_body(frame) == W->ground) {
        float c,s;
        tmath_sincos(this->_angle, &s, &c);

        b2Vec2 p2;

        p2.x = p.x*c - p.y*s;
        p2.y = p.x*s + p.y*c;

        p2 += this->_pos;

        return p2;
    } else
        return p;
}

b2Vec2 entity::local_vector_to_body(b2Vec2 p, uint8_t frame)
{
    return p;
}

b2Vec2 entity::world_to_body(b2Vec2 p, uint8_t frame)
{
    b2Body *b = this->get_body(frame);
    if (b) return b->GetLocalPoint(p);
    return b2Vec2(0,0);
}

b2Vec2 entity::world_to_local(b2Vec2 p, uint8_t frame)
{
    b2Body *b = this->get_body(frame);
    if (b) {
        if (b == W->ground) {
            b2Vec2 pos = p;

            b2Vec2 l = b->GetLocalPoint(p);

            l -= this->_pos;

            float c,s;
            tmath_sincos(-this->_angle, &s, &c);

            b2Vec2 wp;
            wp.x = l.x*c - l.y*s;
            wp.y = l.x*s + l.y*c;

            return wp;
        } else
            return b->GetLocalPoint(p);
    }
    return b2Vec2(0,0);
}

void
entity::set_propertyi8(uint8_t id, uint8_t v)
{
    if (id < this->num_properties)
        this->properties[id].v.i8 = v;
}

void
entity::set_property(uint8_t id, float v)
{
    if (id < this->num_properties)
        this->properties[id].v.f = v;
}

void
entity::set_property(uint8_t id, uint32_t v)
{
    if (id < this->num_properties)
        this->properties[id].v.i = v;
}

void
entity::set_property(uint8_t id, const char *v, uint32_t len)
{
    if (id < this->num_properties) {
        if (this->properties[id].v.s.buf)
            free(this->properties[id].v.s.buf);

        this->properties[id].v.s.buf = (char*)malloc(len);
        memcpy(this->properties[id].v.s.buf, v, len);
        this->properties[id].v.s.len = len;
    }
}

void
entity::set_property(uint8_t id, const char *v)
{
    if (id < this->num_properties) {
        if (this->properties[id].v.s.buf)
            free(this->properties[id].v.s.buf);

        this->properties[id].v.s.buf = strdup(v);
        this->properties[id].v.s.len = strlen(v);

        if (G && W && W->is_paused()) {
            G->state.modified = true;
        }
    }
}

void
entity::set_num_properties(uint8_t num)
{
    this->num_properties = num;
    this->properties = new property[num];
    for (uint8_t x=0; x<num; ++x) {
        this->properties[x].clear();
    }
}

void
entity::pre_write(void)
{
    if (!this->gr) {
        this->_pos = this->get_position();
        this->_angle = this->get_angle();
    }
}

void
entity::remove_connection(connection *cr)
{
    connection *c = this->conn_ll;
    connection **cc = &this->conn_ll;

    if (c) {
        do {
            connection **ccn = &c->next[(c->e == this) ? 0 : 1];
            if (c == cr) {
                *cc = *ccn;
                break;
            }
            cc = ccn;
            c = *ccn;
        } while (c);
    }

    /*
    if (this->gr) {
        this->gr->rebuild(G);
    }
    */
}

void
entity::destroy_connection(connection *cr)
{
    connection *c = this->conn_ll;
    connection **cc = &this->conn_ll;

    tms_infof("destroy connection");

    if (c) {
        do {
            connection **ccn = &c->next[(c->e == this) ? 0 : 1];
            if (c == cr) {
                *cc = *ccn;

                cr->destroy_joint();
                W->erase_connection(cr);
                entity *other = ((this == cr->e) ? cr->o : cr->e);
                other->remove_connection(cr);

                if (!c->owned) {
                    delete c;
                } else {
                    c->pending = true;
                }

                tms_infof("connection found and destroyed");

                break;
            }
            cc = ccn;
            c = *ccn;
        } while (c);
    } else {
        tms_infof("No connection to destroy.");
    }

    if (this->gr) {
        this->gr->rebuild();
    }
}

void
entity::on_grab(game *g)
{
    if (this->body && this->conn_ll == 0) {
        //if (!this->conn_ll) this->body->SetFixedRotation(true);

        if (this->body->GetType() == b2_staticBody && this->fx) {
            this->fx->SetFilterData(world::get_filter_for_layer(0, 0));
            this->fx->Refilter();
        } else
            this->body->SetActive(false);
    }

    if (this->get_body(0) && this->get_body(0)->GetType() == b2_dynamicBody) {
        b2JointEdge *je = 0;

        for (je = this->get_body(0)->GetJointList(); je != NULL; je = je->next) {

            if (je->joint->GetType() == e_revoluteJoint) {
                continue;
            }

            if (je->joint->GetBodyA()->GetType() == b2_staticBody
                    || je->joint->GetBodyB()->GetType() == b2_staticBody) {
                this->set_flag(ENTITY_CAN_MOVE, false);
                break;
            }
        }
    }
}

void
entity::on_release(game *g)
{
    this->set_flag(ENTITY_CAN_MOVE, true);
    if (this->body) {
        //this->body->SetFixedRotation(false);
        //this->body->GetFixtureList()->SetFilterData(world::get_filter_for_layer(this->get_layer(), this->layer_mask));
        //
        if (this->body->GetType() == b2_staticBody && this->fx) {
            this->body->SetActive(false);
            this->fx->SetFilterData(world::get_filter_for_layer(this->get_layer(), this->layer_mask));
            this->fx->Refilter();
        }

        this->body->SetActive(true);
    }
}

void
entity::on_touch(b2Fixture *my, b2Fixture *other)
{
}

void
entity::on_untouch(b2Fixture *my, b2Fixture *other)
{
}

bool
entity::compatible_with(entity *o)
{
    return (this->g_id == o->g_id && this->num_properties == o->num_properties);
}

void
entity::disconnect_all()
{
    connection *c = this->conn_ll;

    if (c) {
        do {
            connection *next = c->next[(c->e == this) ? 0 : 1];
            entity *other = ((this == c->e) ? c->o : c->e);

            if (!c->fixed) {
                c->destroy_joint();

                W->erase_connection(c);

                other->remove_connection(c);
                if (!c->owned) {
                    delete c;
                } else {
                    c->pending = true;
                }
            } else {
                tms_warnf("fixed connection must not be in conn_ll");
            }

            c = next;
        } while (c);
    }

    this->conn_ll = 0;

    if (this->gr) {
        this->gr->rebuild();
    } else if (!W->is_paused() && this->get_body(0) && this->get_body(0)->GetType() == b2_staticBody && !this->flag_active(ENTITY_IS_STATIC)) {
        this->get_body(0)->SetType(b2_dynamicBody);
    }
}

connection *
entity_simpleconnect::load_connection(connection &conn)
{
    this->c = conn;
    return &this->c;
}

float32
entity_simpleconnect::ReportFixture(b2Fixture *f, const b2Vec2 &pt, const b2Vec2 &nor, float32 fraction)
{
    if (f->IsSensor()) {
        return -1.f;
    }

    b2Body *b = f->GetBody();
    entity *e = (entity*)f->GetUserData();

    if (e && e != this && e->allow_connections()
            //&& e->get_layer() == this->get_layer() && (e->layer_mask & 6) != 0
            && world::fixture_in_layer(f, this->get_layer(), 6)
            ) {
        this->query_result = e;
        this->query_result_fx = f;
        this->query_frame = (uint8_t)(uintptr_t)b->GetUserData();
    }

    return -1;
}

void
entity_simpleconnect::find_pairs()
{
    if (this->c.pending) {
        this->query_result = 0;

        W->raycast(this, this->local_to_world(this->query_pt, 0), this->local_to_world(this->query_vec+this->query_pt, 0));

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

connection *
entity_multiconnect::load_connection(connection &conn)
{
    this->c_side[conn.o_index] = conn;
    return &this->c_side[conn.o_index];
}

void
entity_multiconnect::find_pairs()
{
    this->sidecheck4(this->c_side);
}

char*
property::stringify()
{
    char *buf = (char*)calloc(1,
            3 +  /* type */
            +1 /* delimiter */
            +(this->type == P_INT || this->type == P_ID ? 11 : 0)
            +(this->type == P_FLT ? 20 : 0)
            +(this->type == P_STR ? this->v.s.len : 0)
            +(this->type == P_INT8? 3 : 0)
            +1
            );

    switch (this->type) {
        case P_INT: sprintf(buf, "%d;%d", this->type, this->v.i); break;
        case P_FLT: sprintf(buf, "%d;%.8f", this->type, this->v.f); break;
        case P_STR: sprintf(buf, "%d;%s", this->type, this->v.s.buf); break;
        case P_INT8:sprintf(buf, "%d;%d", this->type, this->v.i8); break;
        case P_ID  :sprintf(buf, "%d;%d", this->type, this->v.i); break;
        default:
            tms_errorf("Unknown property type: %d", this->type);
            free(buf);
            return 0;
    }

    return buf;

}

void
property::parse(char *buf)
{
    char tmp[32];
    char *pch;

    pch = strchr(buf, ';');
    if (pch == NULL) {
        tms_errorf("Expected character not found while parsing property string.");
        return;
    }

    this->clear();

    memcpy(tmp, buf, 1);
    tmp[1] = '\0';
    this->type = (uint8_t)atoi(tmp);

    switch (this->type) {
        case P_INT:
        case P_ID:
            strcpy(tmp, buf+2);
            this->v.i = (uint32_t)atoi(tmp);
            break;

        case P_FLT:
            strcpy(tmp, buf+2);
            this->v.f = (float)atof(tmp);
            break;

        case P_STR:
            if (this->v.s.buf)
                free(this->v.s.buf);

            this->v.s.buf = strdup(buf+2); /* XXX: wtf is going on here? */
            this->v.s.len = strlen(buf+2); /* XXX: wtf is going on here? */
            break;

        case P_INT8:
            strcpy(tmp, buf+2);
            this->v.i8 = (uint8_t)atoi(tmp);
            break;
    }
}

ghost::ghost()
{
    this->set_flag(ENTITY_IS_BETA,              true);
    this->set_flag(ENTITY_ALLOW_CONNECTIONS,    false);
    this->set_flag(ENTITY_ALLOW_ROTATION,       false);
    this->set_flag(ENTITY_HAS_CONFIG,           true);

    this->dialog_id = DIALOG_SHAPEEXTRUDER;

    this->set_mesh(mesh_factory::get_mesh(MODEL_BOX0));
    this->set_material(&m_red);
    this->layer_mask = 1;
    this->update_method = ENTITY_UPDATE_CUSTOM;

    this->set_num_properties(4);

    this->properties[0].type = P_FLT;
    this->properties[0].v.f = 0.f; /* extra right */

    this->properties[1].type = P_FLT;
    this->properties[1].v.f = 0.f; /* extra up */

    this->properties[2].type = P_FLT;
    this->properties[2].v.f = 0.f; /* extra left */

    this->properties[3].type = P_FLT;
    this->properties[3].v.f = 1.f; /* extra down */

    this->c.init_owned(0, this);
    this->c.type = CONN_CUSTOM;
}

connection*
ghost::load_connection(connection &conn)
{
    this->c = conn;
    return &this->c;
}

void
ghost::update()
{
    if (G && !W->is_paused()) {
        memset(this->M, 0, sizeof(float)*16);
    } else  {
        entity *ths = static_cast<entity*>(this);

        /*if (this->conn_ll) {
            tms_infof("rendering with conn_ll");
            entity *e = this->c.o;
            b2Vec2 pos = e->get_position();
            tmat4_load_identity(ths->M);
            tmat4_translate(ths->M, pos.x, pos.y, 0);
            tmat3_copy_mat4_sub3x3(ths->N, ths->M);
        } else */if (ths->body) {
            //tms_infof("rendering with body");
            b2Transform t;
            t = ths->body->GetTransform();

            tmat4_load_identity(ths->M);
            ths->M[0] = t.q.c;
            ths->M[1] = t.q.s;
            ths->M[4] = -t.q.s;
            ths->M[5] = t.q.c;
            ths->M[12] = t.p.x;
            ths->M[13] = t.p.y;
            ths->M[14] = ths->prio * LAYER_DEPTH;

            tmat3_copy_mat4_sub3x3(ths->N, ths->M);
        } else {
            //tms_infof("rendering with _pos");
            tmat4_load_identity(ths->M);
            tmat4_translate(ths->M, ths->_pos.x, ths->_pos.y, 0);
            tmat4_rotate(ths->M, ths->_angle * (180.f/M_PI), 0, 0, -1);
            tmat3_copy_mat4_sub3x3(ths->N, ths->M);
            /* XXX rotate */
        }
    }
}

void
ghost::connection_create_joint(connection *c)
{
    c->j = (b2Joint*)1;
    c->max_force = INFINITY;

    if (W->is_paused()) {
        b2WeldJointDef wjd;
        wjd.localAnchorA = c->e->local_to_body(b2Vec2(0.f, 0.f), c->f[0]);
        wjd.localAnchorB = c->o->local_to_body(b2Vec2(0.f, 0.f), c->f[1]);

        wjd.bodyA = c->e->get_body(c->f[0]);
        wjd.bodyB = c->o->get_body(c->f[1]);
        wjd.referenceAngle = c->o->get_body(c->f[1])->GetAngle() - c->e->get_body(c->f[0])->GetAngle();
        wjd.collideConnected = false;
        wjd.frequencyHz = 0.f;

        c->j = W->b2->CreateJoint(&wjd);
    }
}

bool
ghost::connection_destroy_joint(connection *c)
{
    return !(c->j != (void*)1 && c->j != 0);
}

bool
ghost::ReportFixture(b2Fixture *f)
{
    entity *e = static_cast<entity*>(f->GetUserData());

    if (e && e != this) {
        uint8_t frame = (uint8_t)(uintptr_t)(f->GetBody()->GetUserData());
        if (this->c.pending && frame == 0 && e->get_body(frame)->GetFixtureList()[0].TestPoint(this->get_position())
                    && e->flag_active(ENTITY_IS_COMPOSABLE)
            && e->get_layer() == this->get_layer()) {
            this->c.type = CONN_CUSTOM;
            this->c.o = e;
            this->c.p = e->get_position();
            this->c.o_data = e->get_fixture_connection_data(f);
            G->add_pair(this, e, &this->c);
        }
    }

    return true;
}

void
ghost::init()
{
    //this->create_rect(w, b2_dynamicBody, .5f, .5, this->material);

    if (this->conn_ll && this->c.o && this->c.o->flag_active(ENTITY_IS_COMPOSABLE)) {
        composable *other = static_cast<composable*>(this->c.o);

        if (other) {
            float w = other->get_width();
            float h = other->height;

            b2Filter f = other->fx->GetFilterData();
            f.groupIndex = 1+this->get_layer();
            other->fx->SetFilterData(f);
            //other->fx->SetSensor(true);

            b2PolygonShape sh;

            b2Vec2 vertices[4] = {
                other->local_to_body(b2Vec2(w+this->properties[0].v.f, h+this->properties[1].v.f), 0),
                other->local_to_body(b2Vec2(-(w+this->properties[2].v.f), h+this->properties[1].v.f), 0),
                other->local_to_body(b2Vec2(-(w+this->properties[2].v.f), -(h+this->properties[3].v.f)), 0),
                other->local_to_body(b2Vec2(w+this->properties[0].v.f, -(h+this->properties[3].v.f)), 0)
            };

            sh.Set(vertices, 4);

            b2FixtureDef fd;
            fd.shape = &sh;
            fd.density = 0.00001f;
            fd.friction = other->fd.friction;
            fd.restitution = other->fd.restitution;
            fd.filter = other->fd.filter;
            fd.filter.groupIndex = -(1+this->get_layer());

            other->get_body(0)->CreateFixture(&fd)->SetUserData(other);
        }
    }
}

void
ghost::find_pairs()
{
    b2AABB aabb;
    b2Vec2 p = this->get_position();
    aabb.lowerBound = b2Vec2(p.x-.1f, p.y-.1f);
    aabb.upperBound = b2Vec2(p.x+.1f, p.y+.1f);
    W->b2->QueryAABB(this, aabb);
}

void
ghost::add_to_world()
{
    if (W->is_paused()) {
        this->create_rect(b2_dynamicBody, .25f, .25f, this->material);
        this->body->GetFixtureList()[0].SetSensor(true);
    }
}

/**
 * If no personalized tooltip is defined, output the objects name.
 **/
void
entity::write_tooltip(char *out)
{
}

void
entity::write_quickinfo(char *out)
{
    sprintf(out, "%s", this->get_name());
}

void
entity::write_object_id(char *out)
{
    sprintf(out, "\nid: %u, g_id: %u", this->id, this->g_id);
}

void
entity::subscribe(entity *target, int event, void (*listener_func)(entity *self, void *userdata), void *userdata/*=0*/)
{
    //tms_debugf("%p subscribe to %p", this, target);

    // Keep a reference of all the entities whose events we subscribe to
    std::pair<std::map<entity*, int>::iterator, bool> ret;
    this->subscriptions.insert(target);

    entity_listener l = {
        this,
        userdata,
        listener_func
    };

    target->listeners.insert(std::pair<int, entity_listener>(event, l));
}

void
entity::unsubscribe(entity *target)
{
    this->subscriptions.erase(target);
}

void
entity::signal(int event)
{
    typedef std::multimap<int, entity_listener>::iterator iterator;
    std::pair<iterator, iterator> ip = this->listeners.equal_range(event);

    iterator it = ip.first;
    for (; it != ip.second; ++it) {
        void (*cb)(entity*,void*) = (void (*)(entity*,void*))it->second.cb;
        cb(it->second.self, it->second.userdata);
    }
}

/**
 * Reset any resettable flags in here
 **/
void
entity::reset_flags(void)
{
    if (this->flag_active(ENTITY_WAS_HIDDEN)) {
        G->add_entity(this);
    }

    this->set_flag(ENTITY_IS_ABSORBED, false);
    this->set_flag(ENTITY_WAS_HIDDEN, false);
}

bool
entity::is_motor()
{
    return this->g_id == O_DC_MOTOR || this->g_id == O_SERVO_MOTOR || this->g_id == O_SIMPLE_MOTOR;
}

bool
entity::is_high_prio()
{
    return
        this->flag_active(ENTITY_IS_HIGH_PRIO)
        || (W->is_playing() && this->is_control_panel());
}

void
entity::update_protection_status()
{
    uint8_t new_status = 0;
    bool include_platforms = true;

    if (!this->interacted_with) {
        std::set<entity*> *loop = new std::set<entity*>();
        this->gather_connected_entities(loop, false, true, true);

        /* First pass: Check if the group should be protected or not. */
        for (std::set<entity*>::iterator it = loop->begin(); it != loop->end(); ++it) {
            entity *e = *it;
            if (e->g_id == O_AUTO_PROTECTOR) {
                new_status |= ENTITY_PROT_AUTOPROTECTOR;

                if (new_status & ENTITY_PROT_PLATFORM) {
                    /* break out if we're already protected by the other form */
                    break;
                }
            } else if (e->g_id == O_PLATFORM) {
                new_status |= ENTITY_PROT_PLATFORM;

                if (new_status & ENTITY_PROT_AUTOPROTECTOR) {
                    /* break out if we're already protected by the other form */
                    break;
                }
            }
        }

        /* Second pass: Apply the new protection status to every entity in the loop */
        for (std::set<entity*>::iterator it = loop->begin(); it != loop->end(); ++it) {
            entity *e = *it;

            e->protection_status = new_status;
        }

        delete loop;
    }

    /* I'm not sure if *this* is included in the loop, so we apply the protection
     * status here just to be safe */
    this->protection_status = new_status;
}

bool
entity::is_protected(bool include_platforms/*=false*/)
{
    if (!W->is_paused() && this->protection_status & ENTITY_NOT_SEARCHED) {
        this->update_protection_status();
    }

    if (this->protection_status & ENTITY_PROT_AUTOPROTECTOR) {
        return true;
    }

    if (include_platforms && this->protection_status & ENTITY_PROT_PLATFORM) {
        return true;
    }

    return false;
}

void
entity::prepare_fadeout()
{
    if (this->flag_active(ENTITY_IS_COMPOSABLE) && (this->update_method == ENTITY_UPDATE_GROUPED || this->update_method == ENTITY_UPDATE_FASTBODY)) {
        tmat4_load_identity(this->M);
        b2Vec2 p = this->get_position();
        float a = this->get_angle();

        tmat4_translate(this->M, p.x, p.y, this->get_layer()*LAYER_DEPTH);
        tmat4_rotate(this->M, a * (180.f/M_PI), 0.f, 0.f, -1.f);
        tmat3_copy_mat4_sub3x3(this->N, this->M);
    }/* else
        this->update();*/

    //this->M[14]+=.01f;
}

void
entity::entity_damage(float dmg)
{
    this->entity_health -= dmg;

    G->add_hp(this, this->entity_health / ENTITY_MAX_HEALTH, TV_HP_GRAY, 0.5f, true);

    if (this->entity_health <= 0.f) {
        G->lock();
        this->drop_worth();
        this->disconnect_all();
        G->absorb(this);
        G->unlock();
    }
}

void
entity::drop_worth()
{
    const struct worth &w = this->worth;

    for (int x=0; x<NUM_RESOURCES; ++x) {
        uint32_t num = w.resources[x];

        if (num) {
            uint32_t split = 1;

            if (num > 5) {
                split = 15;
            }

            while (num > 0) {
                uint32_t out = std::min(split, num);

                resource *r = static_cast<resource*>(of::create(O_RESOURCE));
                if (r) {
                    r->set_resource_type(x);
                    r->set_amount(out);
                    r->set_position(this->get_position().x, this->get_position().y);
                    r->set_layer(this->get_layer());
                    G->emit(r, this, b2Vec2(((rand()%100)-50)/100.f, ((rand()%100)-50)/100.f));
                }

                num -= out;
            }
        }
    }
}

worth::worth()
{
    memset(this->resources, 0, sizeof(this->resources));
    this->oil = 0.f;
}

struct worth&
worth::add(uint8_t resource_type, uint32_t num)
{
    this->resources[resource_type] = num;

    return *this;
}

struct worth&
worth::add_oil(float val)
{
    this->oil = val;

    return *this;
}

struct worth&
worth::add(const struct worth& worth)
{
    this->oil += worth.oil;
    for (int x=0; x<NUM_RESOURCES; ++x) {
        this->resources[x] += worth.resources[x];
    }

    return *this;
}

float
entity::distance_to_fixture(const b2Vec2 &query_point, b2Fixture *fx)
{
    float dist = INFINITY;

    switch (fx->GetType()) {
        case b2Shape::e_circle:
            {
                b2Vec2 p = fx->GetBody()->GetWorldPoint(((b2CircleShape*)fx->GetShape())->m_p);
                p = query_point - p;
                dist = p.Length() - ((b2CircleShape*)fx->GetShape())->m_radius;
            }
            break;

        case b2Shape::e_polygon:;
            {
                b2Vec2 *ov = (((b2PolygonShape*)fx->GetShape())->m_vertices);
                int num_verts = ((b2PolygonShape*)fx->GetShape())->m_count;

                tvec2 verts[num_verts];
                for (int x=0; x<num_verts; x++) {
                    b2Vec2 tv = fx->GetBody()->GetWorldPoint(ov[x]);
                    verts[x].x = tv.x;
                    verts[x].y = tv.y;
                }

                dist = tintersect_point_poly_distance((tvec2*)&(query_point), verts, num_verts);
            }
            break;

        default:
#ifdef DEBUG
            tms_fatalf("Unhandled fixture type in entity::distance_to_fixture: %d", fx->GetType());
#endif
            break;
    }

    return dist;
}

tvec2
entity::get_nearest_point(const b2Vec2 &query_point, b2Fixture *fx)
{
    tvec2 nearest;

    switch (fx->GetType()) {
        case b2Shape::e_circle:
            {
                b2Vec2 p = fx->GetBody()->GetWorldPoint(((b2CircleShape*)fx->GetShape())->m_p);
                b2Vec2 v = query_point - p;
                v*=1.f/v.Length();
                nearest = (tvec2){p.x+v.x *((b2CircleShape*)fx->GetShape())->m_radius , p.y+v.y*((b2CircleShape*)fx->GetShape())->m_radius};
            }
            break;

        case b2Shape::e_polygon:;
            {
                b2Vec2 *ov = (((b2PolygonShape*)fx->GetShape())->m_vertices);
                int num_verts = ((b2PolygonShape*)fx->GetShape())->m_count;

                tvec2 verts[num_verts];
                for (int x=0; x<num_verts; x++) {
                    b2Vec2 tv = fx->GetBody()->GetWorldPoint(ov[x]);
                    verts[x].x = tv.x;
                    verts[x].y = tv.y;
                }

                nearest = tintersect_point_poly_nearest((tvec2*)&(query_point), verts, num_verts);
            }
            break;

        default:
#ifdef DEBUG
            tms_fatalf("Unhandled fixture type in entity::distance_to_fixture: %d", fx->GetType());
#endif
            break;
    }

    return nearest;
}
