#include "rocket.hh"
#include "model.hh"
#include "world.hh"
#include "game.hh"
#include "explosive.hh"
#include "fxemitter.hh"

#define ROCKET_THRUST_MUL 4.f

#define ROCKET_TYPE_THRUSTER 0
#define ROCKET_TYPE_ROCKET   1

rocket::rocket(int size)
{
    this->set_flag(ENTITY_DO_STEP, true);
    this->set_flag(ENTITY_DO_UPDATE_EFFECTS, true);
    this->set_flag(ENTITY_IS_MAGNETIC, true);

    if (size == 0) {
        this->set_mesh(mesh_factory::get_mesh(MODEL_THRUSTER));
    } else {
        this->set_mesh(mesh_factory::get_mesh(MODEL_ROCKET));
        this->menu_scale = .75f;
    }
    this->set_material(&m_rocket);

    this->rtype = size;

    this->num_sliders = 1;

    tmat4_load_identity(this->M);
    tmat3_load_identity(this->N);

    this->num_s_in = 1;

    this->set_num_properties(1);
    this->set_property(0, 12.f);

    if (size == 0)
        this->s_in[0].lpos = b2Vec2(0.f,0.2f);
    else
        this->s_in[0].lpos = b2Vec2(0.f,0.6f);

    this->s_in[0].ctype = CABLE_RED;

    this->layer_mask = 14; /* sublayer 2, 3, 4 */

    if (size == 0) {
        this->set_as_rect(.25f/2.f, .75f/2.f);
    } else {
        this->set_as_rect(.5f/2.f, 1.688f/2.f);
    }

    this->query_sides[2].SetZero(); /* down */

    this->flames = 0;
}

void
rocket::on_pause()
{
    this->flames = 0;

    this->set_thrustmul(0.f);
}

void
rocket::on_absorb()
{
    if (this->flames) {
        this->flames->done = true;
    }
}

void
rocket::setup()
{
    this->set_thrustmul(0.f);
}

float
rocket::get_slider_snap(int s)
{
    return .05f;
}

float
rocket::get_slider_value(int s)
{
    return this->properties[0].v.f / 40.f;
}

void
rocket::on_slider_change(int s, float value)
{
    this->properties[0].v.f = value * 40.f;
    float factor = (this->rtype == 0 ? 1.f : ROCKET_THRUST_MUL);
    G->show_numfeed(this->properties[0].v.f*factor);
}

void
rocket::step()
{
    if (this->thrustmul > 0.f) {
        if (!this->flames) {
            this->flames = new flame_effect(this->_pos, this->get_layer(), this->rtype);
            tms_debugf("rocket %p flame effect: %p", this, this->flames);
            this->flames->set_thrustmul(0.f);
            if (this->rtype == ROCKET_TYPE_THRUSTER) {
                this->flames->set_z_offset(.1f);
            } else {
                this->flames->set_z_offset(.08f);
            }
        }

        G->emit(flames, this, b2Vec2(0.f, 0.f));

        b2Vec2 centroid = this->get_position();
        b2Body *b = this->get_body(0);

        float bangle = this->get_angle() + M_PI/2.f;

        b2Vec2 force;
        tmath_sincos(bangle, &force.y, &force.x);

        float factor = (this->rtype == 0 ? 1.f : ROCKET_THRUST_MUL) * thrustmul * 4.f;

        force.x *= this->properties[0].v.f * factor;
        force.y *= this->properties[0].v.f * factor;

        b->ApplyForce(force, centroid);

        b2Vec2 p;
        if (this->rtype == 0) {
            p = this->local_to_world(b2Vec2(0.f, -.4f), 0);
            p.x += -.01f + ((rand()%100) / 100.f) * .02f;
            p.y += -.01f + ((rand()%100) / 100.f) * .02f;
        } else {
            p = this->local_to_world(b2Vec2(0.f, -1.05f), 0);
            p.x += -.1f + ((rand()%100) / 100.f) * .2f;
            p.y += -.1f + ((rand()%100) / 100.f) * .2f;
        }

        b2Vec2 v = b->GetLinearVelocity();//.Length() * b->GetWorldVector(b2Vec2(0.f, -1.f));

        this->flames->update_pos(p, v);

        b2AABB aabb;
        b2Vec2 origo = this->local_to_world(b2Vec2(0.f, -2.f), 0);
        aabb.lowerBound.Set(-.25f + origo.x, -.25f + origo.y);
        aabb.upperBound.Set(.25f + origo.x, .25f + origo.y);
        W->b2->QueryAABB(this, aabb);
    }

    if (this->flames) {
        this->flames->set_thrustmul(this->thrustmul);
    }
}

bool
rocket::ReportFixture(b2Fixture *f)
{
    entity *e = (entity*)f->GetUserData();

    if (!e) {
        return true;
    }

    if (W->level.version < LEVEL_VERSION_1_5) {
        if (e->g_id == O_BOMB) {
            ((explosive*)e)->triggered = true;
        }
    } else {
        if (!world::fixture_in_layer(f, this->get_layer())) {
            return true;
        }

        if (W->level.flag_active(LVL_DISABLE_ROCKET_TRIGGER_EXPLOSIVES)) {
            return true;
        }

        if (e->is_explosive()) {
            ((explosive*)e)->triggered = true;
        }
    }

    return true;
}

edevice*
rocket::solve_electronics(void)
{
    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();

    if (this->s_in[0].p == 0) {
        /* cable is unplugged, default to ACTIVE */
        if (W->level.version >= 25 && W->level.type == LCAT_ADVENTURE)
            this->set_thrustmul(0.f);
        else
            this->set_thrustmul(1.f);
    } else
        this->set_thrustmul(this->s_in[0].get_value());

    return 0;
}

void
rocket::set_thrustmul(float thrustmul)
{
    this->thrustmul = thrustmul;
}
