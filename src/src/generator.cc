#include "generator.hh"
#include "material.hh"
#include "game.hh"
#include "model.hh"

generator::generator()
    : voltage(3.f)
{
    this->set_mesh(mesh_factory::get_mesh(MODEL_GENERATOR));
    this->set_material(&m_gen);

    this->menu_scale = 1.f/1.5f;

    /*
    this->ewidth = 3;
    this->eheight = 4;
    */

    delete [] this->s_out;
    this->s_out = new socket_out[9];

    this->num_s_in = 1;
    this->num_s_out = 9;

    for (int x=0; x<9; x++) {
        float sy = .3f - x/3 * .3f;
        float sx = x%3 * .3f;
        this->s_out[x].lpos = b2Vec2(sx,sy);
        this->s_out[x].angle = M_PI/2.f;
        this->s_out[x].ctype = CABLE_BLACK;
    }

    this->s_in[0].lpos = b2Vec2(-.6f,-.3f);
    this->s_in[0].angle = -M_PI/2.f;
    this->s_in[0].ctype = CABLE_RED;
    this->scaleselect = true;

    this->num_sliders = 1;

    this->set_num_properties(1);
    this->properties[0].v.f = 9.f;
    this->properties[0].type = P_FLT;

    this->set_as_rect(0.8f, .5f);
    /* previously 1.6f*.5f, 1.f*.5f */
}

float
generator::get_slider_value(int s)
{
    if (W->level.version >= LEVEL_VERSION_1_3_0_3)
        return tclampf((this->properties[0].v.f - 1.f) / 23.f, 0.f, 1.f);
    else
        return tclampf((this->properties[0].v.f - 1.f) / 47.f, 0.f, 1.f);
}

void
generator::on_load(bool created, bool has_state)
{
    if (W->level.version >= LEVEL_VERSION_1_3_0_3) {
        if (this->properties[0].v.f > 24.f) this->properties[0].v.f = 24.f;
    }
}

float
generator::get_slider_snap(int s)
{
    if (W->level.version >= LEVEL_VERSION_1_3_0_3)
        return 1.f / 23.f;
    else
        return 1.f / 47.f;
}

void
generator::on_slider_change(int s, float value)
{
    if (W->level.version >= LEVEL_VERSION_1_3_0_3) {
        this->properties[0].v.f = (value * 23.f) + 1.f;
    } else
        this->properties[0].v.f = (value * 47.f) + 1.f;
    G->show_numfeed(this->properties[0].v.f);
}

edevice*
generator::solve_electronics()
{
    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();

    float mul = 1.f;
    if (this->s_in[0].p) {
        mul = this->s_in[0].get_value();
    }

    if (W->level.version >= LEVEL_VERSION_1_3_0_3) {
        mul *= 3.f;
    }

    for (int x=0; x<9; x++) {
        this->s_out[x].write(this->properties[0].v.f*mul);// * v);
    }

    return 0;
}

void
generator::write_quickinfo(char *out)
{
    sprintf(out, "%s (%.0fv)", this->get_name(), this->properties[0].v.f);
}
