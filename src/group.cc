#include "group.hh"
#include "world.hh"
#include "game.hh"
#include "material.hh"
#include "model.hh"
#include "object_factory.hh"
#include <vector>

struct wooden_vert {
    tvec3 position;
    tvec3 normal;
    tvec2 texcoord;
} __attribute__ ((packed));

struct plastic_vert {
    tvec3 position;
    tvec3 normal;
    tvec3 color;
} __attribute__ ((packed));

struct cvert {
    tvec3 p;
    tvec3 n;
    tvec2 u;
} __attribute__((packed));

void
group::update()
{
    b2Transform t;
    t = this->body->GetTransform();
    //tmat4_load_identity(this->M);
    this->M[0] = t.q.c;
    this->M[1] = t.q.s;
    this->M[4] = -t.q.s;
    this->M[5] = t.q.c;
    this->M[12] = t.p.x;
    this->M[13] = t.p.y;
    this->M[14] = this->prio * LAYER_DEPTH;
    tmat3_copy_mat4_sub3x3(this->N, this->M);

    for (int x=0; x<3; x++) {
        tmat4_copy(this->wooden_entity[x].M, this->M);
        this->wooden_entity[x].M[14] = x*LAYER_DEPTH;
        tmat4_copy(this->plastic_entity[x].M, this->wooden_entity[x].M);
        tmat3_copy(this->wooden_entity[x].N, this->N);
        tmat3_copy(this->plastic_entity[x].N, this->N);
    }
}

group::group()
{
    this->set_flag(ENTITY_IS_MOVEABLE, true);
    this->body = 0;
    this->type = ENTITY_GROUP;

    this->wooden_count[0] = 0;
    this->wooden_count[1] = 0;
    this->wooden_count[2] = 0;

    this->plastic_count[0] = 0;
    this->plastic_count[1] = 0;
    this->plastic_count[2] = 0;

    this->origo_offset = b2Vec2(0.f, 0.f);

    this->update_method = ENTITY_UPDATE_CUSTOM;
    this->curr_update_method = ENTITY_UPDATE_CUSTOM;

    this->entities.clear();
    this->connections.clear();

    this->va = tms_varray_alloc(2);
    this->vbuf = tms_gbuffer_alloc(1); /* XXX */
    this->ibuf = tms_gbuffer_alloc(1);
    tms_varray_map_attribute(this->va, "position", 3, GL_FLOAT, this->vbuf);
    tms_varray_map_attribute(this->va, "normal", 3, GL_FLOAT, this->vbuf);

    tms_varray_init(&this->wooden_va, 3);
    tms_gbuffer_init(&this->wooden_vbuf, 0);
    tms_gbuffer_init(&this->wooden_ibuf, 0);
    tms_varray_map_attribute(&this->wooden_va, "position", 3, GL_FLOAT, &this->wooden_vbuf);
    tms_varray_map_attribute(&this->wooden_va, "normal", 3, GL_FLOAT, &this->wooden_vbuf);
    tms_varray_map_attribute(&this->wooden_va, "texcoord", 2, GL_FLOAT, &this->wooden_vbuf);

    tms_varray_init(&this->plastic_va, 3);
    tms_gbuffer_init(&this->plastic_vbuf, 0);
    tms_gbuffer_init(&this->plastic_ibuf, 0);
    tms_varray_map_attribute(&this->plastic_va, "position", 3, GL_FLOAT, &this->plastic_vbuf);
    tms_varray_map_attribute(&this->plastic_va, "normal", 3, GL_FLOAT, &this->plastic_vbuf);
    tms_varray_map_attribute(&this->plastic_va, "color", 3, GL_FLOAT, &this->plastic_vbuf);

    for (int x=0; x<3; x++) {
        tms_mesh_init(&this->wooden_mesh[x], &this->wooden_va, &this->wooden_ibuf);
        tms_entity_init(&this->wooden_entity[x]);
        tms_entity_set_mesh(&this->wooden_entity[x], &this->wooden_mesh[x]);
        tms_entity_set_material(&this->wooden_entity[x], &m_wood);
        this->wooden_entity[x].prio = x;

        tms_mesh_init(&this->plastic_mesh[x], &this->plastic_va, &this->plastic_ibuf);
        tms_entity_init(&this->plastic_entity[x]);
        tms_entity_set_mesh(&this->plastic_entity[x], &this->plastic_mesh[x]);
        tms_entity_set_material(&this->plastic_entity[x], &m_pixel);
        this->plastic_entity[x].prio = x;
    }
    this->prio = 0;
    //this->prio_bias = 1;

    this->mesh = tms_mesh_alloc(va, this->ibuf);
    this->mesh->primitive_type = GL_TRIANGLES;
    this->set_material(&m_conn_no_ao);

    tmat4_load_identity(this->M);
    tmat3_load_identity(this->N);
}

group::~group()
{
    this->remove_from_world();
}

void
group::create_mesh(void)
{
    float rm[16];

    struct cvert {
        tvec3 p;
        tvec3 n;
        tvec2 u;
    } __attribute__((packed));

    struct vert {
        tvec3 p;
        tvec3 n;
    };

    //struct tms_mesh *cylinder = tms_meshfactory_get_cylinder();
    struct tms_mesh *cylinder = mesh_factory::get_mesh(MODEL_PLATEJOINT);
    struct tms_gbuffer *c_vbuf = cylinder->vertex_array->gbufs[0].gbuf;
    struct tms_gbuffer *c_ibuf = cylinder->indices;

    //struct tms_mesh *cylinder_2 = tms_meshfactory_get_cylinder();
    struct tms_mesh *cylinder_2 = mesh_factory::get_mesh(MODEL_PIVOTJOINT);
    struct tms_gbuffer *c_vbuf_2 = cylinder_2->vertex_array->gbufs[0].gbuf;
    struct tms_gbuffer *c_ibuf_2 = cylinder_2->indices;

    int _num_v_1 = cylinder->v_count;//c_vbuf->size / sizeof(struct cvert);
    int _num_i_1 = cylinder->i_count;//c_ibuf->size / sizeof(uint16_t);

    int _num_v_2 = cylinder_2->v_count;//c_vbuf->size / sizeof(struct cvert);
    int _num_i_2 = cylinder_2->i_count;//c_ibuf->size / sizeof(uint16_t);

    //tms_infof("numv: %d, numi: %d", num_v, num_i);
    //exit(0);
    //

    int _vsize = _num_v_1 > _num_v_2 ? _num_v_1 : _num_v_2;
    int _isize = _num_i_1 > _num_i_2 ? _num_i_1 : _num_i_2;

    struct cvert *_cv_1 = (struct cvert*)((char*)tms_gbuffer_get_buffer(c_vbuf)+cylinder->v_start);
    uint16_t *_ci_1 = (uint16_t*)((char*)tms_gbuffer_get_buffer(c_ibuf)+cylinder->i_start*2);

    struct cvert *_cv_2 = (struct cvert*)((char*)tms_gbuffer_get_buffer(c_vbuf_2)+cylinder_2->v_start);
    uint16_t *_ci_2 = (uint16_t*)((char*)tms_gbuffer_get_buffer(c_ibuf_2)+cylinder_2->i_start*2);

    tms_gbuffer_realloc(this->vbuf, this->connections.size()*_vsize*sizeof(struct vert));
    tms_gbuffer_realloc(this->ibuf, this->connections.size()*_isize*sizeof(uint16_t));

    struct vert *v = (struct vert*)tms_gbuffer_get_buffer(this->vbuf);
    uint16_t *i = (uint16_t*)tms_gbuffer_get_buffer(this->ibuf);

    int vc = 0;
    int vi = 0;

    for (int x=0; x<this->connections.size(); x++) {
        connection *c = this->connections[x];

        /* dont render connection for brdevices on breadboard */
        if (c->e->g_id == O_BREADBOARD && c->o->flag_active(ENTITY_IS_BRDEVICE)) {
            continue;
        }

        if (c->render_type == CONN_RENDER_HIDE) {
            continue;
        }

        struct cvert *cv;
        uint16_t *ci;
        int num_v, num_i;
        uint16_t ibase;

        if (c->multilayer || (
                    (c->e->layer_mask & c->o->layer_mask) == 0)
            || c->render_type == CONN_RENDER_NAIL) {
            num_v = _num_v_2;
            num_i = _num_i_2;
            cv = _cv_2;
            ci = _ci_2;
            ibase = (cylinder_2->v_start/sizeof(struct cvert));
        } else {
            num_v = _num_v_1;
            num_i = _num_i_1;
            cv = _cv_1;
            ci = _ci_1;
            ibase = (cylinder->v_start/sizeof(struct cvert));
        }

        float scale = 1.f;
        float zs = 1.f;

        if (c->render_type == CONN_RENDER_SMALL) {
            if (c->multilayer) {
                zs = .75f;
            } else {
                scale = .375f;
            }
        }

        //tms_infof("cp %f %f", c->p.x, c->p.y);
        //
        bool is_splank = ( (c->e->layer_mask & c->o->layer_mask) == 0);
        bool rot = !c->multilayer && (!is_splank);

        tmat4_load_identity(rm);

        if (rot) {
            tmat4_rotate(rm, (c->e->_angle+c->angle)*(180.f/M_PI)+90, 0.f, 0.f, -1.f);
            //tmat4_rotate(rm, 90, 0.f, 1.f, 0.f);
        } else {
            //tmat4_rotate(rm, 90, 1.f, 0.f, 0.f);
        }

        if (c->render_type == CONN_RENDER_NAIL) {
            tmat4_rotate(rm, 90, 1,0,0);
            zs = .5f;
        }

        for (int y=0; y<num_v; y++) {
            float z_scale = 1.0f;
            float z_offset = 0.f;

#if 0
            if (c->multilayer) {
                int ldiff = c->e->get_layer() - c->o->get_layer();

                z_scale = 1.f;

                z_scale *= (float)(c->sublayer_dist+1) / 4.f;
                //z_scale += .25f;

                if (ldiff > 0) {
                    z_offset = -(z_scale/1.8f * LAYER_DEPTH);
                } else {
                    z_offset =  (z_scale/1.8f * LAYER_DEPTH);
                }

                int l0 = 1;

                /* initial layer mask */
                if (c->layer_mask & 1) {
                } else if (c->layer_mask & 2) {
                    l0 += 1;
                } else if (c->layer_mask & 3) {
                    l0 += 2;
                } else if (c->layer_mask & 4) {
                    l0 += 3;
                }

                if (c->o->layer_mask & 4) {
                } else if (c->o->layer_mask & 3) {
                    l0 += 1;
                } else if (c->o->layer_mask & 2) {
                    l0 += 2;
                } else if (c->o->layer_mask & 1) {
                    l0 += 3;
                }

                z_offset = l0 * (LAYER_DEPTH/4.f);

                tms_infof("l0: %d", l0);
                tms_infof("c->sublayer_dist: %d --------------", c->sublayer_dist);
                tms_infof("z scale: %.2f --------------", z_scale);
                tms_infof("cvpz: %.2f --------------", cv[y].p.z);
            }

            //z_scale = 1.0f;
            //z_offset = 0.f;
            #endif

            tvec4 p = (tvec4){cv[y].p.x*scale, cv[y].p.y*scale, cv[y].p.z * z_scale, 1.f};
            tvec4 n = (tvec4){cv[y].n.x, cv[y].n.y, cv[y].n.z, 1.f};

            //if (rot || c->multilayer) {
                tvec4_mul_mat4(&p, rm);
                tvec4_mul_mat4(&n, rm);
            //}
            p.z *= zs;

            ///b2Vec2 lp = this->body->GetLocalPoint(c->e->local_to_world(c->p, 0));
            b2Vec2 lp = c->e->local_to_body(c->p, 0);
            //tms_infof("c->p:%f,%f, %p", c->p.x, c->p.y, &c->p);
            //b2Vec2 lp = c->e->_pos;

            p.x += lp.x;
            p.y += lp.y;
            p.z += c->layer * LAYER_DEPTH + (c->multilayer ? LAYER_DEPTH*.85f : 0) + z_offset;

            /*
            if (!rot && !is_splank)
                p.z += .75f;
                */

            int o = vc + y;

            v[o].p.x = p.x;
            v[o].p.y = p.y;
            v[o].p.z = p.z;

            v[o].n.x = n.x;
            v[o].n.y = n.y;
            v[o].n.z = n.z;
        }

        int o = 0;
        for (int y=0; y<num_i; y++) {
            o = vi + y;
            i[o] = vc + ci[y] - ibase;
        }

        vc += num_v;
        vi += num_i;
    }

    //tms_infof("num conns: %d", this->connections.size());

    tms_gbuffer_upload_partial(this->vbuf, vc*sizeof(struct vert));
    tms_gbuffer_upload_partial(this->ibuf, vi*sizeof(uint16_t));
}

void
group::add_to_world()
{
    /* do nothing */
}

void
group::remove_from_world()
{
    if (this->body) {
        W->b2->DestroyBody(this->body);
        this->body = 0;
    }
}

void
group::merge(group *g, connection *c)
{
    //tms_infof("MERGE %d, num_entities: %lu", this->id, this->entities.size());
    for (int x=0; x<g->connections.size(); x++)
        this->connections.push_back(g->connections[x]);

    for (int x=0; x<g->entities.size(); x++) {
        this->add_entity(g->entities[x]);
    }

    W->remove(g);

    if (g->scene) {
        //tms_scene_remove_entity(g->scene, g);
        G->remove_entity(g);
    }

    //this->create_mesh();

    delete g;
}

void
group::on_load(bool created, bool has_state)
{
    b2BodyDef bd;
    bd.position = this->_pos;
    bd.angle = this->_angle;
    //bd.type = b2_dynamicBody; /* TODO change to static if any connected entity is non-moveable*/
    bd.type = this->get_dynamic_type();

    /*
    if (bd.type == b2_staticBody) {tms_debugf("group body is static");}
    else if (bd.type == b2_dynamicBody) {tms_debugf("group body is dynamic");}
    else if (bd.type == b2_kinematicBody) {tms_debugf("group body is kinematic");}
    */

    this->body = W->b2->CreateBody(&bd);

    this->connections.clear();
    this->entities.clear();
}

void
group::push_connection(connection *c)
{
    this->connections.push_back(c);
}

void
group::dangle()
{
    if (this->body) {
        this->_pos = this->body->GetPosition();
        this->_angle = this->body->GetAngle();
    }
    this->body = 0;
}

void
group::push_entity(composable *e, b2Vec2 p, float angle)
{
    //tms_infof("push entity %p(%s), %f %f, %f", e, e->get_name(), p.x, p.y, angle);

    if (!this->body) {
        tms_infof("IMPLICITLY creating body");
        b2BodyDef bd;
        bd.position = _pos;//_pos;
        bd.angle = _angle;
        //bd.position = b2Vec2(0,0);
        //bd.angle = 0.f;
        bd.type = b2_dynamicBody;
        tms_debugf("creating group body at %f %f", _pos.x, _pos.y);
        this->body = W->b2->CreateBody(&bd);
    }

    e->update_shape(p, angle);

    e->_pos = p;
    e->_angle = angle;

    if (e->body) {
        W->b2->DestroyBody(e->body);
        e->fx = 0;
        e->fx_sensor = 0;
        tms_debugf("Destroying body %p. this body: %p", e->body, this->body);
    }

    e->body = 0;

    if (e->scene) {
        bool select = (G->selection.e == e);

        G->remove_entity(e);
        if (e->update_method != ENTITY_UPDATE_CUSTOM) {
            e->curr_update_method = e->update_method = ENTITY_UPDATE_GROUPED;
        }
        G->add_entity(e);

        if (select) {
            G->selection.select(e, 0, (tvec2){0,0}, 0, true);
        }
    } else {
        if (e->update_method != ENTITY_UPDATE_CUSTOM) {
            e->curr_update_method = e->update_method = ENTITY_UPDATE_GROUPED;
        }
    }

    /* TODO: resort */
    /* TODO: decipher above TODO */

    e->fd.filter = world::get_filter_for_layer(e->get_layer(), e->layer_mask);

    (e->fx = this->body->CreateFixture(&e->fd))->SetUserData(e);
    e->gr = this;
    e->create_sensor();

    /* XXX */
    if (W->is_paused()){
        /* apply extra density in editor, i dont remember why */
        e->fx->SetDensity(5.f);
    }

    this->entities.push_back(e);
}

void
group::recreate_all_entity_joints(bool hard)
{
    for (std::vector<composable*>::iterator i = this->entities.begin();
            i != this->entities.end(); i++)
        this->recreate_entity_joints(*i, hard);
}

/**
 * Recreate all of the given entity's joints
 **/
void
group::recreate_entity_joints(composable *e, bool hard)
{
    connection *cc = e->conn_ll;
    if (cc) {
        do {
            if (hard) cc->j = 0;
            cc->create_joint(false);
        } while ((cc = cc->next[(cc->e == e) ? 0:1]));

        e->update_frame(hard);
    }

    if (W->level.type == LCAT_ADVENTURE && !W->is_paused()) {
        edevice *ed = e->get_edevice();
        if (ed) {
            ed->recreate_all_cable_joints();
        }
    }
}

/**
 * Set the world center to the mass centre,
 * and save the offset between the real origo
 * and the mass centre
 **/
void
group::reset_origo(bool hard_recreate)
{
    this->body->ResetMassData();

    b2Vec2 mp = this->body->GetLocalCenter();
    tms_infof("mass centre is: %f %f", mp.x, mp.y);

    mp = -.5f*mp;

    for (std::vector<composable*>::iterator i = this->entities.begin();
            i != this->entities.end(); i++) {
        composable *e = *i;

        b2Vec2 p = e->_pos;
        p += mp;
        e->update_shape(p, e->_angle);
        e->_pos = p;
        e->fd.filter = world::get_filter_for_layer(e->get_layer(), e->layer_mask);
        this->body->DestroyFixture(e->fx);
        (e->fx = this->body->CreateFixture(&e->fd))->SetUserData(e);

        if (e->fx_sensor) {
            this->body->DestroyFixture(e->fx_sensor);
            e->fx_sensor = 0;
        }

        e->create_sensor();
    }

    this->set_position(this->get_position()+this->body->GetWorldVector(-mp));

    this->body->ResetMassData();
    mp = this->body->GetLocalCenter();
    tms_infof("after move: %f %f", mp.x, mp.y);

#if 0
    float min_x = 0;
    float max_x = 0;
    float min_y = 0;
    float max_y = 0;

    b2Vec2 mask[4] = {
        b2Vec2(.5f, 0.5f),
        b2Vec2(.5f, -.5f),
        b2Vec2(-.5f, -.5f),
        b2Vec2(-.5f, .5f)
    };

    for (std::vector<composable*>::iterator i = this->entities.begin();
            i != this->entities.end(); i++) {
        composable *e = *i;

        for (int x=0; x<4; x++) {
            b2Vec2 lp = e->local_to_body(b2Vec2(e->width*mask[x].x, e->height*mask[x].y), 0);

            if (lp.x < min_x) min_x = lp.x;
            if (lp.x > max_x) max_x = lp.x;
            if (lp.y < min_y) min_y = lp.y;
            if (lp.y > max_y) max_y = lp.y;
        }
    }

    this->origo_offset = b2Vec2(
        (min_x+max_x)/2.f,// - mp.x,
        (min_y+max_y)/2.f// - mp.y
    );

    tms_infof("origo offset set to: %f %f", origo_offset.x, origo_offset.y);
#endif
    this->recreate_all_entity_joints(hard_recreate);
}

/**
 * Called in the editor when a group is expanded to contain
 * one more composable.
 **/
void
group::add_entity(composable *e)
{
    //tms_infof("adding entity: %p", e);
    if (this->entities.size() == 0) {
        //tms_infof("%p stealing body pointer %p", this, e->body);
        /* for the first body, we simply take steal the body pointer */

        this->body = e->body;
        e->body = 0;

        if (e->scene) {
            bool select = (G->selection.e == e);

            G->remove_entity(e);
            if (e->update_method != ENTITY_UPDATE_CUSTOM)
                e->curr_update_method = e->update_method = ENTITY_UPDATE_GROUPED;
            G->add_entity(e);

            if (select)
                G->selection.select(e, 0, (tvec2){0,0}, 0, true);
        } else {
            e->curr_update_method = e->update_method = ENTITY_UPDATE_GROUPED;
        }

        e->_pos = b2Vec2(0,0);
        e->_angle = 0;
        e->gr = this;
        this->entities.push_back(e);
    } else {
        b2Vec2 p = this->body->GetLocalPoint(e->get_position());

        /* force the body type to dynamic temporarily, otherwise joints will not be created,
         * and we need to loop through all joints to decide whether we must be static or not */
        this->body->SetType(b2_dynamicBody);

        this->push_entity(e, p, e->get_angle() - this->body->GetAngle());
        this->recreate_entity_joints(e, true);
        this->reset_origo(false);
        this->finalize();
    }
}

void
group::add(connection *c)
{
    if (c->e->gr != this) {
        if (c->e->gr != 0) {
            this->merge(c->e->gr, c);
        } else {
            this->add_entity((composable*)c->e);
        }
    }
    if (c->o->gr != this) {
        if (c->o->gr != 0) {
            this->merge(c->o->gr, c);
        } else {
            this->add_entity((composable*)c->o);
        }
    }

    this->connections.push_back(c);
    this->create_mesh();
}

void
group::make_group(composable *e, std::set<composable *> *pending, std::set<composable *> *found, std::set<connection *> *found_conn)
{
    pending->erase(e);
    found->insert(e);

    //tms_infof("make group");

    connection *c = e->conn_ll;
    if (c) {
        do {
            if (c->type == CONN_GROUP) {
                tms_infof("found conn %p", c);
                found_conn->insert(c);
                composable *other = (composable*)(c->e == e ? c->o : c->e);
                if (found->find(other) == found->end() && pending->find(other) != pending->end())
                    this->make_group(other, pending, found, found_conn);
            } else {
                tms_debugf("ignoreing connection %p, not group", c);
            }
        } while ((c = c->next[(c->e == e ? 0 : 1)]));
    }
}

void
group::rebuild()
{
    tms_infof("rebuilding group");
    this->connections.clear();

    b2Vec2 lvel = b2Vec2(0.f, 0.f);
    float avel = 0.f;

    std::set<composable*> pending;
    std::set<composable*> found;
    std::set<connection*> found_conn;

    std::set<group*> modified_groups;
    std::set<composable*> modified_entities;

    for (std::vector<composable*>::iterator i = this->entities.begin();
            i != this->entities.end(); i++) {
        pending.insert(*i);
        (*i)->_pos = (*i)->get_position();
        (*i)->_angle = (*i)->get_angle();
        (*i)->gr = 0;
    }

    //tms_infof("created pending");

    this->entities.clear();

    if (!W->is_paused() && W->level.type == LCAT_ADVENTURE && this->body) {
        lvel = this->body->GetLinearVelocity();
        avel = this->body->GetAngularVelocity();
    }

    //tms_infof("%p group body: %p", this, this->body);
    if (this->body) {
        //this->_pos = this->body->GetPosition();
        //this->_angle = this->body->GetAngle();
        W->b2->DestroyBody(this->body);
    }
    this->body = 0;
    //tms_infof("%p group body: %p", this, this->body);

    bool created_self = false;

    while (pending.size() > 0) {
        found.clear();
        found_conn.clear();
        this->make_group(*pending.begin(), &pending, &found, &found_conn);

        if (found.size() > 1) {
            group *target;
            if (!created_self) {
                target = this;
                created_self = true;
            } else {
                target = new group();
                target->id = of::get_next_id();
                G->add_entity(target);
                W->add(target);
            }

            int xx = 0;
            for (std::set<composable*>::iterator i = found.begin();
                    i != found.end(); i++) {
                if (xx == 0) {
                    target->_pos = (*i)->_pos;
                    target->_angle = (*i)->_angle;
                    target->push_entity(*i, b2Vec2(.0f,.0f), 0.f);
                } else {
                    b2Vec2 p = target->body->GetLocalPoint((*i)->_pos);
                    target->push_entity(*i, p, (*i)->_angle - target->_angle);
                }
                xx = 1;
            }
            std::copy(found_conn.begin(), found_conn.end(), std::back_inserter(target->connections));

            //target->create_mesh();
            modified_groups.insert(target);
        } else {
            composable *e = *found.begin();
            e->update_shape(b2Vec2(0,0), 0);

            tms_debugf("loose entity: %u", e->id);

            if (e->scene) {
                bool select = (G->selection.e == e);

                G->remove_entity(e);
                if (e->update_method == ENTITY_UPDATE_GROUPED) {
                    e->curr_update_method = e->update_method = ENTITY_UPDATE_FASTBODY;
                }
                G->add_entity(e);

                if (select)
                    G->selection.select(e, 0, (tvec2){0,0}, 0, true);
            } else {
                if (e->update_method == ENTITY_UPDATE_GROUPED) {
                    e->curr_update_method = e->update_method = ENTITY_UPDATE_FASTBODY;
                }

                /* why isnt it added to the scene?
                 * one reason would be because the entity's mesh
                 * was either wooden or plastic, so it was added to the big mesh.
                 *
                 * must we check for that case, or is it safe to always assume that
                 * was why the entity was not added to the scene? */
                G->add_entity(e);
            }

            W->add(e);

            if (e->get_body(0)) {
                e->get_body(0)->SetAngularVelocity(avel);
                e->get_body(0)->SetLinearVelocity(lvel);
            }

            modified_entities.insert(e);
        }
    }

    for (std::set<group*>::iterator i = modified_groups.begin();
            i != modified_groups.end(); i++) {
        (*i)->reset_origo(true);
        (*i)->create_mesh();
    }

    for (std::set<composable*>::iterator i = modified_entities.begin();
            i != modified_entities.end(); i++) {
        this->recreate_entity_joints(*i, true);
        (*i)->update_frame(true);
    }

    for (std::set<group*>::iterator i = modified_groups.begin();
            i != modified_groups.end(); i++) {
        (*i)->finalize();
        if ((*i)->body) {
            (*i)->body->SetAngularVelocity(avel);
            (*i)->body->SetLinearVelocity(lvel);
        }
    }

    if (!created_self) {
        /* nothing left in the group */
        tms_infof("WARNING: REMOVING SELF (%p)", this);
        if (this->scene) {
            G->remove_entity(this);
        }
        W->remove(this);
        delete this;
    }
}

static bool sort_by_id(composable *a, composable *b){return a->id < b->id;};

void
group::finalize()
{
    bool found_static = false;

    std::vector<connection *> to_be_removed;

    tms_debugf("Group %p finalize", this);

    if (this->body) {
        /* look through the list of joints, if any joint is
         * to a static body, we change the type of this body to static too */

        for (b2JointEdge *je = this->body->GetJointList();
                je != NULL; je = je->next) {
            b2Joint *j = je->joint;

            if (j->GetUserData() != 0) {
                joint_info *ji = static_cast<joint_info*>(j->GetUserData());

                if (ji->type == JOINT_TYPE_CONN) {
                    connection *c = static_cast<connection*>(ji->data);

                    if ((c->type == CONN_GROUP || c->type == CONN_WELD || c->type == CONN_PLATE) && c->max_force == INFINITY) {
                        if (j->GetBodyA() != this->body && j->GetBodyA()->GetType() == b2_staticBody) {
                            found_static = true;
                            to_be_removed.push_back(c);
                        } else if (j->GetBodyB() != this->body && j->GetBodyB()->GetType() == b2_staticBody) {
                            found_static = true;
                            to_be_removed.push_back(c);
                        }
                    }
                }
            }
        }

        if (found_static && !W->is_paused()) {
            tms_debugf("group connected to static body, setting to static");

            for (std::vector<connection *>::iterator i = to_be_removed.begin(); i != to_be_removed.end(); i++) {
                connection *c = (*i);
                if (c->j) {
                    this->body->GetWorld()->DestroyJoint(c->j);
                    c->j = 0;
                }
            }

            this->body->SetType(b2_staticBody);
        } else if (W->is_paused() && this->is_locked()) {
            this->body->SetType(b2_staticBody);

            for (std::vector<connection *>::iterator i = to_be_removed.begin(); i != to_be_removed.end(); i++) {
                connection *c = (*i);
                if (c->j) {
                    this->body->GetWorld()->DestroyJoint(c->j);
                    c->j = 0;
                }
            }
        } else {
            this->body->SetType(b2_dynamicBody);
        }
    }

    /* TODO: readd the entity to the scene */

    bool was_added = false;
    if (this->scene) {
        was_added = true;
        G->remove_entity(this);
    }

    if (!found_static) {
        this->curr_update_method = this->update_method = ENTITY_UPDATE_CUSTOM;
    } else {
        this->curr_update_method = this->update_method = ENTITY_UPDATE_STATIC_CUSTOM;
    }

    if (was_added)
        G->add_entity(this);

    int num_wood = 0, num_plastic = 0;
    size_t wood_vbuf_size = 0, wood_ibuf_size = 0;
    size_t plastic_vbuf_size = 0, plastic_ibuf_size = 0;

    /* sort the entities to make sure the mesh always looks the same (z order) */
    std::sort(this->entities.begin(), this->entities.end(), sort_by_id);

    for (std::vector<composable*>::iterator i = this->entities.begin();
            i != this->entities.end(); i++) {
        if ((*i)->material == &m_wood) {
            num_wood ++;

            struct tms_mesh *mm = (*i)->mesh;
            wood_vbuf_size += mm->v_count * sizeof(struct wooden_vert);
            wood_ibuf_size += mm->i_count * sizeof(uint16_t);

        } else if ((*i)->material == &m_plastic) {
            num_plastic ++;

            struct tms_mesh *mm = (*i)->mesh;
            plastic_vbuf_size += mm->v_count * sizeof(struct plastic_vert);
            plastic_ibuf_size += mm->i_count * sizeof(uint16_t);
        }
    }

    for (int x=0; x<3; x++) {
        this->wooden_count[x] = 0;
        this->plastic_count[x] = 0;
        this->wooden_mesh[x].v_count = 0;
        this->wooden_mesh[x].i_count = 0;
        this->wooden_mesh[x].i_start = 0;
        this->plastic_mesh[x].v_count = 0;
        this->plastic_mesh[x].i_count = 0;
        this->plastic_mesh[x].i_start = 0;
    }

    if (num_wood > 0) {
        tms_gbuffer_realloc(&this->wooden_vbuf, wood_vbuf_size);
        tms_gbuffer_realloc(&this->wooden_ibuf, wood_ibuf_size);
    }
    if (num_plastic > 0) {
        tms_gbuffer_realloc(&this->plastic_vbuf, plastic_vbuf_size);
        tms_gbuffer_realloc(&this->plastic_ibuf, plastic_ibuf_size);
    }

    size_t v_accum_wood = 0;
    size_t v_accum_plastic = 0;

    for (int layer=0; layer<3; layer++) {
        if (layer > 0) {
            wooden_mesh[layer].i_start = wooden_mesh[layer-1].i_start + wooden_mesh[layer-1].i_count;
            plastic_mesh[layer].i_start = plastic_mesh[layer-1].i_start + plastic_mesh[layer-1].i_count;
        }

        for (std::vector<composable*>::iterator i = this->entities.begin();
                i != this->entities.end(); i++) {
            if (((*i)->material == &m_wood || (*i)->material == &m_plastic) && (*i)->get_layer() == layer) {
                bool had_scene = false;
                if ((*i)->scene) {
                    G->remove_entity((*i));
                    had_scene = true;
                }

                bool is_wood = ((*i)->material == &m_wood);

                entity *e = (*i);

                size_t v_accum = is_wood ? v_accum_wood : v_accum_plastic;

                struct tms_gbuffer *ibuf = is_wood ? &wooden_ibuf : &plastic_ibuf;
                struct tms_gbuffer *vbuf = is_wood ? &wooden_vbuf : &plastic_vbuf;
                struct tms_mesh *lm = is_wood ? &wooden_mesh[layer] : &plastic_mesh[layer];

                struct tms_mesh *mm = (*i)->mesh;

                uint16_t *i = (uint16_t*)((char*)tms_gbuffer_get_buffer(ibuf) + lm->i_count*sizeof(uint16_t) + lm->i_start*sizeof(uint16_t));
                uint16_t *ri = (uint16_t*)((char*)tms_gbuffer_get_buffer(mm->indices)+mm->i_start*sizeof(uint16_t));

                struct tms_gbuffer *r_vbuf = mm->vertex_array->gbufs[0].gbuf;
                struct cvert *rv = (struct cvert*)((char*)tms_gbuffer_get_buffer(r_vbuf)+mm->v_start);
                int num_rv = mm->v_count;

                uint16_t ibase = mm->v_start / sizeof(struct cvert);

                b2Vec2 p = e->_pos;
                float a = e->_angle;
                float cs,sn;
                tmath_sincos(a, &sn, &cs);

                for (int y=0; y<mm->i_count; y++) {
                    i[y] = ri[y] - ibase + (uint16_t)lm->v_count + v_accum;
                }

                if (is_wood) {
                    struct wooden_vert *v = (struct wooden_vert *)((char*)tms_gbuffer_get_buffer(&wooden_vbuf) + (wooden_mesh[layer].v_count+v_accum)*sizeof(struct wooden_vert));

                    for (int x=0; x<num_rv; x++) {
                        v[x].position = (tvec3){
                            rv[x].p.x * cs - rv[x].p.y * sn + p.x,
                            rv[x].p.x * sn + rv[x].p.y * cs + p.y,
                            rv[x].p.z
                        };
                        v[x].normal = (tvec3){
                            rv[x].n.x * cs - rv[x].n.y * sn,
                            rv[x].n.x * sn + rv[x].n.y * cs,
                            rv[x].n.z
                        };
                        v[x].texcoord = (tvec2){
                            rv[x].u.x, rv[x].u.y
                        };
                    }
                } else {
                    struct plastic_vert *v = (struct plastic_vert *)((char*)tms_gbuffer_get_buffer(&plastic_vbuf) + (lm->v_count+v_accum)*sizeof(struct plastic_vert));

                    for (int x=0; x<num_rv; x++) {
                        v[x].position = (tvec3){
                            rv[x].p.x * cs - rv[x].p.y * sn + p.x,
                            rv[x].p.x * sn + rv[x].p.y * cs + p.y,
                            rv[x].p.z
                        };
                        v[x].normal = (tvec3){
                            rv[x].n.x * cs - rv[x].n.y * sn,
                            rv[x].n.x * sn + rv[x].n.y * cs,
                            rv[x].n.z
                        };

                        if (_tms.gamma_correct) {
                            v[x].color = (tvec3){
                                powf(e->properties[1].v.f, _tms.gamma),
                                powf(e->properties[2].v.f, _tms.gamma),
                                powf(e->properties[3].v.f, _tms.gamma),
                            };
                        } else {
                            v[x].color = (tvec3){
                                e->properties[1].v.f,
                                e->properties[2].v.f,
                                e->properties[3].v.f,
                            };
                        }
                    }
                }

                lm->v_count += num_rv;
                lm->i_count += mm->i_count;

                /*
                (*i)->set_mesh((tms_mesh*)0);
                (*i)->update_method = ENTITY_UPDATE_NONE;

                if (had_scene)
                    G->add_entity(e);
                    */
            }
        }

        if (wooden_mesh[layer].i_count) {
            if (!this->wooden_entity[layer].scene) {
                tms_entity_add_child(this, &this->wooden_entity[layer]);
            }
        } else {
            if (this->wooden_entity[layer].parent) {
                if (this->wooden_entity[layer].scene) tms_scene_remove_entity(G->get_scene(), &this->wooden_entity[layer]);
                tms_entity_remove_child(this, &this->wooden_entity[layer]);
            }
        }

        if (plastic_mesh[layer].i_count) {
            if (!this->plastic_entity[layer].scene) {
                tms_entity_add_child(this, &this->plastic_entity[layer]);
            }
        } else {
            if (this->plastic_entity[layer].parent) {
                if (this->plastic_entity[layer].scene) tms_scene_remove_entity(G->get_scene(), &this->plastic_entity[layer]);
                tms_entity_remove_child(this, &this->plastic_entity[layer]);
            }
        }

        v_accum_wood += wooden_mesh[layer].v_count;
        v_accum_plastic += plastic_mesh[layer].v_count;
    }

    if (num_wood > 0) {
        tms_gbuffer_upload(&this->wooden_ibuf);
        tms_gbuffer_upload(&this->wooden_vbuf);
    }

    if (num_plastic > 0) {
        tms_gbuffer_upload(&this->plastic_ibuf);
        tms_gbuffer_upload(&this->plastic_vbuf);
    }
}

bool
group::is_locked()
{
    for (std::vector<composable*>::iterator it = this->entities.begin();
            it != this->entities.end(); ++it) {
        entity *e = *it;

        if (e->flag_active(ENTITY_IS_LOCKED)) {
            return true;
        }
    }

    return false;
}
