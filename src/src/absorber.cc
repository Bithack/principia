#include "absorber.hh"
#include "material.hh"
#include "model.hh"
#include "game.hh"
#include "adventure.hh"

#include "object_factory.hh"

absorber::absorber(int size)
{
    this->set_flag(ENTITY_DO_STEP, true);
    this->set_flag(ENTITY_HAS_TRACKER, true);

    this->size = size;
    this->absorbed = false;
    this->state = 0;
    this->absorb_interval = 0;

    if (this->size == 0) {
        this->absorber_w = .375f;
        if (W->level.version >= LEVEL_VERSION_1_5) {
            this->absorber_h = .375f;
        } else {
            this->absorber_h = .475f;
        }

        const float qw = this->absorber_w/2.f+0.15f;
        const float qh = this->absorber_h/2.f+0.55f;

        this->query_sides[0].Set(0.f,  qh);
        this->query_sides[1].Set(-qw, 0.f);
        this->query_sides[3].Set( qw, 0.f);

        this->frame_w   = absorber_w;
        this->frame_h   = 0.1f;
        this->set_mesh(mesh_factory::get_mesh(MODEL_MINIEMITTER));
        this->update_method = ENTITY_UPDATE_FASTBODY;

        this->width = this->absorber_w;
        this->height = this->absorber_h + this->frame_h + 0.1f;
    } else {
        this->absorber_w = 2.2f /2.f;
        this->absorber_h = 2.2f/2.f;

        this->set_flag(ENTITY_IS_STATIC, true);
        this->set_flag(ENTITY_IS_LOW_PRIO, true);
        this->set_flag(ENTITY_ALLOW_CONNECTIONS, false);
        this->frame_w   = absorber_w;
        this->frame_h   = 0.1f;
        this->set_mesh(mesh_factory::get_mesh(MODEL_EMITTER));
        this->update_method = ENTITY_UPDATE_STATIC;

        this->width = this->absorber_w;
        this->height = this->absorber_h + .15f;
    }

    this->num_sliders = 1;

    this->set_material(&m_pv_colored);

    this->set_uniform("~color", .7f, .3f, .5f, 1.f);

    this->set_num_properties(2);
    this->set_property(0, (uint32_t)500); /* Absorb interval in milliseconds */
    this->set_property(1, (uint32_t)0);   /* g_id of object to absorb */

    this->num_s_in = 1;
    this->num_s_out = 1;

    this->s_out[0].lpos = b2Vec2(-.125f, +this->absorber_h+this->frame_h);
    this->s_in[0].lpos  = b2Vec2( .125f, +this->absorber_h+this->frame_h);

    this->scaleselect = true;
    this->menu_scale = .5f;

    if (this->size == 1) {
        struct tms_entity *e = tms_entity_alloc();
        //tms_entity_set_mesh(e, tms_meshfactory_get_cube());
        tms_entity_set_mesh(e, const_cast<tms_mesh*>(tms_meshfactory_get_cube()));
        tms_entity_set_uniform4f(e, "~color", 1.f, 1.f, 1.f, 0.5f);
        tms_entity_set_material(e, &m_field);
        tms_entity_add_child(this, e);
        this->set_flag(ENTITY_DO_UPDATE_EFFECTS, true);

        tmat3_load_identity(e->N);

        this->field = e;
    }
}

void
absorber::add_to_world()
{
    b2BodyDef bd;
    bd.type = (this->size == 0 ? this->get_dynamic_type() : b2_staticBody);
    bd.position = _pos;
    bd.angle = _angle;
    b2Body *b = W->b2->CreateBody(&bd);
    this->body = b;

    if (this->size == 1) {
        /* big emitter */
        b2PolygonShape box;
        box.SetAsBox(this->absorber_w, this->absorber_h);

        b2FixtureDef fd;
        fd.shape = &box;
        fd.density = 1.f;
        fd.friction = .5f;
        fd.restitution = .3f;
        fd.isSensor = true;

        b2PolygonShape frame_shape;
        frame_shape.SetAsBox(this->frame_w, this->frame_h, b2Vec2(0.f, +this->absorber_h+this->frame_h), 0.f);

        b2FixtureDef frame_fd;
        frame_fd.shape = &frame_shape;
        frame_fd.density = 1.0f;
        frame_fd.friction = .5f;
        frame_fd.restitution = .3f;
        frame_fd.filter = world::get_filter_for_layer(this->get_layer(), 15);

        (this->body->CreateFixture(&frame_fd))->SetUserData(this);
        (this->body->CreateFixture(&fd))->SetUserData(this);

        this->field_life = 0.f;
    } else {
        /* mini emitter */

        b2PolygonShape sh;
        b2FixtureDef fd;
        fd.shape = &sh;
        fd.density = this->get_material()->density;
        fd.friction = this->get_material()->friction;
        fd.restitution = this->get_material()->restitution;
        fd.isSensor = false;
        fd.filter = world::get_filter_for_layer(this->get_layer(), 15);

        /* top */
        sh.SetAsBox(this->frame_w+.05f, this->frame_h, b2Vec2(0.f, +this->absorber_h+this->frame_h), 0.f);
        (this->body->CreateFixture(&fd))->SetUserData(this);

        /* left */
        sh.SetAsBox(.05f, this->absorber_h, b2Vec2(-.365f, 0.f), 0.f);
        (this->body->CreateFixture(&fd))->SetUserData(this);

        /* right */
        sh.SetAsBox(.05f, this->absorber_h, b2Vec2(.365f, 0.f), 0.f);
        (this->body->CreateFixture(&fd))->SetUserData(this);
    }

    if (!W->is_paused()) {
        b2PolygonShape sensor_shape;
        if (this->size == 0) {
            sensor_shape.SetAsBox(0.25f, 0.25f);
        } else {
            sensor_shape.SetAsBox(this->absorber_w, this->absorber_h);
        }

        b2FixtureDef sfd;
        sfd.shape = &sensor_shape;
        sfd.density = .0f;
        sfd.isSensor = true;
        sfd.filter = world::get_filter_for_layer(this->get_layer(), 15);

        (this->body->CreateFixture(&sfd))->SetUserData(this);
    }
}

void
absorber::setup()
{
    this->state = 0;
    this->time = absorb_interval;
    this->do_accumulate = false;
    this->absorbed = false;
}

void
absorber::init()
{
    this->absorb_interval = (uint64_t)(roundf(((double)this->properties[0].v.f)*1000000.));
    this->pending_fixtures.clear();
}

void
absorber::step()
{
    int g_id = this->properties[1].v.i;

    this->time += G->timemul(WORLD_STEP);

    if (!this->do_accumulate && this->time >= this->absorb_interval) {
        this->time = this->absorb_interval;
    }

    switch (this->state) {
        case 0:
            /* waiting for signal */
            break;

        case 1:
            /* check cooldown timer */
            if (this->time >= this->absorb_interval) {
                this->time -= this->absorb_interval;
                this->state = 2;
            } else {
                this->state = 0;
            }
            break;

        case 2:
            /* do we have any entity pending removal? */
            if (pending_fixtures.size() > 0) {
                entity *e = static_cast<entity*>(pending_fixtures.front()->GetUserData());
                if ((g_id == O_PLANK || e->g_id == g_id) && G->absorb(e)) {
                    W->events[WORLD_EVENT_ABSORB] ++;

                    G->play_sound(SND_ABSORB, this->get_position().x, this->get_position().y, rand(), 1.f);
                    //this->field_life = 1.f;
                    this->absorbed = true;
                    this->state = 0;
                } else {
                    this->pending_fixtures.remove(pending_fixtures.front());
                }
            }

            break;

        default:
            this->state = 0;
            break;
    }
}

void
absorber::on_touch(b2Fixture *my, b2Fixture *other)
{
    entity *e = (entity*)other->GetUserData();
    if (!other->IsSensor() && e && this->can_handle(e) && e != adventure::player) {
        this->pending_fixtures.push_back(other);
    }
}

void
absorber::on_untouch(b2Fixture *my, b2Fixture *other)
{
    entity *e = (entity*)other->GetUserData();
    if (!other->IsSensor() && e && this->can_handle(e) && e != adventure::player) {
        this->pending_fixtures.remove(other);
    }
}

float
absorber::get_slider_snap(int s)
{
    return 1.f / 19.f;
}

float
absorber::get_slider_value(int s)
{
    float v = (((float)this->properties[0].v.i) / 100.f) - 1.f;

    return v / 19.f;
}

void
absorber::on_slider_change(int s, float value)
{
    uint32_t ai = (uint32_t)((1.f + (value * 19.f)) * 100.f);
    this->set_property(0, ai);
    G->show_numfeed((float)ai / 1000.f);
}

edevice*
absorber::solve_electronics()
{
    if (!this->s_out[0].written()) {
        this->s_out[0].write(this->absorbed ? 1.f : 0.f);
        this->absorbed = false;
    }

    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();

    float v = this->s_in[0].get_value();

    if (this->s_in[0].p == 0) {
        /* cable is disconnected, hardcode input value to 1 */
        v = 1.f;
    }

    if ((bool)roundf(v)) {
        this->do_accumulate = true;

        if (this->state == 0) {
            this->state = 1;
        }
    } else {
        this->do_accumulate = false;
        this->state = 0;
    }

    return 0;
}

void
absorber::update_effects()
{
    b2Vec2 p = this->get_position();
    tmat4_load_identity(field->M);
    tmat4_translate(field->M, p.x, p.y, this->get_layer());
    tmat4_rotate(field->M, this->_angle * (180.f/M_PI), 0, 0, -1);
    tmat4_scale(field->M, 2.f, 2.f, 1.f);

    //tms_entity_set_uniform4f(this, "~color", .2f+this->field_life*.5f, .2f+this->field_life*.5f, .2f+this->field_life*.5f, 1.f);
    tms_entity_set_uniform4f(this->field, "~color", 0.8f, 1.f, 0.8f, 0.1f+this->field_life*this->field_life);

    this->field_life -= _tms.dt*4.f;
    if (this->field_life < 0.f) this->field_life = 0.f;
    else {
        /*
        spritebuffer::add(p.x, p.y, this->get_layer()+1.f,
                1.f, 1.f, 1.f, this->field_life*this->field_life*.5f,
                6, 6 ,2);
                */
    }
}

bool
absorber::can_handle(entity *e)
{
    if (this->size == 0) { /* mini absorber */
        switch (e->g_id) {
            case O_BALL:
            case O_METAL_BALL:
            case O_CORNER:
            case O_LAND_MINE:
            case O_BOMB:
            case O_INTERACTIVE_BALL:
                return true;

            case O_INTERACTIVE_BOX:
            case O_BOX:
            case O_CYLINDER:
            case O_INTERACTIVE_CYLINDER:
                return e->properties[0].v.i == 0;

            case O_RESOURCE:
                return W->level.version >= LEVEL_VERSION_1_5;
        }
    } else { /* big absorber */
        switch (e->g_id) {
            case O_BALL:
            case O_METAL_BALL:
            case O_CYLINDER:
            case O_INTERACTIVE_CYLINDER:
            case O_ROBOT:
            case O_CORNER:
            case O_DUMMY:
            case O_LAND_MINE:
            case O_BOMB:
            case O_BOX:
            case O_INTERACTIVE_BOX:
            case O_INTERACTIVE_BALL:
            case O_WEIGHT:
                return true;

            case O_LOBBER:
            case O_BOMBER:
            case O_SPIKEBOT:
            case O_PLASTIC_BOX:
                return W->level.version >= LEVEL_VERSION_1_4;

            case O_ITEM:
            case O_RESOURCE:
                return W->level.version >= LEVEL_VERSION_1_5;
        }

        if (e->is_creature()) {
            return W->level.version >= LEVEL_VERSION_1_5_1;
        }
    }

    return false;
}

autoabsorber::autoabsorber()
{
    this->set_num_properties(1);
    this->properties[0].type = P_INT8;
    this->properties[0].v.i8 = 1;
}

edevice*
autoabsorber::solve_electronics()
{
    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();

    if ((bool)roundf(this->s_in[0].get_value())) {
        std::set<entity*> loop;
        this->gather_connected_entities(&loop, true, true);
        G->absorb(&loop);
        W->events[WORLD_EVENT_ABSORB] ++;
    }

    return 0;
}

edevice*
autoprotector::solve_electronics()
{
    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();

    if ((bool)roundf(this->s_in[0].get_value()) && G && W->level.type == LCAT_ADVENTURE) {
        this->disconnect_all();
        G->absorb(this);
    }

    return 0;
}
