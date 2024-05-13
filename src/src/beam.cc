#include "beam.hh"
#include "material.hh"
#include "model.hh"
#include "game.hh"
#include "ui.hh"

class beam_ray_cb : public b2RayCastCallback
{
  public:
    entity *result;
    beam *ignore;
    b2Vec2 result_point;
    uint8_t result_frame;
    b2Fixture *result_fx;
    b2Vec2 vec;
    int dir;

    beam_ray_cb(beam *ignore)
    {
        this->ignore = ignore;
        result = 0;
    }

    float32 ReportFixture(b2Fixture *f, const b2Vec2 &pt, const b2Vec2 &nor, float32 fraction)
    {
        if (f->IsSensor()) {
            return -1.f;
        }

        entity *r = (entity*)f->GetUserData();

        float ret = 1.f;

        if (r && r != this->ignore) {
            /* TODO: verify type of entity */

            connection *c = 0;

            if ((f->GetFilterData().categoryBits & ignore->fx->GetFilterData().categoryBits) != 0) {
                c = (this->dir == 0 ? &ignore->c[0] : &ignore->c[1]);
                if (c->pending) {
                    this->result = r;
                    this->result_fx = f;
                    this->result_frame = VOID_TO_UINT8(f->GetBody()->GetUserData());
                    this->result_point = pt;
                }
                ret = fraction;
            } else if (r->get_layer() == ignore->get_layer()+1  || (r->g_id == O_LASER_BOUNCER && r->get_layer() == ignore->get_layer()-1)
                    /* && (r->group == 0 ||
                     r->group != ignore->group
                     )*/) { /* XXX */

                if (r->type == ENTITY_PLANK) {
                    b2Vec2 p = pt + vec;

                    if (ignore->fx->TestPoint(p)) {
                        c = G->get_tmp_conn();
                        c->type = CONN_GROUP;
                        c->typeselect = 1;
                        c->p = pt + vec;
                        c->e = ignore;
                        c->o = r;
                        c->o_data = r->get_fixture_connection_data(f);

                        if (W->level.flag_active(LVL_NAIL_CONNS)) {
                            c->render_type = CONN_RENDER_SMALL;
                        } else {
                            c->render_type = CONN_RENDER_DEFAULT;
                        }

                        c->f[1] = VOID_TO_UINT8(f->GetBody()->GetUserData());

                        if (!G->add_pair(ignore, r, c)) {
                            G->return_tmp_conn(c);
                        }
                    }
                }
            }

            /* TODO: splank */
        }

        return ret;
    }
};

beam::beam(int btype)
{
    this->do_update_fixture = false;
    this->btype = btype;
    this->type = ENTITY_PLANK;
    this->num_sliders = 1;
    this->menu_scale = .25f;

    this->set_flag(ENTITY_IS_MOVEABLE,  true);
    this->set_flag(ENTITY_DO_TICK,      true);
    this->set_flag(ENTITY_IS_BEAM,      true);

    switch (this->btype) {
        case BEAM_THICK:
            this->set_num_properties(1); /* 1 property for the size */
            this->set_mesh(mesh_factory::get_mesh(MODEL_PLANK4));
            this->set_material(&m_wood);
            break;

        case BEAM_THIN:
            this->set_num_properties(1); /* 1 property for the size */
            this->set_mesh(mesh_factory::get_mesh(MODEL_THINPLANK4));
            this->set_material(&m_wood);
            break;

        case BEAM_RUBBER:
            this->set_num_properties(3); /* 3 property for the size, restitution, friction */
            this->properties[1].type = P_FLT; // restitution
            this->properties[2].type = P_FLT; // friction
            this->properties[1].v.f = m_rubber.restitution;
            this->properties[2].v.f = m_rubber.friction;
            this->set_mesh(mesh_factory::get_mesh(MODEL_PLANK4));
            this->rubber_material = m_rubber;
            this->set_material(&this->rubber_material);
            if (W->level.version >= LEVEL_VERSION_1_4) {
                this->set_flag(ENTITY_HAS_CONFIG, true);
                this->dialog_id = DIALOG_RUBBER;
            }
            break;

        case BEAM_SEP:
        case BEAM_PLASTIC:
            this->set_flag(ENTITY_HAS_CONFIG, true);
            this->set_flag(ENTITY_IS_PLASTIC, true);
            this->dialog_id = DIALOG_BEAM_COLOR;
            this->set_num_properties(5); /* property for the size */
                                         /* + color in RGB */
                                         /* + density scale */
            this->properties[1].type = P_FLT;
            this->properties[2].type = P_FLT;
            this->properties[3].type = P_FLT;
            this->properties[4].type = P_FLT;
            this->set_color4(.2f, .2f, .2f);
            this->set_property(4, 1.f); /* density scale */
            this->set_mesh(mesh_factory::get_mesh(MODEL_PLANK4));
            this->set_material(&m_plastic);
            this->num_sliders = 2;
            break;
    }

    if (this->btype == BEAM_SEP) {
        this->set_flag(ENTITY_IS_DEV, true);
        this->dialog_id = DIALOG_BEAM_COLOR;
        this->set_property(0, (uint32_t)1);
        this->num_sliders = 0;
        this->set_mesh(mesh_factory::get_mesh(MODEL_SEPARATOR));
    } else {
        this->set_property(0, (uint32_t)3);
    }

    this->c[0].init_owned(0, this);
    this->c[0].type = CONN_GROUP;
    this->c[0].angle = M_PI;
    this->c[1].init_owned(1, this);
    this->c[1].angle = 0.f;
    this->c[1].type = CONN_GROUP;

    tmat4_load_identity(this->M);
    tmat3_load_identity(this->N);

    this->layer_mask = 6;

    this->update_fixture();
}

connection*
beam::load_connection(connection &conn)
{
    this->c[conn.o_index] = conn;
    this->c[0].angle = M_PI;
    this->c[1].angle = 0.f;
    return &this->c[conn.o_index];
}

void
beam::on_load(bool created, bool has_state)
{
    this->update_fixture();

    if (this->btype == BEAM_PLASTIC || this->btype == BEAM_SEP) {
        float r = this->properties[1].v.f;
        float g = this->properties[2].v.f;
        float b = this->properties[3].v.f;
        this->set_uniform("~color", r, g, b, 1.f);

        if (created) {
            this->set_color4(r, g, b);
        }
    }
}


void
beam::set_color(tvec4 c)
{
    if (this->btype == BEAM_PLASTIC || this->btype == BEAM_SEP) {
        this->properties[1].v.f = c.r;
        this->properties[2].v.f = c.g;
        this->properties[3].v.f = c.b;
        this->set_uniform("~color", c.r, c.g, c.b, c.a);

        if (this->gr) {
            W->add_action(this->id, ACTION_FINALIZE_GROUP);
        }
    }
}

tvec4
beam::get_color()
{
    if (this->btype == BEAM_PLASTIC || this->btype == BEAM_SEP) {
        float r = this->properties[1].v.f;
        float g = this->properties[2].v.f;
        float b = this->properties[3].v.f;

        return tvec4f(r, g, b, 1.0f);
    } else {
        return tvec4f(0.f, 0.f, 0.f, 0.f);
    }
}

void
beam::update_fixture()
{
    uint32_t size = this->properties[0].v.i;
    if (size > 3) size = 3;
    this->properties[0].v.i = (uint32_t)size;

    this->set_property(0, (uint32_t)size);

    if (this->btype == BEAM_RUBBER) {
        // before we recreate the shape, we need to update the material!
        if (W->level.version >= LEVEL_VERSION_1_4) {
            this->rubber_material.restitution = this->properties[1].v.f;
            this->rubber_material.friction = this->properties[2].v.f;
        }
        //this->set_material(&this->rubber_material);
    }

    switch (this->btype) {

        case BEAM_SEP:
            this->set_as_rect(((float)size+1.f)/2.f-.025f, .125f);
            break;

        case BEAM_THICK:
        case BEAM_RUBBER:
        case BEAM_PLASTIC:
            this->set_mesh(mesh_factory::models[MODEL_PLANK1+size].mesh);

            this->set_as_rect(((float)size+1.f)/2.f, .15f);
            break;

        case BEAM_THIN:
            this->set_mesh(mesh_factory::models[MODEL_THINPLANK1+size].mesh);

            this->set_as_rect(((float)size+1.f)/2.f, .125f);
            break;
    }

    if (this->body) {
        this->recreate_shape();
    }
}

void
beam::tick()
{
    if (this->do_update_fixture) {
        this->update_fixture();
    }
}

void
beam::on_slider_change(int s, float value)
{
    if (s <= 0) {
        uint32_t size = (uint32_t)roundf(value * 3.f);
        G->animate_disconnect(this);
        this->disconnect_all();
        this->set_property(0, size);

        this->update_fixture();
        G->show_numfeed(1.f+size);
    } else {
        this->help_set_density_scale(value);
        G->show_numfeed(this->properties[4].v.f);
    }
}

void
beam::find_pairs()
{
    beam_ray_cb handler(this);

    for (int x=0; x<2; x++) {
        b2Vec2 dir[2];
        float sign = (x == 0 ? 1.f : -1.f);
        dir[0] = b2Vec2(-0.25f*sign, 0.f);
        dir[1] = b2Vec2((this->width + .25f)*sign, 0.f);

        b2Vec2 p1 = this->local_to_world(dir[0], 0);
        b2Vec2 p2 = this->local_to_world(dir[1], 0);

        handler.result = 0;
        handler.dir = x;
        handler.vec = p2 - this->get_position();
        handler.vec *= 1.f/handler.vec.Length();
        handler.vec *= .15f;

        W->raycast(&handler, p1, p2);

        if (handler.result && this->c[x].pending) {
            this->c[x].type = CONN_GROUP;
            this->c[x].o = handler.result;
            this->c[x].p = handler.result_point;
            this->c[x].f[1] = handler.result_frame;
            this->c[x].o_data = handler.result->get_fixture_connection_data(handler.result_fx);

            if (W->level.flag_active(LVL_NAIL_CONNS)) {
                this->c[x].render_type = CONN_RENDER_NAIL;
            } else {
                this->c[x].render_type = CONN_RENDER_DEFAULT;
            }
            G->add_pair(this, handler.result, &this->c[x]);
        }
    }
}

room::room()
{
    this->set_flag(ENTITY_IS_DEV, true);
    this->set_flag(ENTITY_ALLOW_CONNECTIONS, true);
    this->set_flag(ENTITY_IS_LOW_PRIO, true);
    this->layer_mask = 1;
    this->num_sliders = 2;

    this->set_num_properties(2);
    this->properties[0].type = P_INT;
    this->properties[0].v.i = 1;
    this->properties[1].type = P_INT;
    this->properties[1].v.i = 1;

    this->set_mesh(mesh_factory::get_mesh(MODEL_ROOM_BG));
    this->set_material(&m_room);
    this->set_uniform("size", 2.f, 2.f, 0.f, 0.f);
    this->set_uniform("ao_mask2", 1.f, 0.f, 0.f, 0.f);

    this->set_as_rect(2.48f/2.f, 2.48f/2.f);
}


void
room::set_layer(int z)
{
    switch (z) {
        case 0: this->set_uniform("ao_mask2", 1.f, 0.f, 0.f, 0.f); break;
        case 1: this->set_uniform("ao_mask2", 0.f, 1.f, 0.f, 0.f); break;
        case 2: this->set_uniform("ao_mask2", 0.f, 0.f, 1.f, 0.f); break;
    }
    entity::set_layer(z);
}

void
room::on_slider_change(int s, float value)
{
    if (s == 0) {
        uint32_t num_corners = (uint32_t)roundf(value * 4.f);
        G->animate_disconnect(this);
        this->disconnect_all();
        this->set_property(0, num_corners);
    } else {
        this->properties[1].v.i = roundf(value);
    }
}

void
room::create_sensor()
{
    /* abuse of the craete_sensor function, we actually force the fixture not to collide with dynamic objects */
    b2Filter d = world::get_filter_for_layer(this->get_layer(), 1);
    d.groupIndex = 1+this->get_layer();
    this->fx->SetFilterData(d);
}
