#include "box.hh"
#include "model.hh"
#include "material.hh"
#include "game.hh"
#include "ui.hh"
#include "group.hh"

box::box(int box_type)
{
    this->type = ENTITY_WHEEL;
    this->box_type = box_type;
    this->set_mesh(mesh_factory::get_mesh(MODEL_BOX0));

    switch (box_type) {
        case BOX_NORMAL:
            this->set_material(&m_wood);
            break;
        case BOX_INTERACTIVE:
            this->set_material(&m_interactive);
            this->set_flag(ENTITY_IS_INTERACTIVE, true);
            break;
        case BOX_PLASTIC:
            {
                this->set_flag(ENTITY_HAS_CONFIG, true);
                this->set_flag(ENTITY_IS_PLASTIC, true);
                this->dialog_id = DIALOG_BEAM_COLOR;

                if (W->level.version >= LEVEL_VERSION_1_5) {
                    this->set_material(&m_plastic);
                } else {
                    this->set_material(&m_pv_colored);
                }
            }
            break;
    }

    this->set_flag(ENTITY_IS_MOVEABLE, true);
    this->num_sliders = 1;

    switch (this->box_type) {
        case BOX_PLASTIC:
            this->set_num_properties(5);
            this->properties[0].type = P_INT;
            this->properties[0].v.i = 1;
            this->properties[1].type = P_FLT;
            this->properties[2].type = P_FLT;
            this->properties[3].type = P_FLT;
            this->properties[4].type = P_FLT;
            this->set_color4(.5f, .3f, .3f);
            this->set_property(4, 1.f); /* density scale */
            this->num_sliders = 2;
            break;

        case BOX_NORMAL:
        case BOX_INTERACTIVE:
        default:
            this->set_num_properties(1);
            this->properties[0].type = P_INT;
            this->properties[0].v.i = 1;
            break;
    }

    this->c_back.init_owned(0, this);
    this->c_back.type = CONN_GROUP;
    this->c_back.typeselect = 1; /* allow the player to select between weld and pivot joints */

    this->c_front.init_owned(1, this);
    this->c_front.type = CONN_GROUP;
    this->c_front.typeselect = 1;

    this->c_side[0].init_owned(2, this);
    this->c_side[0].type = CONN_GROUP;
    this->c_side[1].init_owned(3, this);
    this->c_side[1].type = CONN_GROUP;
    this->c_side[2].init_owned(4, this);
    this->c_side[2].type = CONN_GROUP;
    this->c_side[3].init_owned(5, this);
    this->c_side[3].type = CONN_GROUP;

    float qw = .5f/2.f+0.15f;
    float qh = .5f/2.f+0.15f;
    this->query_sides[0].Set(0.f,  qh); /* up */
    this->query_sides[1].Set(-qw, 0.f); /* left */
    this->query_sides[2].Set(0.f, -qh); /* down */
    this->query_sides[3].Set( qw, 0.f); /* right */
    this->set_as_rect(.5f, .5f);
}

void
box::setup()
{
    if (this->box_type == BOX_INTERACTIVE) {
        this->initialize_interactive();
    }
}

float
box::get_slider_value(int s)
{
    if (s == 0) {
        return (float)this->properties[0].v.i;
    } else {
        return ((float)this->properties[4].v.f - ENTITY_DENSITY_SCALE_MIN) / ENTITY_DENSITY_SCALE_MAX;
    }
}

float
box::get_slider_snap(int s)
{
    if (s == 0)
        return 1.f;
    else
        return .05f;
}

void
box::on_load(bool created, bool has_state)
{
    this->on_slider_change(-1, (float)this->properties[0].v.i);

    if (this->box_type == BOX_PLASTIC) {
        float r = this->properties[1].v.f;
        float g = this->properties[2].v.f;
        float b = this->properties[3].v.f;
        this->set_uniform("~color", r, g, b, 1.f);

        if (created) {
            this->set_color4(r, g, b);
        }
    }
}

connection *
box::load_connection(connection &conn)
{
    if (conn.o_index == 0) {
        this->c_back = conn;
        return &this->c_back;
    } else if (conn.o_index == 1) {
        this->c_front = conn;
        return &this->c_front;
    } else if (conn.o_index >= 2 && conn.o_index <= 6) {
        this->c_side[conn.o_index-2] = conn;
        return &this->c_side[conn.o_index-2];
    }

    return 0;
}

void
box::on_slider_change(int s, float value)
{
    if (s <= 0) {
        uint32_t size = (uint32_t)roundf(value);
        if (size > 1) size = 1;
        this->set_property(0, size);
        this->set_mesh(mesh_factory::get_mesh(MODEL_BOX0+size));

        this->width = .5f*(size+1);

        //this->width = ((float)size+1.f)/2.f;
        /* XXX s set to -1 when called from on_load */
        if (s != -1) this->disconnect_all();
        float qw = (this->width/2.f)/2.f+0.15f;
        float qh = (this->width/2.f)/2.f+0.15f;
        this->query_sides[0].Set(0.f,  qh); /* up */
        this->query_sides[1].Set(-qw, 0.f); /* left */
        this->query_sides[2].Set(0.f, -qh); /* down */
        this->query_sides[3].Set( qw, 0.f); /* right */
        this->set_as_rect(this->width/2.f, this->width/2.f);
        this->recreate_shape();
    } else {
        this->help_set_density_scale(value);
        G->show_numfeed(this->properties[4].v.f);
    }
}

bool
box::ReportFixture(b2Fixture *f)
{
    entity *e = (entity*)f->GetUserData();
    uint8_t fr = VOID_TO_UINT8(f->GetBody()->GetUserData());

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
                q_result_fx = f;
                return false;
            }
        }
    }

    return true;
}

void
box::find_pairs()
{
    if (this->c_back.pending || this->c_front.pending) {
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
                c->o_data = q_result->get_fixture_connection_data(q_result_fx);

                c->typeselect = this->find_pivot(0,false) == 0 && this->find_pivot(1, false) == 0;

                if (W->level.type == LCAT_ADVENTURE && !W->is_paused()) {
                    c->typeselect = false;
                    c->option = 1;
                }

                c->o = q_result;
                c->p = p;
                c->f[1] = q_frame;
                G->add_pair(this, q_result, c);
            }
        }
    }

    this->sidecheck4(this->c_side);
}

void
box::set_color(tvec4 c)
{
    if (this->box_type == 2) {
        this->properties[1].v.f = c.r;
        this->properties[2].v.f = c.g;
        this->properties[3].v.f = c.b;
        this->set_uniform("~color", TVEC4_INLINE(c));

        if (this->gr) {
            W->add_action(this->id, ACTION_FINALIZE_GROUP);
        }
    }
}

tvec4
box::get_color()
{
    if (this->box_type == 2) {
        float r = this->properties[1].v.f;
        float g = this->properties[2].v.f;
        float b = this->properties[3].v.f;

        return tvec4f(r, g, b, 1.0f);
    } else {
        return tvec4f(0.f, 0.f, 0.f, 0.f);
    }
}
