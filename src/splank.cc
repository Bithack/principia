#include "splank.hh"
#include "world.hh"
#include "game.hh"
#include "model.hh"
#include "gui.hh"

float32
splank::ReportFixture(b2Fixture *f, const b2Vec2 &pt, const b2Vec2 &nor, float32 fraction)
{
    if (f->IsSensor()) {
        return -1.f;
    }

    entity *r = (entity*)f->GetUserData();

    float ret = 1.f;

    if (r && r != this) {
        connection *c = 0;

        if ((f->GetFilterData().categoryBits & this->fx->GetFilterData().categoryBits) != 0) {
            c = (this->q_dir == 0 ? &this->c[0] : &this->c[1]);
            if (c->pending) {
                this->q_result = r;
                this->q_frame = VOID_TO_UINT8(f->GetBody()->GetUserData());
                this->q_point = pt;
                this->q_fx = f;
            }
            ret = fraction;
        } else {
            if (r->type == ENTITY_PLANK) {
                b2Vec2 p = pt + this->q_vec;

                if (this->fx->TestPoint(p)) {
                    c = G->get_tmp_conn();
                    c->type = CONN_GROUP;
                    c->typeselect = 1;

                    c->e = this;
                    c->o = r;
                    c->f[1] = VOID_TO_UINT8(f->GetBody()->GetUserData());
                    c->p = p;
                    c->o_data = r->get_fixture_connection_data(f);

                    if (!G->add_pair(this, r, c)) {
                        G->return_tmp_conn(c);
                    }
                }
            }
        }
    }

    return ret;
}

splank::splank()
{
    if (W->level.version >= LEVEL_VERSION_1_5) {
        this->set_flag(ENTITY_ALLOW_AXIS_ROT, true);
    }

    this->type = ENTITY_PLANK;
    this->menu_scale = .5f;
    this->width = .5f;

    this->set_mesh(mesh_factory::get_mesh(MODEL_SPLANK0));
    this->set_material(&m_wood);

    this->set_num_properties(1);
    this->properties[0].type = P_INT; /* sublayer */
    this->properties[0].v.i = 0;

    this->layer_mask = 1;
    this->set_as_rect(1.f, .15f);
    this->num_sliders = 1;

    this->c[0].init_owned(0, this);
    this->c[0].type = CONN_GROUP;
    this->c[0].angle = M_PI;
    this->c[1].init_owned(1, this);
    this->c[1].angle = 0.f;
    this->c[1].type = CONN_GROUP;
}

connection*
splank::load_connection(connection &conn)
{
    this->c[conn.o_index] = conn;
    this->c[0].angle = M_PI;
    this->c[1].angle = 0.f;
    return &this->c[conn.o_index];
}

void
splank::on_slider_change(int s, float value)
{
    uint32_t size = (uint32_t)roundf(value * 3.f);
    G->animate_disconnect(this);
    this->disconnect_all();
    this->set_property(0, size);

    this->on_load(false, false);

    G->show_numfeed(size);
}

void
splank::toggle_axis_rot()
{
    this->set_flag(ENTITY_AXIS_ROT, !this->flag_active(ENTITY_AXIS_ROT));

    this->on_load(false, false);
}

struct tms_sprite*
splank::get_axis_rot_sprite()
{
    if (this->flag_active(ENTITY_AXIS_ROT)) {
        return gui_spritesheet::get_sprite(S_FLOOR);
    } else {
        return gui_spritesheet::get_sprite(S_NOT_FLOOR);
    }
}

const char*
splank::get_axis_rot_tooltip()
{
    if (this->flag_active(ENTITY_AXIS_ROT)) {
        return "Make wide";
    } else {
        return "Make thin";
    }
}

void
splank::on_load(bool created, bool has_state)
{
    if (this->properties[0].v.i > 3) {
        this->properties[0].v.i = 3;
    }

    if (!this->flag_active(ENTITY_AXIS_ROT)) {
        this->layer_mask = (1<<this->properties[0].v.i);
        this->set_mesh(mesh_factory::get_mesh(MODEL_SPLANK0 + this->properties[0].v.i));
        this->set_as_rect(1.f, .15f);
    } else {
        if (this->properties[0].v.i <= 1) {
            this->layer_mask = (1<<0) | (1<<1);
            this->set_mesh(mesh_factory::get_mesh(MODEL_SPLANK_BACK));
        } else {
            this->layer_mask = (1<<2) | (1<<3);
            this->set_mesh(mesh_factory::get_mesh(MODEL_SPLANK_FRONT));
        }
        this->set_as_rect(1.f, .15f/2.f);
    }

    this->recreate_shape();
}

void
splank::find_pairs()
{
    for (int x=0; x<2; x++) {
        b2Vec2 dir[2];
        float sign = (x == 0 ? 1.f : -1.f);
        dir[0] = b2Vec2(-(this->width + .25f) * sign, 0.f);
        dir[1] = b2Vec2( (this->width + .25f) * sign, 0.f);

        b2Vec2 p1 = this->local_to_world(dir[0], 0);
        b2Vec2 p2 = this->local_to_world(dir[1], 0);

        this->q_result = 0;
        this->q_dir = x;
        this->q_vec = p2 - this->get_position();
        this->q_vec *= 1.f/this->q_vec.Length();
        this->q_vec *= .15f;

        W->raycast(this, p1, p2,
                x?1.f:0.f, x?0.f:1.f);

        if (this->q_result && this->c[x].pending) {
            this->c[x].type = CONN_GROUP;
            this->c[x].o = q_result;
            this->c[x].p = q_point;
            this->c[x].f[1] = q_frame;
            this->c[x].o_data = q_result->get_fixture_connection_data(q_fx);
            G->add_pair(this, q_result, &this->c[x]);
        }
    }
}
