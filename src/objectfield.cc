#include "objectfield.hh"
#include "material.hh"
#include "model.hh"
#include "game.hh"

objectfield::objectfield(int _object_type)
    : object_type(_object_type)
{
    this->set_flag(ENTITY_HAS_TRACKER, true);

    this->num_sliders = 2;
    this->menu_scale = .25f;

    this->set_mesh(mesh_factory::get_mesh(MODEL_FIELD3));
    this->set_material(&m_pv_colored);

    this->set_num_properties(3);
    this->set_property(0, (uint32_t)3); /* length */
    this->set_property(1, (uint32_t)2); /* sensor height */

    switch (this->object_type) {
        case OBJECT_FIELD_ID:
            this->properties[2].type = P_INT; /* Unique object ID */
            this->properties[2].v.i = 0;
            this->set_uniform("~color", .7f, .2f, .2f, 1.f);
            break;

        case OBJECT_FIELD_OBJECT:
            this->properties[2].type = P_INT; /* Object type ID */
            this->properties[2].v.i = 0;
            this->set_uniform("~color", .2f, .7f, .2f, 1.f);
            break;

        case OBJECT_FIELD_TARGET_SETTER:
            this->properties[2].type = P_INT; /* Unique object ID */
            this->properties[2].v.i = 0;
            this->set_uniform("~color", .2f, .2f, .7f, 1.f);
            break;
    }

    tmat4_load_identity(this->M);
    tmat3_load_identity(this->N);

    this->layer_mask = 6;

    this->num_s_in = 0;
    this->num_s_out = 1;

    this->s_out[0].lpos = b2Vec2(.25f, -.025f);

    this->counter = 0;
}

float
objectfield::get_slider_value(int s)
{
    if (s == 0) /* length */
        return this->properties[0].v.i / 3.f;
    else if (s == 1) /* sensor height */
        return this->properties[1].v.i / 9.f;

    tms_fatalf("unknown slider value in objectfield");
    return .0f;
}

float
objectfield::get_slider_snap(int s)
{
    if (s == 0) /* length */
        return 1.f / 3.f;
    else /* sensor height */
        return 1.f / 9.f;
}

const char*
objectfield::get_slider_label(int s)
{
    if (s == 0) /* length */
        return "Length";
    else if (s == 1) /* sensor height */
        return "Sensor Height";

    tms_fatalf("unknown slider value in objectfield");
    return 0;
}

void
objectfield::add_to_world()
{
    uint32_t length = (uint32_t)roundf(this->properties[0].v.i);
    uint32_t sensor_height = (uint32_t)roundf(this->properties[1].v.i);
    this->set_size(length, sensor_height, false);

    if (!this->body) {
        b2BodyDef bd;
        bd.type = this->get_dynamic_type();
        bd.position = _pos;
        bd.angle = _angle;
        b2Body *b = W->b2->CreateBody(&bd);
        this->body = b;
    } else {
        while (this->body->GetFixtureList()) {
            this->body->DestroyFixture(this->body->GetFixtureList());
        }
    }

    b2FixtureDef fd;
    fd.shape = &this->box_shape;
    fd.density = 1.f;
    fd.friction = .5f;
    fd.restitution = .3f;
    fd.filter = world::get_filter_for_layer(this->get_layer(), 6);

    (this->body->CreateFixture(&fd))->SetUserData(this);

    if (!W->is_paused()) {
        b2FixtureDef sfd;
        sfd.shape = &this->sensor_shape;
        sfd.density = .0f;
        sfd.isSensor = true;
        sfd.filter = world::get_filter_for_layer(this->get_layer(), 15);

        (this->body->CreateFixture(&sfd))->SetUserData(this);
    }
}

void
objectfield::remove_from_world()
{
    if (this->body) {
        W->b2->DestroyBody(this->body);
        this->body = 0;
    }
}

void
objectfield::on_load(bool created, bool has_state)
{
    this->counter = 0;
    this->set_size(this->properties[0].v.i, this->properties[1].v.i, false);
}

void
objectfield::setup()
{
    this->counter = 0;
}

void
objectfield::on_pause()
{
    this->counter = 0;
}

void
objectfield::on_touch(b2Fixture *my, b2Fixture *other)
{
    entity *e;
    if ((e = static_cast<entity*>(other->GetUserData())) && (!other->IsSensor() || e->g_id == O_INTERACTIVE_BALL || e->g_id == O_BALL || e->g_id == O_METAL_BALL)) {
        switch (this->object_type) {
            case OBJECT_FIELD_ID:
                if (this->properties[2].v.i == e->id) {
#ifdef DEBUG
                    other->m_isHighlighted = true;
#endif
                    this->counter ++;
                }
                break;

            case OBJECT_FIELD_TARGET_SETTER:
                // the target setter will light up any time we give a command
                if (e->flag_active(ENTITY_IS_CREATURE)) {
                    creature *c = static_cast<creature*>(e);
                    c->roam_target_id = this->properties[2].v.i;
                    this->counter ++;
                }
                break;

            case OBJECT_FIELD_OBJECT:
                if (this->properties[2].v.i == e->g_id) {
#ifdef DEBUG
                    other->m_isHighlighted = true;
#endif
                    this->counter ++;
                }
                break;
        }
    }
}

void
objectfield::on_untouch(b2Fixture *my, b2Fixture *other)
{
    if (this->object_type == OBJECT_FIELD_TARGET_SETTER) return;

    entity *e;
    if ((e = static_cast<entity*>(other->GetUserData())) && (!other->IsSensor() || e->g_id == O_INTERACTIVE_BALL || e->g_id == O_BALL || e->g_id == O_METAL_BALL)) {
        switch (this->object_type) {
            case OBJECT_FIELD_ID:
                if (this->properties[2].v.i == e->id) {
#ifdef DEBUG
                    other->m_isHighlighted = false;
#endif
                    this->counter --;
                }
                break;

            case OBJECT_FIELD_OBJECT:
                if (this->properties[2].v.i == e->g_id) {
#ifdef DEBUG
                    other->m_isHighlighted = false;
#endif
                    this->counter --;
                }
                break;
        }
    }

    if (this->counter < 0) {
        tms_errorf("!!! field counter below 0");
        this->counter = 0;
    }
}

void
objectfield::set_size(int length, int sensor_height, bool add_to_world)
{
    if (length>3) length = 3;
    if (length<0) length = 0;

    this->width = ((float)length+1.f)/2.f;

    if (sensor_height>9) sensor_height = 9;
    if (sensor_height<0) sensor_height = 0;

    this->set_property(0, (uint32_t)length);
    this->set_property(1, (uint32_t)sensor_height);

    this->set_mesh(mesh_factory::get_mesh(MODEL_FIELD0+length));

    this->box_shape.SetAsBox(((float)length+1.f)/2.f, .15f);
    this->sensor_shape.SetAsBox(((float)length+1.f)/2.f, ((float)sensor_height+1.f)/2.f, b2Vec2(0.f, ((float)sensor_height+1.f)/2.f), 0.f);

    this->width = ((float)length+1.f)/2.f;
    this->height = .15f;
    float qw = this->width/2.f + 0.15f;
    float qh = this->height/2.f + 0.15f;
    this->query_sides[0].Set(0.f,  qh); /* up */
    this->query_sides[1].Set(-qw, 0.f); /* left */
    this->query_sides[2].Set(0.f, -qh); /* down */
    this->query_sides[3].Set( qw, 0.f); /* right */

    if (add_to_world) {
        if (this->body) {
            this->body->DestroyFixture(this->body->GetFixtureList());
        }

        this->add_to_world();
    }
}

void
objectfield::on_slider_change(int s, float value)
{
    if (s == 0) { /* length */
        uint32_t length = (uint32_t)roundf(value * 3.f);
        uint32_t sensor_height = (uint32_t)roundf(this->properties[1].v.i);

        G->animate_disconnect(this);
        this->disconnect_all();
        G->show_numfeed(1.f + length);

        this->set_size(length, sensor_height, true);
    } else if (s == 1) { /* sensor height */
        uint32_t length = (uint32_t)roundf(this->properties[0].v.i);
        uint32_t sensor_height = (uint32_t)roundf(value * 9.f);

        G->show_numfeed(1.f + sensor_height);

        this->set_size(length, sensor_height, true);
    }
}

edevice*
objectfield::solve_electronics()
{
    this->s_out[0].write((this->counter > 0) ? 1.f : 0.f);

    if (this->object_type == OBJECT_FIELD_TARGET_SETTER) {
        this->counter = 0;
    }

    /* always done immediately */
    return 0;
}
