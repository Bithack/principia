#include "ladder.hh"
#include "material.hh"
#include "model.hh"
#include "game.hh"
#include "world.hh"

ladder::ladder()
    : ladder_ud2(UD2_CLIMBABLE)
    , ladder_step_ud2(UD2_LADDER_STEP)
{
    this->set_flag(ENTITY_ALLOW_CONNECTIONS, false);
    this->set_flag(ENTITY_DYNAMIC_UNLOADING, true);

    this->width = .25f;
    this->height = 1.5f;
    this->set_mesh(mesh_factory::get_mesh(MODEL_LADDER));
    this->set_material(&m_ladder);

    this->layer_mask = 1;

    this->c_back.init_owned(0, this);
    this->c_back.type = CONN_WELD;
    this->c_back.typeselect = false;

    this->c_vert[0].init_owned(1, this);
    this->c_vert[0].type = CONN_WELD;
    this->c_vert[0].typeselect = false;

    this->c_vert[1].init_owned(2, this);
    this->c_vert[1].type = CONN_WELD;
    this->c_vert[1].typeselect = false;

    this->c_hori[0].init_owned(3, this);
    this->c_hori[0].type = CONN_WELD;
    this->c_hori[0].typeselect = false;

    this->c_hori[1].init_owned(4, this);
    this->c_hori[1].type = CONN_WELD;
    this->c_hori[1].typeselect = false;
}

void
ladder::add_to_world()
{
    this->create_rect(this->get_dynamic_type(), .35f, 1.45f, this->material);
    this->fx->SetUserData2(&this->ladder_ud2);

    b2PolygonShape box;
    box.SetAsBox(.35f, 0.1f, b2Vec2(0.f,  0.75f), 0);

    /* Create the ladder "step" */
    b2FixtureDef fd;
    fd.isSensor = false;
    fd.restitution = 0.1f;
    fd.friction = 0.5f;
    fd.density = 2.0f;
    fd.filter = world::get_filter_for_layer(this->get_layer(), 1);
    fd.shape = &box;

    this->ladder_step = this->body->CreateFixture(&fd);
    this->ladder_step->SetUserData(this);
    this->ladder_step->SetUserData2(&this->ladder_step_ud2);
}

connection *
ladder::load_connection(connection &conn)
{
    if (conn.o_index == 0) {
        this->c_back = conn;
        return &this->c_back;
    } else if (conn.o_index < 3) {
        this->c_vert[conn.o_index-1] = conn;
        return &this->c_vert[conn.o_index-1];
    } else if (conn.o_index < 5) {
        this->c_hori[conn.o_index-3] = conn;
        return &this->c_hori[conn.o_index-3];
    }

    return 0;
}

bool
ladder::ReportFixture(b2Fixture *f)
{
    if (f->IsSensor()) {
        return true;
    }

    entity *e = (entity*)f->GetUserData();
    uint8_t fr = VOID_TO_UINT8(f->GetBody()->GetUserData());

    if (e && e!=this && f->TestPoint(this->q_point)
        && e->allow_connections() && e->allow_connection(this,fr,this->q_point)) {
        int dist = e->get_layer() - this->get_layer();
        if (dist == -1) {

            q_result = e;
            q_frame = fr;
            q_result_fx = f;
            return false;
        }
    }

    return true;
}

void
ladder::find_pairs()
{
    connection *c;

    if (this->c_back.pending) {
        b2Vec2 p = this->get_position();
        b2AABB aabb;
        aabb.lowerBound.Set(p.x - .05f, p.y - .05f);
        aabb.upperBound.Set(p.x + .05f, p.y + .05f);

        this->q_result = 0;
        this->q_point = p;

        W->b2->QueryAABB(this, aabb);

        if (q_result != 0) {
            c = &this->c_back;

            if (c->pending) {
                c->o_data = q_result->get_fixture_connection_data(q_result_fx);

                c->o = q_result;
                c->p = p;
                c->f[1] = q_frame;
                G->add_pair(this, q_result, c);
            }
        }
    }

#define QUERY_LEN .2f

    static b2Vec2 ray_p_vert[2] = {
        b2Vec2(-this->width, 0.f),
        b2Vec2( this->width, 0.f),
    };

    b2Vec2 ray_v_vert[2] = {
        b2Vec2(-(this->width/2.f+0.15f), 0.f),
        b2Vec2( (this->width/2.f+0.15f), 0.f),
    };

    static b2Vec2 ray_p_hori[2] = {
        b2Vec2(0.f,  this->height),
        b2Vec2(0.f, -this->height),
    };

    b2Vec2 ray_v_hori[2] = {
        b2Vec2(0.f,  (this->height/2.f+0.15f)),
        b2Vec2(0.f, -(this->height/2.f+0.15f)),
    };

    for (int x=0; x<2; ++x) {
        c = &this->c_vert[x];

        if (ray_v_vert[x].Length() > 0.01f && c->pending) {
            ray_v_vert[x] *= 1.f/ray_v_vert[x].Length() * QUERY_LEN;
            this->q_result = 0;
            this->q_ask_for_permission = true;

            W->raycast(this,
                    this->local_to_world(ray_p_vert[x], 0),
                    this->local_to_world(ray_p_vert[x]+ray_v_vert[x], 0));

            if (this->q_result) {
                c->o = this->q_result;
                c->angle = atan2f(ray_v_vert[x].y, ray_v_vert[x].x);
                c->render_type = CONN_RENDER_SMALL;
                c->f[0] = 0;
                c->f[1] = this->q_frame;
                c->p = ray_v_vert[x];
                c->p *= this->q_fraction * .5;
                c->p += ray_p_vert[x];
                c->p = this->local_to_world(c->p, 0);
                c->o_data = this->q_result->get_fixture_connection_data(this->q_result_fx);
                G->add_pair(this, this->q_result, c);
            }
        }
    }

    for (int x=0; x<2; ++x) {
        c = &this->c_hori[x];

        if (ray_v_hori[x].Length() > 0.01f && c->pending) {
            ray_v_hori[x] *= 1.f/ray_v_hori[x].Length() * QUERY_LEN;
            this->q_result = 0;
            this->q_ask_for_permission = false;

            W->raycast(this,
                    this->local_to_world(ray_p_hori[x], 0),
                    this->local_to_world(ray_p_hori[x]+ray_v_hori[x], 0));

            if (this->q_result) {
                c->o = this->q_result;
                c->angle = atan2f(ray_v_hori[x].y, ray_v_hori[x].x);
                c->render_type = CONN_RENDER_SMALL;
                c->f[0] = 0;
                c->f[1] = this->q_frame;
                c->p = ray_v_hori[x];
                c->p *= this->q_fraction * .5;
                c->p += ray_p_hori[x];
                c->p = this->local_to_world(c->p, 0);
                c->o_data = this->q_result->get_fixture_connection_data(this->q_result_fx);
                G->add_pair(this, this->q_result, c);
            }
        }
    }

#undef QUERY_LEN
}

float32
ladder::ReportFixture(b2Fixture *f, const b2Vec2 &pt, const b2Vec2 &nor, float32 fraction)
{
    if (f->IsSensor()) {
        return -1.f;
    }

    entity *e = static_cast<entity*>(f->GetUserData());
    b2Body *b = f->GetBody();

    if (!e) {
        return -1.f;
    }

    if (e->get_layer() != this->get_layer()) {
        return -1.f;
    }

    if (this->q_ask_for_permission && !e->allow_connections()) {
        return -1.f;
    }

    if (!this->q_ask_for_permission && e->g_id != O_LADDER) {
        return -1.f;
    }

    this->q_result = e;
    this->q_result_fx = f;
    this->q_frame = VOID_TO_UINT8(b->GetUserData());
    this->q_fraction = fraction;

    return fraction;
}

bool
ladder::enjoys_connection(uint32_t g_id)
{
    return (g_id == O_LADDER);
}

ladder_step::ladder_step()
    : ladder_step_ud2(UD2_LADDER_STEP)
{
    this->has_pair = false;
    this->set_flag(ENTITY_ALLOW_CONNECTIONS, false);
    this->set_flag(ENTITY_DYNAMIC_UNLOADING, true);
    this->set_flag(ENTITY_IS_ZAPPABLE,       true);

    this->width = .25f;
    this->height = .125f;
    this->set_mesh(mesh_factory::get_mesh(MODEL_LADDER_STEP));
    this->set_material(&m_ladder);

    this->layer_mask = 1;

    this->worth.add(RESOURCE_WOOD, 2);

    this->c_back.init_owned(0, this);
    this->c_back.type = CONN_WELD;
    this->c_back.typeselect = false;
}

void
ladder_step::add_to_world()
{
    this->create_rect(this->get_dynamic_type(), .35f, .1f, this->material);
    this->fx->SetUserData2(&this->ladder_step_ud2);
}

connection *
ladder_step::load_connection(connection &conn)
{
    if (conn.o_index == 0) {
        this->c_back = conn;
        return &this->c_back;
    }

    return 0;
}

bool
ladder_step::ReportFixture(b2Fixture *f)
{
    if (f->IsSensor()) {
        return true;
    }

    entity *e = (entity*)f->GetUserData();
    uint8_t fr = VOID_TO_UINT8(f->GetBody()->GetUserData());

    if (e && e->g_id == O_CHUNK && world::fixture_in_layer(f, 0)) {
        q_result = e;
        q_frame = fr;
        q_result_fx = f;
        return false;
    }

    return true;
}

void
ladder_step::find_pairs()
{
    connection *c;

    this->has_pair = false;

    if (this->c_back.pending && this->get_layer() == 1) {
        b2Vec2 p = this->get_position();
        b2AABB aabb;
        aabb.lowerBound.Set(p.x - .05f, p.y - .05f);
        aabb.upperBound.Set(p.x + .05f, p.y + .05f);

        this->q_result = 0;
        this->q_point = p;

        W->b2->QueryAABB(this, aabb);

        if (q_result != 0) {
            c = &this->c_back;

            if (c->pending) {
                c->o_data = q_result->get_fixture_connection_data(q_result_fx);

                c->o = q_result;
                c->p = p;
                c->f[1] = q_frame;
                this->has_pair = true;
                G->add_pair(this, q_result, c);
            }
        }
    }
}
