#include "pointer.hh"
#include "model.hh"
#include "material.hh"

pointer::pointer()
    : arrow_angle(0.f)
{
    this->set_mesh(mesh_factory::get_mesh(MODEL_POINTER_BODY));
    this->set_material(&m_pv_colored);

    this->arrow = new tms::entity();
    this->arrow->set_mesh(mesh_factory::get_mesh(MODEL_POINTER_ARROW));
    this->arrow->set_material(&m_pv_colored);
    this->arrow->set_uniform("~color", .7f, .9f, .7f, 1.f);
    this->add_child(this->arrow);

    this->num_s_in = 1;
    this->num_s_out = 0;

    this->update_method = ENTITY_UPDATE_CUSTOM;

    this->s_in[0].lpos = b2Vec2(.35f, .35f);
    this->s_in[0].ctype = CABLE_RED;

    this->set_uniform("~color", .3f, 0.3f, 0.3f, 1.f);

    this->set_as_rect(.5f, .5f);
}

void
pointer::update()
{
    b2Vec2 p = this->get_position();
    float a = this->get_angle();

    tmat4_load_identity(this->M);
    tmat4_translate(this->M, p.x, p.y, this->get_layer()*LAYER_DEPTH);
    tmat4_rotate(this->M, a*(180.f/M_PI), 0.f, 0.f, -1.f);

    tmat3_copy_mat4_sub3x3(this->N, this->M);

    tmat4_copy(this->arrow->M, this->M);
    tmat4_rotate(this->arrow->M, this->arrow_angle*(180.f/M_PI)-90.f, 0.f, 0.f, -1.f);

    tmat3_copy_mat4_sub3x3(this->arrow->N, this->arrow->M);
}

edevice*
pointer::solve_electronics()
{
    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();

    float v = this->s_in[0].get_value();
    float a = v*M_PI*2.f;

    this->arrow_angle = a;

    return 0;
}

