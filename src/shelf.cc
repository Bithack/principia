#include "shelf.hh"
#include "material.hh"
#include "model.hh"

shelf::shelf()
{
    this->set_flag(ENTITY_IS_MAGNETIC, true);
    this->set_flag(ENTITY_IS_STATIC, true);

    this->width = 4.f;
    this->menu_scale = .25f;

    this->update_method = ENTITY_UPDATE_STATIC;

    this->set_mesh(mesh_factory::get_mesh(MODEL_WALLTHING2));
    this->set_material(&m_rackhouse); /* TODO: specific material? */
    //this->set_uniform("~color", .1f, .1f, .1f, 1.f);

    this->num_sliders = 1;

    this->set_num_properties(1);
    this->properties[0].v.i = 2;

    /* XXX size 3 is actually the smallest size */

    tmat4_load_identity(this->M);
    tmat3_load_identity(this->N);
}

float
shelf::get_slider_value(int s)
{
    uint32_t v = this->properties[0].v.i;
    float vv;

    if (v == 3) vv = 0.f;
    else vv = (float)(v+1)*.33333333f;

    return vv;
}

float
shelf::get_slider_snap(int s)
{
    return .333333333f;
}

void
shelf::on_load(bool created, bool has_state)
{
    if (this->properties[0].v.i > 3) {
        this->properties[0].v.i = 3;
    }

    this->update_mesh();
}

void
shelf::on_slider_change(int s, float value)
{
    uint32_t size = (uint32_t)roundf(value * 3.f);

    if (size == 0) size = 3;
    else size --;

    this->set_property(0, size);

    this->disconnect_all();
    this->recreate_shape();
}

float
shelf::update_mesh()
{
    float ww = 1.f;

    switch (this->properties[0].v.i) {
        case 0:
            this->set_mesh(mesh_factory::get_mesh(MODEL_WALLTHING0));
            ww = 1.f;
            break;
        case 1:
            this->set_mesh(mesh_factory::get_mesh(MODEL_WALLTHING1));
            ww = 1.5f;
            break;
        default:
        case 2:
            this->set_mesh(mesh_factory::get_mesh(MODEL_WALLTHING2));
            ww = 2.0f;
            break;

        case 3:
            this->set_mesh(mesh_factory::get_mesh(MODEL_WALLTHING00));
            ww = .5f;
            break;
    }

    return ww;
}

void
shelf::recreate_shape()
{
    float ww = this->update_mesh();

    if (this->body && this->fx) {
        this->body->DestroyFixture(this->fx);
    }

    this->create_rect(b2_staticBody, ww, .25f, this->material);
}

void
shelf::add_to_world()
{
    this->recreate_shape();
}

