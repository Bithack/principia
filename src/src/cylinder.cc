#include "cylinder.hh"
#include "game.hh"
#include "model.hh"
#include "material.hh"

cylinder::cylinder(int _type)
{
    this->set_mesh(mesh_factory::get_mesh(MODEL_CYLINDER1));

    this->type = ENTITY_WHEEL;
    this->obj_type = _type;

    if (this->obj_type == 1) {
        this->set_flag(ENTITY_IS_INTERACTIVE, true);
        this->set_material(&m_interactive);
    } else {
        this->set_material(&m_wood);
    }

    this->layer_mask = 2+4;

    tmat4_load_identity(this->M);
    tmat3_load_identity(this->N);
    this->set_flag(ENTITY_IS_MOVEABLE, true);

    this->c_back.init_owned(0, this);
    this->c_back.type = CONN_GROUP;
    this->c_back.typeselect = 1; /* allow the player to select between weld and pivot joints */

    this->c_front.init_owned(1, this);
    this->c_front.type = CONN_GROUP;
    this->c_front.typeselect = 1;

    this->width = 1.0f;
    this->height = 1.0f;
    this->num_sliders = 1;

    //this->set_uniform("~color", .1f, 0.1f, 0.1f, 1.f);
    this->width = .5f*(1.f+.5f/2.f);
    this->set_as_circle(.5f);

    this->set_num_properties(1);
    this->properties[0].type = P_INT;
    this->properties[0].v.i = 1;
}

float
cylinder::get_slider_value(int s)
{
    return (float)this->properties[0].v.i / 3.f;
}

float
cylinder::get_slider_snap(int s)
{
    return .333333333f;
}

void
cylinder::on_load(bool created, bool has_state)
{
    this->on_slider_change(-1, (float)this->properties[0].v.i / 3.f);
}

connection *
cylinder::load_connection(connection &conn)
{
    if (conn.o_index == 0) {
        this->c_back = conn;
        return &this->c_back;
    } else {
        this->c_front = conn;
        return &this->c_front;
    }
}

void
cylinder::setup()
{
    this->initialize_interactive();
}

void
cylinder::on_slider_change(int s, float value)
{
    uint32_t size = (uint32_t)roundf(value*3.f);
    if (size > 3) size = 3;
    this->set_property(0, size);

    this->set_mesh(mesh_factory::get_mesh(MODEL_CYLINDER05+size));

    this->width = .25f+size*.5f;

    //this->width = ((float)size+1.f)/2.f;
    /* XXX s set to -1 when called from on_load */
    if (s != -1) this->disconnect_all();
    this->set_as_circle(.25f+size*.25f);
    this->recreate_shape();
}

class cyl_query_cb : public b2QueryCallback
{
  public:
    entity *result;
    b2Fixture *result_fx;
    cylinder *ignore;
    uint8_t result_frame;
    int result_dir;
    b2Vec2 point;

    cyl_query_cb(cylinder *ignore, b2Vec2 point)
    {
        this->result = 0;
        this->ignore = ignore;
        this->point = point;
    }

    bool ReportFixture(b2Fixture *f)
    {
        entity *e = (entity*)f->GetUserData();
        uint8_t fr = (uint8_t)(uintptr_t)f->GetBody()->GetUserData();

        int open = ignore->c_back.pending | (ignore->c_front.pending <<1);

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

void
cylinder::find_pairs()
{
    if (this->c_back.pending || this->c_front.pending/* && this->body*/) {
        b2Vec2 p = this->get_position();//this->body->GetPosition();
        b2AABB aabb;
        cyl_query_cb cb(this, p);
        aabb.lowerBound.Set(p.x - .05f, p.y - .05f);
        aabb.upperBound.Set(p.x + .05f, p.y + .05f);
        W->b2->QueryAABB(&cb, aabb);

        if (cb.result != 0) {
            connection *c;
            if (cb.result_dir == 1)
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
                c->o_data = cb.result->get_fixture_connection_data(cb.result_fx);
                c->o = cb.result;
                c->p = p;
                c->f[1] = cb.result_frame;
                G->add_pair(this, cb.result, c);
            }
        }
    }
}
