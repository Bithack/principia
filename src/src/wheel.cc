#include "wheel.hh"
#include "model.hh"
#include "material.hh"
#include "world.hh"
#include "game.hh"
#include "ui.hh"

wheel::wheel()
{
    this->set_flag(ENTITY_IS_MOVEABLE, true);
    this->set_flag(ENTITY_ALLOW_AXIS_ROT, true);
    this->set_flag(ENTITY_DO_TICK, true);
    if (W->level.version >= LEVEL_VERSION_1_4) {
        this->set_flag(ENTITY_HAS_CONFIG, true);
        this->dialog_id = DIALOG_RUBBER;
    }

    if (W->level.version >= LEVEL_VERSION_1_1_6) {
        m_wheel.friction = 2.5f;
    } else {
        m_wheel.friction = 1.5f;
    }

    this->set_mesh(mesh_factory::get_mesh(MODEL_WHEEL));
    this->rubber_material = m_wheel;
    this->set_material(&this->rubber_material);

    this->update_method = ENTITY_UPDATE_CUSTOM;

    this->type = ENTITY_WHEEL;
    this->menu_scale = 1.f/1.5f;

    this->set_num_properties(3);
    this->properties[0].type = P_INT; // size
    this->properties[1].type = P_FLT; // restitution
    this->properties[2].type = P_FLT; // friction
    this->properties[0].v.i = 0;
    this->properties[1].v.f = m_wheel.restitution;
    this->properties[2].v.f = m_wheel.friction;

    this->num_sliders = 1;

    tmat4_load_identity(this->M);
    tmat3_load_identity(this->N);

    this->c_back.init_owned(0, this);
    this->c_back.type = CONN_GROUP;
    this->c_back.typeselect = 1; /* allow the player to select between weld and pivot joints */

    this->c_front.init_owned(1, this);
    this->c_front.type = CONN_GROUP;
    this->c_front.typeselect = 1;

    this->set_as_circle(.75f);

    this->do_update_fixture = true;
}

void
wheel::tick()
{
    if (this->do_update_fixture) {
        if (W->level.version >= LEVEL_VERSION_1_4) {
            this->rubber_material.restitution = this->properties[1].v.f;
            this->rubber_material.friction = this->properties[2].v.f;

            this->set_as_circle((.75f*((float)this->properties[0].v.i*.5f+1.f)));
        }

        this->do_update_fixture = false;
    }
}

bool
wheel::ReportFixture(b2Fixture *f)
{
    entity *e = static_cast<entity*>(f->GetUserData());
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
                q_fx = f;
                q_frame = fr;
                q_dir = dist;
                return false;
            }
        }
    }

    return true;
}

void
wheel::find_pairs()
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
                c->type = CONN_GROUP;
                c->typeselect = this->find_pivot(0,false) == 0 && this->find_pivot(1, false) == 0;

                if (W->level.type == LCAT_ADVENTURE && !W->is_paused()) {
                    c->typeselect = false;
                    c->option = 1;
                }

                c->o = q_result;
                c->p = p;
                c->f[1] = q_frame;
                c->o_data = q_result->get_fixture_connection_data(q_fx);
                G->add_pair(this, q_result, c);
            }
        }
    }
}

void
wheel::setup()
{
    if (W->level.version >= LEVEL_VERSION_1_4) {
        this->fd.friction = this->rubber_material.friction = this->properties[2].v.f;
        this->fd.restitution = this->rubber_material.restitution = this->properties[1].v.f;
    }
}

void
wheel::on_load(bool created, bool has_state)
{
    this->on_slider_change(-1, (float)this->properties[0].v.i / 2.f);

    if (W->level.version >= LEVEL_VERSION_1_4) {
        this->fd.friction = this->rubber_material.friction = this->properties[2].v.f;
        this->fd.restitution = this->rubber_material.restitution = this->properties[1].v.f;
    }
}

connection *
wheel::load_connection(connection &conn)
{
    if (conn.o_index == 0) {
        this->c_back = conn;
        return &this->c_back;
    } else {
        this->c_front = conn;
        return &this->c_front;
    }
}

float
wheel::get_slider_value(int s)
{
    return this->properties[0].v.i / 2.f;
}

float
wheel::get_slider_snap(int s)
{
    return .5f;
}

void
wheel::on_slider_change(int s, float value)
{
    uint32_t size = (uint32_t)roundf(value*2.f);
    if (size > 2) size = 2;

    this->set_property(0, size);

    if (s != -1) this->disconnect_all();

    this->set_as_circle((.75f*((float)size*.5f+1.f)));
    this->fd.density = .5f - .125f * size;
    this->recreate_shape();
}

void
wheel::update()
{
    //if (this->body) {
        b2Vec2 p = this->get_position();
        float a = this->get_angle();

        tmat4_load_identity(this->M);
        tmat4_translate(this->M, p.x, p.y, this->get_layer()*LAYER_DEPTH);
        tmat4_rotate(this->M, a*(180.f/M_PI), 0.f, 0.f, -1.f);
        if (this->flag_active(ENTITY_AXIS_ROT))
            tmat4_rotate(this->M, 180, 1.f, 0.f, 0.f);

        tmat3_copy_mat4_sub3x3(this->N, this->M);

        tmat4_scale(this->M, (1.f+this->properties[0].v.i*.5f), (1.f+this->properties[0].v.i*.5f), 1.f);
        /*
    } else {
        tmat4_load_identity(this->M);
        tmat4_translate(this->M, this->_pos.x, this->_pos.y, 0);
        tmat4_rotate(this->M, this->_angle * (180.f/M_PI), 0, 0, -1);
        tmat3_copy_mat4_sub3x3(this->N, this->M);
    }
*/
}

void
wheel::toggle_axis_rot()
{
    this->set_flag(ENTITY_AXIS_ROT, !this->flag_active(ENTITY_AXIS_ROT));
}
