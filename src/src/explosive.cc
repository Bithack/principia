#include "explosive.hh"
#include "model.hh"
#include "world.hh"
#include "game.hh"
#include "fxemitter.hh"
#include "game.hh"

#define FORCE 300.f

explosive::explosive(int explosive_type)
{
    this->set_flag(ENTITY_ALLOW_CONNECTIONS,    false);
    this->set_flag(ENTITY_FADE_ON_ABSORB,       false);
    this->set_flag(ENTITY_DO_PRE_STEP,          true);
    this->set_flag(ENTITY_DO_STEP,              false);
    this->set_flag(ENTITY_IS_EXPLOSIVE,         true);

    this->hp = EXPLOSIVE_MAX_HP;
    this->explosive_type = explosive_type;
    this->width = .5f;
    this->set_uniform("~color", .2f, .2f, .2f, 1.f);
    this->layer_mask = 6;
    this->num_sliders = 2;

    this->set_num_properties(3);

    switch (this->explosive_type) {
        case EXPLOSIVE_BOMB:
            this->set_mesh(mesh_factory::get_mesh(MODEL_SPHERE));
            this->set_material(&m_colored);
            this->set_property(0, (uint32_t)5000); /* Fuse Timer in milliseconds */
            break;

        case EXPLOSIVE_LANDMINE:
            this->set_mesh(mesh_factory::get_mesh(MODEL_LANDMINE));
            this->set_material(&m_colored);
            this->properties[0].type = P_FLT;
            this->set_property(0, .5f); /* threshold */
            break;

        case EXPLOSIVE_TRIGGER:
            this->set_mesh(mesh_factory::get_mesh(MODEL_BOMB));
            this->set_material(&m_colored);
            this->properties[0].type = P_FLT;
            this->set_property(0, .0f); /* no functionality */
            break;
    }

    this->properties[1].type = P_FLT;
    this->properties[1].v.f = 1.f; // damage multiplier

    this->properties[2].type = P_FLT; /* version 1.5 */
    this->properties[2].v.f = .35f; // force multiplier

    this->time = 0;
    this->triggered = false;
    this->hl_time = 0.f;
}

void
explosive::init()
{
    this->trigger_time = ((uint64_t)this->properties[0].v.i)*1000llu;
}

void
explosive::setup()
{
    this->time = 0;
    this->hp = EXPLOSIVE_MAX_HP;
}

void
explosive::pre_step()
{
    switch (this->explosive_type) {
        case EXPLOSIVE_BOMB:
            this->time += G->timemul(WORLD_STEP);
            this->hl_time += G->get_time_mul() * WORLD_STEP * .00001f * ((double)this->time/(double)this->trigger_time);

            float hl = 1.f-powf(fmodf(this->hl_time, 1.f), 1.f/4.f);
            hl *= .5f;

            this->set_uniform("~color", .2f+hl, .2f+hl, .2f+hl, 1.f);

            if (this->time >= this->trigger_time) {
                tms_debugf("Triggering explosive. %" PRIu64, this->time);
                this->trigger();
            }
            break;
    }

    if (this->triggered || this->hp <= 0.f) /* external triggering (other bombs, rockets, hp reduced below 0) */
        this->trigger();
}

void
explosive::add_to_world()
{
    if (this->explosive_type == EXPLOSIVE_BOMB) {
        this->create_circle(this->get_dynamic_type(), .25f, this->material);
    } else if (this->explosive_type == EXPLOSIVE_TRIGGER) {
        this->create_circle(this->get_dynamic_type(), .1f, this->material);

        this->get_body(0)->SetAngularDamping(6.f);
    } else {
        b2BodyDef bd;
        bd.type = this->get_dynamic_type();
        bd.position = this->_pos;
        bd.angle = this->_angle;
        b2Body *b = W->b2->CreateBody(&bd);
        this->body = b;

        b2Vec2 verts[5] = {
            b2Vec2(0.f, 0.934f * .332f),
            b2Vec2(-.577f * .346f, .577f * .332f),
            b2Vec2(-.577f * .346f, -.577f * .332f),
            b2Vec2(.577f * .346f, -.577f * .332f),
            b2Vec2(.577f * .346f, .577f * .332f),
        };

        b2PolygonShape box;
        box.Set(verts, 5);

        b2FixtureDef fd;
        fd.shape = &box;
        fd.density = this->get_material()->density;
        fd.friction = this->get_material()->friction;
        fd.restitution = this->get_material()->restitution;
        fd.filter = world::get_filter_for_layer(this->prio, this->layer_mask);

        (b->CreateFixture(&fd))->SetUserData(this);
    }
}

bool
explosive::ReportFixture(b2Fixture *f)
{
    if (f->IsSensor()) {
        return true;
    }

    entity *e = static_cast<entity*>(f->GetUserData());

    if (e && e->g_id == O_BOMB) {
        explosive *ex = (explosive*)e;
        ex->time = ex->trigger_time-((uint64_t)rand())%120000llu;
    }

    return true;
}

void
explosive::trigger()
{
    /* trigger nearby bombs */
    b2AABB aabb;
    b2Vec2 origo = this->get_position();
    aabb.lowerBound.Set(-2.f + origo.x, -2.f + origo.y);
    aabb.upperBound.Set(2.f + origo.x, 2.f + origo.y);
    W->b2->QueryAABB(this, aabb);

    this->time = 0llu;
    b2Vec2 trigger_point = this->get_position();
    b2Vec2 p = this->get_position();

    W->explode(this, trigger_point, this->get_layer(), 20,
            (W->level.version >= LEVEL_VERSION_1_5 ? FORCE*this->properties[2].v.f : FORCE),
            this->properties[1].v.f,
            (W->level.version >= LEVEL_VERSION_1_5 ? .5f : 1.f)
            );
    G->absorb(this);

    bool with_debris = (this->emit_step == 0); /* disable debris for emitted explosives */
    G->emit(new explosion_effect(p, this->get_layer(), with_debris), 0);
}

float
explosive::get_slider_snap(int s)
{
    if (s == 0) {
        if (this->explosive_type == EXPLOSIVE_BOMB) {
            return 1.f / 49.f;
        } else {
            return 0.025f;
        }
    } else {
        return 1.f / 59.f;
    }
}

float
explosive::get_slider_value(int s)
{
    if (s == 0) {
        if (this->explosive_type == EXPLOSIVE_BOMB) {
            float v = ((float)this->properties[0].v.i / 1000.f) - 1.f;

            return v / 49.f;
        } else
            return this->properties[0].v.f / 10.f;
    } else {
        return this->properties[1].v.f / 3.f;
    }
}

void
explosive::on_slider_change(int s, float value)
{
    if (s == 0) {
        if (this->explosive_type == EXPLOSIVE_BOMB) {
            uint32_t fuse_timer = (uint32_t)((1.f + (value * 49.f)) * 1000.f);

            this->set_property(0, fuse_timer);
            G->show_numfeed((float)fuse_timer / 1000.f);
        } else {
            this->set_property(0, value*10.f);
            G->show_numfeed(value*10.f);
        }
    } else {
        this->properties[1].v.f = (value * 2.95f) + 0.05f;
        G->show_numfeed(this->properties[1].v.f);
    }
}
