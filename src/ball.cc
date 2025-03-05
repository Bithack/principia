#include "material.hh"
#include "ball.hh"
#include "world.hh"
#include "model.hh"
#include "game.hh"

void ball_update_customz(struct tms_entity *e)
{
    ball *b = (ball*)e;

    b2Transform t;
    t = b->get_body(0)->GetTransform();

    //tmat4_load_identity(this->M);
    b->M[0] = t.q.c;
    b->M[1] = t.q.s;
    b->M[4] = -t.q.s;
    b->M[5] = t.q.c;
    b->M[12] = t.p.x;
    b->M[13] = t.p.y;
    b->M[14] = b->z * LAYER_DEPTH;

    tmat3_copy_mat4_sub3x3(b->N, b->M);
}

ball::ball(int type)
{
    this->width = .5f;
    this->set_flag(ENTITY_ALLOW_ROTATION, false);
    this->set_flag(ENTITY_ALLOW_CONNECTIONS, false);

    this->type = ENTITY_BALL;

    this->btype = type;

    this->saved_layer = 0;
    this->layer_new = 0;
    this->layer_old = 0;
    this->layer_blend = 1.f;

    this->set_mesh(mesh_factory::get_mesh(MODEL_SPHERE));

    switch (type) {
        case 0: // Ball
            this->set_material(&m_wood);
            break;
        case 1: // Metal ball
            this->set_material(&m_iron);
            this->set_flag(ENTITY_IS_MAGNETIC, true);
            break;
        case 2: // Interactive ball
            this->set_material(&m_interactive);
            this->set_flag(ENTITY_IS_INTERACTIVE, true);
            break;
    }

    this->layer_mask = 6;

    tmat4_load_identity(this->M);
    tmat3_load_identity(this->N);
}

void
ball::on_load(bool created, bool has_state)
{
    this->saved_layer = this->get_layer();
    this->layer_new = this->get_layer();
    this->layer_old = this->get_layer();
    this->layer_blend = 1.f;
}

void
ball::setup()
{
    this->saved_layer = this->get_layer();

    this->layer_new = this->get_layer();
    this->layer_old = this->get_layer();
    this->layer_blend = 1.f;

    this->initialize_interactive();
}

void
ball::on_pause()
{
    this->set_layer(this->saved_layer);
    this->layer_new = this->get_layer();
    this->layer_old = this->get_layer();
    this->layer_blend = 1.f;
}

void
ball::construct()
{
    this->saved_layer = this->get_layer();
}

void
ball::set_layer(int l)
{
    entity::set_layer(l);

    if (W->is_paused()) {
        this->layer_new = this->get_layer();
        this->layer_old = this->get_layer();
        this->layer_blend = 1.f;
    }
}

void
ball::layermove(int dir)
{
    int newlayer = this->get_layer()+dir;
    if (newlayer < 0) newlayer = 0;
    else if (newlayer > 2) newlayer = 2;

    this->layer_new = (float)newlayer;
    this->layer_old = this->get_layer();
    this->layer_blend = 0.f;

    this->set_layer(newlayer);
}

void
ball::add_to_world()
{
    if (W->is_paused())
        this->create_circle(this->get_dynamic_type(), .26f, this->material);
    else
        this->create_circle(this->get_dynamic_type(), .25f, this->material);

    ((struct tms_entity*)this)->update = tms::_oopassthrough_entity_update;
}

