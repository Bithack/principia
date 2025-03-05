#include "tester.hh"
#include "material.hh"
#include "game.hh"
#include "model.hh"

tester::tester()
{
    this->set_mesh(mesh_factory::get_mesh(MODEL_DEBUGGER1));
    this->set_material(&m_edev_dark);

    this->num_sliders = 1;
    this->curr_update_method = this->update_method = ENTITY_UPDATE_CUSTOM;

    this->num_s_in = 1;
    this->num_s_out = 1;

    this->s_in[0].lpos = b2Vec2(-.125f, -.35f);
    this->s_out[0].lpos = b2Vec2(.125f, -.35f);

    this->set_num_properties(1);
    this->properties[0].type = P_INT;
    this->properties[0].v.i  = 1;

    tms_entity_init(&this->lamp);
    tms_entity_set_mesh(&this->lamp, mesh_factory::get_mesh(MODEL_BOX1));
    tms_entity_set_material(&this->lamp, &m_pv_colored);
    float v = 0.f;
    tms_entity_set_uniform4f(&this->lamp, "~color", v, 1.f+v, v, 1.f);
    tms_entity_add_child(this, &this->lamp);

    this->set_shape();
}

tester::~tester()
{
    tms_entity_uninit(&this->lamp);
}

void
tester::update()
{
    b2Vec2 p = this->get_position();
    float a = this->get_angle();
    float cs,sn;
    tmath_sincos(a, &sn, &cs);

    tmat4_load_identity(this->M);

    this->M[0] = cs;
    this->M[1] = sn;
    this->M[4] = -sn;
    this->M[5] = cs;
    this->M[12] = p.x;
    this->M[13] = p.y;
    this->M[14] = this->prio * LAYER_DEPTH;

    tmat3_copy_mat4_sub3x3(this->N, this->M);

    tmat4_copy(this->lamp.M, this->M);
    tmat3_copy(this->lamp.N, this->N);
}

void
tester::on_load(bool created, bool has_state)
{
    this->set_shape();
}

void
tester::set_shape()
{
    if (this->properties[0].v.i > 1) this->properties[0].v.i = 1;

    this->set_mesh(mesh_factory::get_mesh(MODEL_DEBUGGER0+this->properties[0].v.i));
    tms_entity_set_mesh(&this->lamp, mesh_factory::get_mesh(MODEL_BOX0+this->properties[0].v.i));

    switch (this->properties[0].v.i) {
        case 0:
            this->s_in[0].lpos = b2Vec2(-.125f, -.125f);
            this->s_out[0].lpos = b2Vec2(.125f, -.125f);
            break;
        case 1:
        default:
            this->s_in[0].lpos = b2Vec2(-.125f, -.35f);
            this->s_out[0].lpos = b2Vec2(.125f, -.35f);
            break;
    }

    float s = ((float)this->properties[0].v.i+1.f)/4.f;

    this->set_as_rect(s, s);
    this->recreate_shape();

    this->query_sides[0].SetZero(); /* up */
    this->query_sides[1].SetZero(); /* left */
    this->query_sides[3].SetZero(); /* right */
}

edevice*
tester::solve_electronics()
{
    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();

    float v = tclampf(this->s_in[0].get_value(), 0.f, 1.f);
    this->s_out[0].write(v);

    tms_entity_set_uniform4f(&this->lamp, "~color", v, 1.f+v, v, 1.f);

    return 0;
}

void
tester::on_slider_change(int s, float value)
{
    uint32_t size = (uint32_t)roundf(value);
    if (size > 1) size = 1;
    this->set_property(0, size);

    this->set_mesh(mesh_factory::get_mesh(MODEL_DEBUGGER0+size));
    tms_entity_set_mesh(&this->lamp, (struct tms_mesh*)mesh_factory::get_mesh(MODEL_BOX0+size));

    if (s != -1) {
        this->disconnect_all();
        this->set_shape();
    }
}
