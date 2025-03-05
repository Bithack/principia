#include "corner.hh"
#include "material.hh"
#include "model.hh"
#include "game.hh"

class corner_ray_cb : public b2RayCastCallback
{
  public:
    entity *result;
    int which;
    corner *ignore;
    b2Vec2 result_point;
    b2Vec2 vec;

    corner_ray_cb(corner *ignore)
    {
        this->ignore = ignore;
        result = 0;
    }

    float32 ReportFixture(b2Fixture *f, const b2Vec2 &pt, const b2Vec2 &nor, float32 fraction)
    {
        if (f->IsSensor()) {
            return -1.f;
        }

        entity *r = static_cast<entity*>(f->GetUserData());
        if (r && r != this->ignore) {
            /* TODO: verify type of entity */

            connection *c = 0;

            if (r->get_layer() == ignore->get_layer()
                    && (r->g_id != 4)) {
                c = &ignore->c[this->which];

                if (c->pending) {
                    c->type = CONN_GROUP;
                    c->e = ignore;
                    c->o = r;
                    c->p = pt;
                    c->o_data = r->get_fixture_connection_data(f);
                    c->f[1] = VOID_TO_UINT8(f->GetBody()->GetUserData());
                    c->render_type = CONN_RENDER_SMALL;

                    switch (which) {
                        default: case 0: c->angle = 0.f; break;
                        case 1: c->angle = M_PI/4.f * 3.f; break;
                        case 2: c->angle = -M_PI/2.f; break;
                    }

                    if (!G->add_pair(ignore, r, c)) {
                        //if (!c->owned)
                            //G->return_tmp_conn(c);
                    }

                    return fraction;
                    //tms_infof("found %d", which);
                }
            }
        }

        return -1.f;
    }
};

corner::corner()
{
    this->set_flag(ENTITY_ALLOW_CONNECTIONS, false);
    this->set_flag(ENTITY_IS_MOVEABLE, true);
    this->set_material(&m_wood);
    this->set_mesh(mesh_factory::get_mesh(MODEL_CORNER));

    this->c[0].init_owned(0, this);
    this->c[0].type = CONN_GROUP;
    this->c[1].init_owned(1, this);
    this->c[1].type = CONN_GROUP;
    this->c[2].init_owned(2, this);
    this->c[2].type = CONN_GROUP;

    tmat4_load_identity(this->M);
    tmat3_load_identity(this->N);

    this->layer_mask = 6;
    this->set_as_tri(.35f, .35f);
}

connection*
corner::load_connection(connection &conn)
{
    this->c[conn.o_index] = conn;
    return &this->c[conn.o_index];
}

void
corner::on_load(bool created, bool has_state)
{

}

void
corner::find_pairs()
{
    corner_ray_cb handler(this);
    b2Vec2 vec[3] = {
        b2Vec2(1.f, 0.f),
        b2Vec2(-.707f, 0.707f),
        b2Vec2(0.f, -1.f),
    };
    b2Vec2 start[3] = {
        b2Vec2(.15f, 0.f),
        b2Vec2(.0f, 0.f),
        b2Vec2(0.f, -.15f),
    };

    for (int x=0; x<3; x++) {

        handler.vec = vec[x];
        handler.vec*=.1f;
        handler.which = x;

        W->raycast(&handler,
                this->local_to_world(start[x], 0),
                this->local_to_world(start[x]+handler.vec, 0));
    }
}
