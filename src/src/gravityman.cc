#include "gravityman.hh"
#include "creature.hh"
#include "game.hh"
#include "gui.hh"
#include "material.hh"
#include "model.hh"

gravityman::gravityman(int _type)
{
    this->_type = _type;

    this->num_sliders = 2;
    //this->menu_scale = .25f;

    this->set_material(&m_edev);

    float qw, qh;

    switch (this->_type) {
        case GRAVITY_MANAGER:
            this->set_mesh(mesh_factory::get_mesh(MODEL_GRAVITYMAN));
            this->set_num_properties(2);
            this->properties[0].type = P_FLT; /* Fallback angle */
            this->properties[0].v.f = (3.f*M_PI)/2.f;

            this->properties[1].type = P_INT; /* Fallback force */
            this->properties[1].v.i = 20;

            this->num_s_out = 0;
            this->num_s_in = 3;

            this->s_in[0].lpos = b2Vec2(-.225f, 0.f); /* Angle */
            this->s_in[1].lpos = b2Vec2(0.f,  -0.000001f); /* Force */
            this->s_in[2].lpos = b2Vec2(.25f, .0f);  /* Active (+ modifier) */
            this->s_in[0].tag = SOCK_TAG_ANGLE;
            this->s_in[1].tag = SOCK_TAG_FORCE;
            this->s_in[2].tag = SOCK_TAG_MULTIPLIER;

            this->set_as_rect(.375f, .25f);
            break;

        case GRAVITY_SETTER:
            this->set_mesh(mesh_factory::get_mesh(MODEL_GRAVITYSET));
            this->set_num_properties(2);
            this->properties[0].type = P_FLT; /* Gravity X */
            this->properties[0].v.f = 0.f;

            this->properties[1].type = P_FLT; /* Gravity Y */
            this->properties[1].v.f = 20.f;

            this->num_s_out = 0;
            this->num_s_in = 1;

            this->s_in[0].lpos = b2Vec2(0.f, -.125f); /* Active */
            this->s_in[0].tag = SOCK_TAG_MULTIPLIER;

            this->set_as_rect(.125f, .25f);
            break;
    }
}

float
gravityman::get_slider_snap(int s)
{
    switch (this->_type) {
        case GRAVITY_MANAGER:
            switch (s) {
                case 0: return 1.f / 24.f;
                case 1: return 1.f / 20.f;
            }
            break;

        case GRAVITY_SETTER:
            switch (s) {
                case 0: return 1.f / 40.f;
                case 1: return 1.f / 40.f;
            }
            break;
    }

    return 0.f;
}

float
gravityman::get_slider_value(int s)
{
    switch (this->_type) {
        case GRAVITY_MANAGER:
            switch (s) {
                case 0: return (this->properties[0].v.f) / (2.f * M_PI);
                case 1: return (this->properties[1].v.i / 5) / 20.f;
            }
            break;

        case GRAVITY_SETTER:
            switch (s) {
                case 0: return ((this->properties[0].v.f + 100.f) / 10.f) / 20.f;
                case 1: return ((this->properties[1].v.f + 100.f) / 10.f) / 20.f;
            }
            break;
    }

    return 0.f;
}

void
gravityman::on_slider_change(int s, float value)
{
    if (this->_type == GRAVITY_MANAGER) {
        if (s == 0) {
            this->set_property(0, (float)(value * (2.f*M_PI)));
            G->show_numfeed(this->properties[0].v.f);
        } else if (s == 1) {
            this->set_property(1, (uint32_t)((value * 5) * 20.f));
            G->show_numfeed(this->properties[1].v.i);
        }
    } else if (this->_type == GRAVITY_SETTER) {
        if (s == 0 || s == 1) {
            this->set_property(s, (value * 10.f * 20.f) - 100.f);
            G->show_numfeed(this->properties[s].v.f);
        }
    }
}

edevice*
gravityman::solve_electronics()
{
    switch (this->_type) {
        case GRAVITY_MANAGER:
            {
                if (!this->s_in[0].is_ready())
                    return this->s_in[0].get_connected_edevice();

                if (!this->s_in[1].is_ready())
                    return this->s_in[1].get_connected_edevice();

                if (!this->s_in[2].is_ready())
                    return this->s_in[2].get_connected_edevice();

                float angle = 0.f;
                int force = 0;
                float mul = 0.f;

                if (this->s_in[0].p == 0) {
                    /* angle input is unplugged, set to fallback value */
                    angle = this->properties[0].v.f;
                } else {
                    angle = this->s_in[0].get_value() * (2.f * M_PI);
                }

                if (this->s_in[1].p == 0) {
                    /* force input is unplugged, set to fallback value */
                    force = this->properties[1].v.i;
                } else {
                    force = (this->s_in[1].get_value() * 5) * 20;
                }

                if (this->s_in[2].p == 0) {
                    /* multiplier input is unplugged, set to fallback value */
                    mul = 1.f;
                } else {
                    mul = this->s_in[2].get_value();
                }

                if (mul > 0.f) {
                    float ax = cos(angle);
                    float ay = sin(angle);

                    W->add_gravity_force(this->id, b2Vec2((force * ax) * mul, (force * ay) * mul));
                } else {
                    W->remove_gravity_force(this->id);
                }
            }
            break;

        case GRAVITY_SETTER:
            if (!this->s_in[0].is_ready())
                return this->s_in[0].get_connected_edevice();

            float mul = this->s_in[0].get_value();
            if (mul > 0.f) {
                W->set_gravity(this->properties[0].v.f * mul, this->properties[1].v.f * mul);
            }

            break;
    }

    return 0;
}

localgravity::localgravity()
    : mul(0.f)
{
    this->set_flag(ENTITY_DO_STEP, false); /* XXX do NOT add ourselves to stepable, world handles gravity objects separately, step is still called though */
    this->set_flag(ENTITY_IS_BETA, true);
    if (W->level.version >= LEVEL_VERSION_1_5) {
        this->set_flag(ENTITY_ALLOW_AXIS_ROT, true);
    }

    this->set_material(&m_pv_colored);
    this->set_uniform("~color", .7f, .35f, .35f, 1.f);

    this->set_num_properties(1);

    this->properties[0].type = P_FLT;
    this->properties[0].v.f = 1.f;

    this->num_sliders = 1;

    if (W->level.version < LEVEL_VERSION_1_5) {
        this->set_as_rect(.25f, .25f);
    }

}

float
localgravity::get_slider_snap(int s)
{
    return .05f;
}

float
localgravity::get_slider_value(int s)
{
    return this->properties[0].v.f / LOCALGRAVITY_MAX_MASS;
}

void
localgravity::step()
{
    b2Body *b = 0, *next;
    b2Vec2 p = this->get_position();
    float m1, m2;

    //m1 = this->body->GetMass();
    m1 = this->properties[0].v.f * this->mul;

    if (m1 > 0.0001) {
        for (b = W->b2->GetBodyList(); b; b = b->GetNext()) {
            if (b == this->get_body(0) || b->GetType() != b2_dynamicBody)
                continue;

            b2Vec2 v = b->GetWorldCenter() - p;

            float ll = v.LengthSquared();

            ll = fmaxf(ll, 1.f);

            v.Normalize();

            m2 = b->GetMass();

            b2Vec2 f = -((m1*m2) / ll) * v;

            entity *e = b->GetFixtureList() ? static_cast<entity*>(b->GetFixtureList()->GetUserData()) : 0;

            if (e && e->flag_active(ENTITY_IS_CREATURE)) {
                /* creatures keep track of all gravity forces */
                static_cast<creature*>(e)->gravity_forces += f;
            }

            b->ApplyForceToCenter(f);

            //f = ((m2*this->get_body(0)->GetMass()) / ll) * v;
            f = -f;
            this->get_body(0)->ApplyForceToCenter(f);
        }
    }
}

void
localgravity::on_slider_change(int s, float value)
{
    this->properties[0].v.f = value*LOCALGRAVITY_MAX_MASS;
    G->show_numfeed(this->properties[0].v.f);
}

edevice*
localgravity::solve_electronics()
{
    if (!this->s_in[0].is_ready()) {
        return this->s_in[0].get_connected_edevice();
    }

    if (this->s_in[0].p) {
        this->mul = this->s_in[0].get_value();
    } else {
        this->mul = 1.f;
    }

    return 0;
}

struct tms_sprite*
localgravity::get_axis_rot_sprite()
{
    if (this->flag_active(ENTITY_AXIS_ROT)) {
        return gui_spritesheet::get_sprite(S_WIP);
    } else {
        return gui_spritesheet::get_sprite(S_WIP_2);
    }
}

const char*
localgravity::get_axis_rot_tooltip()
{
    if (this->flag_active(ENTITY_AXIS_ROT)) {
        return "Make dynamic";
    } else {
        return "Make static";
    }
}

void
localgravity::toggle_axis_rot()
{
    this->set_flag(ENTITY_AXIS_ROT, !this->flag_active(ENTITY_AXIS_ROT));

    this->body->SetType(this->get_dynamic_type());
}

b2BodyType
localgravity::get_dynamic_type()
{
    if (this->flag_active(ENTITY_AXIS_ROT)) {
        return b2_staticBody;
    }

    return b2_dynamicBody;
}
