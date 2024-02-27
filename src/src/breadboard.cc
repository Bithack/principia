#include "breadboard.hh"
#include "model.hh"
#include "material.hh"
#include "game.hh"
#include "edevice.hh"

breadboard::breadboard()
{
    this->set_flag(ENTITY_IS_LOW_PRIO, true);
    this->set_mesh(mesh_factory::get_mesh(MODEL_BREADBOARD));
    this->set_material(&m_breadboard);

    this->curr_update_method = this->update_method = ENTITY_UPDATE_CUSTOM;

    this->type = ENTITY_BREADBOARD;

    this->layer_mask = 1;
    this->num_sliders = 2;

    this->menu_scale = 1.f/4.f;
    this->set_uniform("size", 2.f, 2.f, 0.f, 0.f);

    this->set_num_properties(2);
    this->properties[0].type = P_FLT;
    this->properties[1].type = P_FLT;
    this->properties[0].v.f = 2.f;
    this->properties[1].v.f = 2.f;

    tmat4_load_identity(this->M);
    tmat3_load_identity(this->N);

    this->set_uniform("ao_mask2", 1.f, 0.f, 0.f, 0.f);

    for (int x=0; x<4; x++) {
        this->c[x].init_owned(x, this);
        this->c[x].type = CONN_PLATE;
    }

    update_size();
}

void breadboard::update()
{
    b2Vec2 p;
    float a;
    if (this->get_body(0)) {
        p = this->get_position();
        a = this->get_angle();
    } else {
        p = this->_pos;
        a = this->_angle;
    }

    tmat4_load_identity(this->M);
    tmat4_translate(this->M, p.x, p.y, this->get_layer()*LAYER_DEPTH);
    tmat4_rotate(this->M, a * (180.f/M_PI), 0, 0, -1);
    tmat3_copy_mat4_sub3x3(this->N, this->M);
    tmat4_scale(this->M, 1.f+this->properties[0].v.f, 1.f+this->properties[1].v.f, 1.f);
}

void
breadboard::set_layer(int z)
{
    switch (z) {
        case 0: this->set_uniform("ao_mask2", 1.f, 0.f, 0.f, 0.f); break;
        case 1: this->set_uniform("ao_mask2", 0.f, 1.f, 0.f, 0.f); break;
        case 2: this->set_uniform("ao_mask2", 0.f, 0.f, 1.f, 0.f); break;
    }
    entity::set_layer(z);
}

void
breadboard::on_slider_change(int s, float value)
{
    this->properties[s].v.f=value*BR_MAX_SIZE;
    this->disconnect_all();
    this->update_size();
}

void
breadboard::on_load(bool created, bool has_state)
{
    this->update_size();
}

void
breadboard::update_size(void)
{
    this->set_uniform("size", 1.f+this->properties[0].v.f, 1.f+this->properties[1].v.f, 0.f, 0.f);
    this->set_as_rect(.5f+this->properties[0].v.f/2.f, .5f+this->properties[1].v.f/2.f);
    this->recreate_shape();
}

connection *
breadboard::load_connection(connection &conn)
{
    this->c[conn.o_index] = conn;
    return &this->c[conn.o_index];
}

float32
breadboard::ReportFixture(b2Fixture *f, const b2Vec2 &pt, const b2Vec2 &nor, float32 fraction)
{
    if (f->IsSensor()) {
        return -1.f;
    }

    b2Body *b = f->GetBody();
    entity *e = (entity*)f->GetUserData();

    if (e && e->type == ENTITY_PLANK && e->get_layer() == this->get_layer()) {
        this->q_result = e;
        this->q_frame = (uint8_t)(uintptr_t)b->GetUserData();
        this->q_fraction = fraction;
    }

    return -1;
}

void
breadboard::find_pairs()
{
#if 0
    b2Vec2 ray_p[4] = {
        b2Vec2(0.f, .5f + 1.f * this->properties[1].v.f/2.f),
        b2Vec2(-.5f -1.f * this->properties[0].v.f/2.f, 0.f),
        b2Vec2(0.f, -.5f - 1.f * this->properties[1].v.f/2.f),
        b2Vec2(.5f + 1.f * this->properties[0].v.f/2.f, 0.f),
    };
    static b2Vec2 ray_v[4] = {
        b2Vec2(0.f, .5f),
        b2Vec2(-.5f, 0.f),
        b2Vec2(0.f, -.5f),
        b2Vec2(.5f, 0.f),
    };

    for (int x=0; x<4; x++) {
        if (this->c[x].pending) {
            this->q_result = 0;

            W->b2->RayCast(this,
                    this->local_to_world(ray_p[x], 0),
                    this->local_to_world(ray_p[x]+ray_v[x], 0));

            if (this->q_result) {
                this->c[x].o = this->q_result;
                this->c[x].f[0] = 0;
                this->c[x].f[1] = this->q_frame;
                this->c[x].p = ray_v[x];
                this->c[x].p *= this->q_fraction;
                this->c[x].p += ray_p[x];
                this->c[x].p = this->local_to_world(this->c[x].p, 0);
                g->add_pair(this, this->q_result, &this->c[x]);
            }
        }
    }
#endif

    b2Vec2 p = this->get_position();
    b2AABB aabb;
    float ww = .5f+this->properties[0].v.f/2.f, hh = .5f+this->properties[1].v.f/2.f;
    aabb.lowerBound.Set(p.x - ww, p.y - hh);
    aabb.upperBound.Set(p.x + ww, p.y + hh);
    W->b2->QueryAABB(this, aabb);
}

bool
breadboard::ReportFixture(b2Fixture *f)
{
    entity *e = static_cast<entity*>(f->GetUserData());

    if (e) {
        if (e->flag_active(ENTITY_IS_BRDEVICE)) {
            if (this->fx->TestPoint(e->get_position())) {
                connection *c = G->get_tmp_conn();
                c->type = CONN_GROUP;
                c->e = this;
                c->o = e;
                c->pending = true;
                c->o_data = e->get_fixture_connection_data(f);
                c->p = e->get_position();
                if (!G->add_pair(this, e, c))
                    G->return_tmp_conn(c);
            }
        } else if (e->type == ENTITY_PLANK) {
            b2Vec2 corners[4] = {
                this->local_to_world(b2Vec2(.5f+this->properties[0].v.f / 2.f - .125f, .5f+this->properties[1].v.f/2.f - .125f), 0),
                this->local_to_world(b2Vec2(-.5f-this->properties[0].v.f / 2.f + .125f, .5f+this->properties[1].v.f/2.f - .125f), 0),
                this->local_to_world(b2Vec2(-.5f-this->properties[0].v.f / 2.f + .125f, -.5f-this->properties[1].v.f/2.f + .125f), 0),
                this->local_to_world(b2Vec2(.5f+this->properties[0].v.f / 2.f - .125f, -.5f-this->properties[1].v.f/2.f + .125f), 0)
            };

            for (int x=0; x<4; x++) {
                if (f->TestPoint(corners[x])) {
                    connection *c = G->get_tmp_conn();
                    c->type = CONN_GROUP;
                    c->e = this;
                    c->o = e;
                    c->pending = true;
                    c->p = corners[x];
                    c->o_data = e->get_fixture_connection_data(f);
                    if (!G->add_pair(this, e, c))
                        G->return_tmp_conn(c);
                }
            }
        }
    }

    return true;
}

