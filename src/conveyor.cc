#include "conveyor.hh"
#include "material.hh"
#include "world.hh"
#include "model.hh"
#include "game.hh"
#include "gui.hh"

#define MAX_SPEED 20.f

struct tms_sprite*
conveyor::get_axis_rot_sprite(){
    return gui_spritesheet::get_sprite(S_CONVEYOR_AXISROT);
}
class conveyor_query_cb : public b2QueryCallback
{
  public:
    entity *result;
    conveyor *ignore;
    uint8_t result_frame;
    int result_dir;
    b2Fixture *result_fx;
    b2Vec2 point;
    uint8_t frame;

    conveyor_query_cb(conveyor *ignore, b2Vec2 point, uint8_t frame)
    {
        this->result = 0;
        this->ignore = ignore;
        this->point = point;
        this->frame = frame;
    }

    bool ReportFixture(b2Fixture *f)
    {
        entity *e = (entity*)f->GetUserData();
        uint8_t fr = (uint8_t)(uintptr_t)f->GetBody()->GetUserData();

        int open = ignore->c_back[this->frame].pending | (ignore->c_front[this->frame].pending <<1);

        if (!f->IsSensor() && e && e!=ignore && f->TestPoint(this->point)
            && e->allow_connections() && e->allow_connection(ignore, fr, this->point)) {

            int dist = e->get_layer() - ignore->get_layer();
            if (abs(dist) == 1 || (e->layer_mask & ignore->layer_mask) == 0) {

                if (dist < 0) dist = 0;
                dist++;

                if (open & dist) {
                    result = e;
                    result_frame = fr;
                    result_dir = dist;
                    result_fx = f;
                    return false;
                }
            }

        }

        return true;
    }
};

conveyor::conveyor()
{
    this->speed_mul = 1.f;
    this->invert = false;

    this->set_flag(ENTITY_IS_STATIC,            true);
    this->set_flag(ENTITY_DO_TICK,              true);
    this->set_flag(ENTITY_DYNAMIC_UNLOADING,    true);

    this->width = 4.f;
    this->menu_scale = .25f;
    this->num_sliders = 2;
    this->do_recreate_shape = false;

    this->update_method = ENTITY_UPDATE_STATIC;

    this->set_mesh(mesh_factory::get_mesh(MODEL_CONVEYOR2));
    this->set_material(&m_conveyor);
    this->set_uniform("~color", .6f, .6f, .6f, 1.f);

    tmat4_load_identity(this->M);
    tmat3_load_identity(this->N);

    this->num_s_in = 2;
    this->num_s_out = 0;

    this->s_in[0].lpos = b2Vec2(-.15f, .0f); /* speed multiplier */
    this->s_in[1].lpos = b2Vec2( .15f, .0f);  /* invert direction */

    this->set_num_properties(3);
    this->properties[0].type = P_INT;
    this->properties[1].type = P_FLT;
    this->properties[2].type = P_INT8;
    this->set_property(0, (uint32_t)5); /* conveyor length */
    this->set_property(1, 5.f); /* conveyor speed */
    this->properties[2].v.i8 = CONVEYOR_STATIC;
    if (W->level.version >= LEVEL_VERSION_1_3_0_3) {
        this->set_flag(ENTITY_ALLOW_AXIS_ROT, true);
    }

    for (int n=0; n<2; ++n) {
        this->c_back[n].init_owned(2*n, this);
        this->c_back[n].type = CONN_GROUP;
        this->c_back[n].typeselect = 1; /* allow the player to select between weld and pivot joints */

        this->c_front[n].init_owned(1+2*n, this);
        this->c_front[n].type = CONN_GROUP;
        this->c_front[n].typeselect = 1;
    }
}

void
conveyor::toggle_axis_rot()
{
    this->properties[2].v.i8 = (this->properties[2].v.i8+1) % 2;
    this->set_flag(ENTITY_AXIS_ROT, !this->flag_active(ENTITY_AXIS_ROT));
    this->do_recreate_shape = true;
}

void
conveyor::tick()
{
    if (this->do_recreate_shape) {
        this->recreate_shape();
        this->do_recreate_shape = false;
    }
}

void
conveyor::setup()
{
    this->invert = false;
    this->speed_mul = 1.f;
}

void
conveyor::recreate_shape()
{
    if (this->properties[0].v.i > 5) this->properties[0].v.i = 5;

    this->set_mesh(mesh_factory::get_mesh(MODEL_CONVEYOR0+this->properties[0].v.i));

    if (!this->body) {
        b2BodyDef bd;
        if (this->scene) {
            G->remove_entity(this);
        }
        if (this->properties[2].v.i8 == CONVEYOR_STATIC) {
            this->set_flag(ENTITY_IS_STATIC, true);
            this->curr_update_method = this->update_method = ENTITY_UPDATE_STATIC;
            bd.type = b2_staticBody;
        } else {
            this->set_flag(ENTITY_IS_STATIC, false);
            this->curr_update_method = this->update_method = ENTITY_UPDATE_FASTBODY;
            bd.type = b2_dynamicBody;
        }
        if (!this->scene) {
            G->add_entity(this);
        }
        bd.position = _pos;
        bd.angle = _angle;

        b2Body *b = W->b2->CreateBody(&bd);
        this->body = b;
    } else {
        while (this->body->GetFixtureList()) {
            this->body->DestroyFixture(this->body->GetFixtureList());
        }
    }

    this->width = .5f+this->properties[0].v.i*.5f;

    b2PolygonShape bd_shape;
    bd_shape.SetAsBox(this->width, .25f);

    b2FixtureDef bd_fd;
    bd_fd.shape = &bd_shape;
    bd_fd.density = .5f;
    bd_fd.friction = .6f;
    bd_fd.restitution = .1f;
    bd_fd.filter = world::get_filter_for_layer(this->get_layer(), 15);

    (this->body->CreateFixture(&bd_fd))->SetUserData(this);

    b2CircleShape dampera_shape;
    dampera_shape.m_radius = .25f;
    dampera_shape.m_p.Set(-this->width, 0.f);

    b2CircleShape damperb_shape;
    damperb_shape.m_radius = .25f;
    damperb_shape.m_p.Set(this->width, 0.f);

    b2FixtureDef damper_fd;
    damper_fd.shape = &dampera_shape;
    damper_fd.density = .5f;
    damper_fd.friction = .6f;
    damper_fd.restitution = .1f;
    damper_fd.filter = world::get_filter_for_layer(this->get_layer(), 15);
    (this->body->CreateFixture(&damper_fd))->SetUserData(this);
    damper_fd.shape = &damperb_shape;
    (this->body->CreateFixture(&damper_fd))->SetUserData(this);
    this->body->SetSleepingAllowed(false);
}

float
conveyor::get_tangent_speed()
{
    float speed = this->properties[1].v.f * this->speed_mul;
    return (this->invert ? -speed : speed);
}

void
conveyor::add_to_world()
{
    this->recreate_shape();
}

float
conveyor::get_slider_snap(int s)
{
    if (s == 0) return 1.f / 5.f;
    else return 1.f / 40.f;
}

float
conveyor::get_slider_value(int s)
{
    if (s == 0) return ((float)this->properties[0].v.i) / 5.f;
    else
        return (this->properties[1].v.f/MAX_SPEED + 1.f) / 2.f;
}

void
conveyor::on_slider_change(int s, float value)
{
    if (s == 0) {
        uint32_t size = (uint32_t)roundf((value * 5.f));

        G->animate_disconnect(this);
        this->disconnect_all();
        this->set_property(0, size);
        this->recreate_shape();
    } else {
        this->properties[1].v.f = (value - .5f) * 2.f * MAX_SPEED;
        G->show_numfeed(this->get_tangent_speed());
    }
}

edevice*
conveyor::solve_electronics()
{
    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();
    if (!this->s_in[1].is_ready())
        return this->s_in[1].get_connected_edevice();

    if (this->s_in[0].p) {
        this->speed_mul = this->s_in[0].get_value();
    } else {
        if (W->level.version >= 25 && W->level.type == LCAT_ADVENTURE)
            this->speed_mul = 0.f;
        else
            this->speed_mul = 1.f;
    }

    this->invert = (bool)((int)roundf(this->s_in[1].get_value()));

    return 0;
}

void
conveyor::find_pairs()
{
    if (this->properties[2].v.i8 == CONVEYOR_STATIC) return;

    b2Vec2 _p[2] = {
        this->local_to_world(b2Vec2(-this->width, 0.f), 0),
        this->local_to_world(b2Vec2( this->width, 0.f), 0),
    };

    for (int n=0; n<2; ++n) {
        if (this->c_back[n].pending || this->c_front[n].pending/* && this->body*/) {
            b2Vec2 p = _p[n];
            b2AABB aabb;
            conveyor_query_cb cb(this, p, n);
            aabb.lowerBound.Set(p.x - .05f, p.y - .05f);
            aabb.upperBound.Set(p.x + .05f, p.y + .05f);
            W->b2->QueryAABB(&cb, aabb);

            if (cb.result != 0) {
                connection *c;
                if (cb.result_dir == 1)
                    c = &this->c_back[n];
                else
                    c = &this->c_front[n];

                if (c->pending) {
                    c->type = CONN_GROUP;
                    c->typeselect = this->find_pivot(0,false) == 0 && this->find_pivot(1, false) == 0;
                    c->o = cb.result;
                    c->o_data = c->o->get_fixture_connection_data(cb.result_fx);
                    c->p = p;
                    c->f[1] = cb.result_frame;
                    G->add_pair(this, cb.result, c);
                }
            }
        }
    }
}

connection*
conveyor::load_connection(connection &conn)
{
    if (conn.o_index == 0) {
        this->c_back[0] = conn;
        return &this->c_back[0];
    } else if (conn.o_index == 1) {
        this->c_front[0] = conn;
        return &this->c_front[0];
    } else if (conn.o_index == 2) {
        this->c_back[1] = conn;
        return &this->c_back[1];
    } else if (conn.o_index == 3) {
        this->c_front[1] = conn;
        return &this->c_front[1];
    }

    return 0;
}
