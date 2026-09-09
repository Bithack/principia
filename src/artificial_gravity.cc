#include "artificial_gravity.hh"
#include "creature.hh"
#include "game.hh"
#include "gravityman.hh"
#include "gui.hh"
#include "world.hh"

artificialgravity::artificialgravity() : mul(0.f) {
    this->set_flag(ENTITY_DO_STEP, false); /* XXX do NOT add ourselves to stepable, world handles gravity objects separately, step is still called though */
    this->set_flag(ENTITY_IS_BETA, true);
    if (W->level.version >= LEVEL_VERSION_1_5)
        this->set_flag(ENTITY_ALLOW_AXIS_ROT, true);

    this->set_material(&m_pv_colored);
    this->set_uniform("~color", .7f, .35f, .35f, 1.f);

    this->set_num_properties(1);

    this->properties[0].type = P_FLT;
    this->properties[0].v.f = 1.f;

    this->num_sliders = 1;

    if (W->level.version < LEVEL_VERSION_1_5)
        this->set_as_rect(.25f, .25f);
}

float artificialgravity::get_slider_snap(int s) {
    return .05f;
}

float artificialgravity::get_slider_value(int s) {
    return this->properties[0].v.f / LOCALGRAVITY_MAX_MASS;
}

void artificialgravity::step() {
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

void artificialgravity::on_slider_change(int s, float value) {
    this->properties[0].v.f = value*LOCALGRAVITY_MAX_MASS;
    G->show_numfeed(this->properties[0].v.f);
}

edevice *artificialgravity::solve_electronics() {
    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();

    if (this->s_in[0].p)
        this->mul = this->s_in[0].get_value();
    else
        this->mul = 1.f;

    return 0;
}

struct tms_sprite *artificialgravity::get_axis_rot_sprite() {
    if (this->flag_active(ENTITY_AXIS_ROT))
        return gui_spritesheet::get_sprite(S_WIP);
    else
        return gui_spritesheet::get_sprite(S_WIP_2);
}

const char *artificialgravity::get_axis_rot_tooltip() {
    if (this->flag_active(ENTITY_AXIS_ROT))
        return "Make dynamic";
    else
        return "Make static";
}

void artificialgravity::toggle_axis_rot() {
    this->set_flag(ENTITY_AXIS_ROT, !this->flag_active(ENTITY_AXIS_ROT));

    this->body->SetType(this->get_dynamic_type());
}

b2BodyType artificialgravity::get_dynamic_type() {
    if (this->flag_active(ENTITY_AXIS_ROT))
        return b2_staticBody;

    return b2_dynamicBody;
}
