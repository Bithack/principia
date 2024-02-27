#include "stabilizer.hh"
#include "material.hh"
#include "model.hh"
#include "game.hh"

#define DAMPING_MUL 100.f
#define LDAMPING_MUL 5.f

estabilizer::estabilizer()
    : adamp(0.f)
    , ldamp(0.f)
    , do_refresh_damping(false)
{
    this->set_flag(ENTITY_DO_STEP, true);
    this->set_flag(ENTITY_ALLOW_CONNECTIONS, false);

    this->set_mesh(mesh_factory::get_mesh(MODEL_STABILIZER));
    this->set_material(&m_iomisc);

    this->num_sliders = 2;

    this->num_s_in = 2;

    this->s_in[0].lpos = b2Vec2(-.125f, -.04f);
    this->s_in[1].lpos = b2Vec2( .125f, -.04f);

    this->set_num_properties(2);
    this->properties[0].type = P_FLT;
    this->properties[0].v.f = .05f;
    this->properties[1].type = P_FLT;
    this->properties[1].v.f = .00f;

    if (W->level.version >= LEVEL_VERSION_1_5) {
        this->set_as_rect(.3f, .2f);
    } else {
        this->set_as_rect(.25f, .25f);
    }
}

void
estabilizer::refresh_damping()
{
    b2Body *b = this->get_body(0);
    if (b) {
        b->SetAngularDamping(this->adamp * DAMPING_MUL);
        b->SetLinearDamping(this->ldamp * LDAMPING_MUL);

        std::set<entity*> loop;
        this->gather_connected_entities(&loop, false, false);

        for (std::set<entity*>::iterator i = loop.begin();
                i != loop.end(); i++) {
            if ((b = (*i)->get_body(0))) {
                b->SetAngularDamping(this->adamp * DAMPING_MUL);
                b->SetLinearDamping(this->ldamp * LDAMPING_MUL);
            }
        }
    }
}

void
estabilizer::step()
{
    if (this->do_refresh_damping) {
        this->refresh_damping();

        this->do_refresh_damping = false;
    }
}

void
estabilizer::setup()
{
    this->adamp = this->properties[0].v.f;
    this->ldamp = this->properties[1].v.f;

    this->refresh_damping();
}

void
estabilizer::on_slider_change(int s, float value)
{
    this->properties[s].v.f = value;
    G->show_numfeed(value);
}

edevice*
estabilizer::solve_electronics()
{
    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();
    if (!this->s_in[1].is_ready())
        return this->s_in[1].get_connected_edevice();

    if (this->s_in[0].p) {
        float v = this->s_in[0].get_value();

        if (std::abs(v - this->adamp) > 0.00125f) {
            this->adamp = v;
            this->do_refresh_damping = true;
        }
    }

    if (this->s_in[1].p) {
        float v = this->s_in[1].get_value();

        if (std::abs(v - this->ldamp) > 0.00125f) {
            this->ldamp = v;
            this->do_refresh_damping = true;
        }
    }

    return 0;
}
