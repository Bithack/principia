#include "emitter.hh"
#include "material.hh"
#include "model.hh"
#include "game.hh"
#include "ui.hh"

#include "object_factory.hh"

#define VELOCITY_MAX 40.f

enum {
    TYPE_MINI = 0,
    TYPE_NORMAL = 1,
    TYPE_MULTI = 2
};

emitter::emitter(int size)
{
    this->set_flag(ENTITY_HAS_CONFIG, true);

    this->size = size;
    this->state = 0;
    this->emit_properties = 0;
    this->emit_interval = 0;

    this->f = 0;
    this->f_frame = 0;
    this->frame_entity = 0;

    this->num_sliders = 1;
    if (this->size == TYPE_MINI) {
        /* mini emitter */
        this->emitter_w = .375f;
        this->emitter_h = .375f;

        this->frame_w   = emitter_w;
        this->frame_h   = 0.1f;

        const float qw = this->emitter_w/2.f+0.15f;
        const float qh = this->emitter_h/2.f+0.55f;

        this->query_sides[0].Set(0.f,  qh);
        this->query_sides[1].Set(-qw, 0.f);
        this->query_sides[3].Set( qw, 0.f);

        this->set_mesh(mesh_factory::get_mesh(MODEL_MINIEMITTER));
        this->num_sliders = 2;
        this->dialog_id = DIALOG_EMITTER;

        this->width = this->emitter_w;
        this->height = this->emitter_h + this->frame_h + 0.1f;
    } else if (this->size == TYPE_NORMAL) {
        /* emitter */
        this->emitter_w = 2.2f /2.f;
        this->emitter_h = 2.2f/2.f;
        this->frame_w   = emitter_w;
        this->frame_h   = 0.1f;
        this->set_mesh(mesh_factory::get_mesh(MODEL_EMITTER));
        this->set_flag(ENTITY_ALLOW_CONNECTIONS, false);
        this->update_method = ENTITY_UPDATE_STATIC;
        this->set_flag(ENTITY_IS_LOW_PRIO, true);
        this->set_flag(ENTITY_IS_STATIC, true);
        this->dialog_id = DIALOG_EMITTER;

        this->width = this->emitter_w;
        this->height = this->emitter_h;
    } else if (this->size == TYPE_MULTI) {
        /* multi emitter */
        this->emitter_w = 5.2f /2.f;
        this->emitter_h = 5.2f/2.f;
        this->frame_w   = emitter_w;
        this->frame_h   = 0.1f;
        this->set_mesh(const_cast<tms::mesh*>(tms::meshfactory::get_cube()));
        this->set_flag(ENTITY_ALLOW_CONNECTIONS, false);
        this->update_method = ENTITY_UPDATE_STATIC_CUSTOM;
        this->set_flag(ENTITY_IS_LOW_PRIO, true);
        this->set_flag(ENTITY_IS_STATIC, true);

        this->dialog_id = DIALOG_MULTIEMITTER;

        this->width = this->emitter_w;
        this->height = this->emitter_h;
    }

    this->set_flag(ENTITY_HAS_TRACKER, this->size != 2);

    this->set_material(&m_pv_colored);
    this->set_uniform("~color", .2f, .2f, .2f, 1.f);

    this->set_flag(ENTITY_DO_STEP, true);

    this->set_num_properties(this->size == TYPE_MULTI ? 8 : 7);
    this->properties[0].type = P_INT;
    this->set_property(0, (uint32_t)500); /* Emit Interval in milliseconds*/
    this->set_property(1, (uint32_t)0);   /* g_id of entity to emit */
    this->set_property(2, (uint32_t)0);   /* id of entity to copy properties from */
    this->properties[3].type = P_FLT;
    this->set_property(3, 0.f);   /* velocity of emitted object */
    this->properties[4].type = P_INT;
    this->properties[4].v.i = 0; /* num properties of the copied object */
    /* for mini and normal, a string containing a parsable version of the copied objects properties */
    this->properties[5].type = P_STR;
    this->set_property(5, "");

    if (this->size == TYPE_MULTI) {
        this->properties[6].type = P_FLT;
        this->properties[6].v.i = 4.f; /* width */
        this->properties[7].type = P_FLT;
        this->properties[7].v.i = 4.f; /* height */
    } else {
        this->properties[6].type = P_FLT; /* absorb after n seconds */
        this->properties[6].v.i = 0.f;
    }

    this->num_s_in = 1;
    this->num_s_out = 1;

    this->s_out[0].lpos = b2Vec2(-.125f, +this->emitter_h+this->frame_h-0.025f);
    this->s_in[0].lpos  = b2Vec2( .125f, +this->emitter_h+this->frame_h-0.025f);

    this->scaleselect = true;
    this->menu_scale = .5f;

    if (this->size == TYPE_NORMAL) {
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

    if (this->size == TYPE_MULTI) {
        struct tms_entity *e = tms_entity_alloc();
        //tms_entity_set_mesh(e, tms_meshfactory_get_cube());
        tms_entity_set_mesh(e, mesh_factory::get_mesh(MODEL_EMITTER_FRAME));
        tms_entity_set_uniform4f(e, "~color", .2f, .2f, .2f, 1.0f);
        tms_entity_set_material(e, &m_pv_colored);
        tms_entity_add_child(this, e);

        tmat4_load_identity(e->M);
        tmat3_load_identity(e->N);
        this->frame_entity = e;
    }
}

void
emitter::on_touch(b2Fixture *my, b2Fixture *other)
{
    /*
    entity *o = other->GetUserData();
    if (o && )
    */
    if (!other->IsSensor()) {
        this->num_in_field ++;
    }
}

void
emitter::on_untouch(b2Fixture *my, b2Fixture *other)
{
    /*
    entity *o = other->GetUserData();
    if (o && )
    */
    if (!other->IsSensor()) {
        this->num_in_field --;
        if (this->num_in_field <0) this->num_in_field = 0;
    }
}

void
emitter::add_to_world()
{
    b2BodyDef bd;
    bd.type = (this->size == TYPE_MINI ? this->get_dynamic_type() : b2_staticBody);
    bd.position = _pos;
    bd.angle = _angle;
    b2Body *b = W->b2->CreateBody(&bd);
    this->body = b;

    if (this->size == TYPE_NORMAL) {
        /* big emitter */
        b2PolygonShape box;
        box.SetAsBox(this->emitter_w, this->emitter_h);

        b2FixtureDef fd;
        fd.shape = &box;
        fd.density = 1.f;
        fd.friction = .5f;
        fd.restitution = .3f;
        fd.isSensor = true;
        fd.filter = world::get_filter_for_layer(this->get_layer(), 15);

        b2PolygonShape frame_shape;
        frame_shape.SetAsBox(this->frame_w, this->frame_h, b2Vec2(0.f, +this->emitter_h+this->frame_h), 0.f);

        b2FixtureDef frame_fd;
        frame_fd.shape = &frame_shape;
        frame_fd.density = 1.0f;
        frame_fd.friction = .5f;
        frame_fd.restitution = .3f;
        frame_fd.filter = world::get_filter_for_layer(this->get_layer(), 15);

        (this->body->CreateFixture(&frame_fd))->SetUserData(this);
        (this->body->CreateFixture(&fd))->SetUserData(this);

        this->field_life = 0.f;
    } else if (this->size == TYPE_MINI) {
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
        sh.SetAsBox(this->frame_w+.05f, this->frame_h, b2Vec2(0.f, +this->emitter_h+this->frame_h), 0.f);
        (this->body->CreateFixture(&fd))->SetUserData(this);

        /* left */
        sh.SetAsBox(.05f, this->emitter_h, b2Vec2(-.365f, 0.f), 0.f);
        (this->body->CreateFixture(&fd))->SetUserData(this);

        /* right */
        sh.SetAsBox(.05f, this->emitter_h, b2Vec2(.365f, 0.f), 0.f);
        (this->body->CreateFixture(&fd))->SetUserData(this);
    } else if (this->size == TYPE_MULTI) {
        /* multi emitter */
        this->f = 0;
        this->f_frame = 0;
        this->recreate_multiemitter_shape();
    }
}

/* only used by multiemitter */
void
emitter::update()
{
    if (this->body) {
        b2Transform t;
        t = this->body->GetTransform();

        tmat4_load_identity(this->M);
        this->M[0] = t.q.c;
        this->M[1] = t.q.s;
        this->M[4] = -t.q.s;
        this->M[5] = t.q.c;
        this->M[12] = t.p.x;
        this->M[13] = t.p.y;
        this->M[14] = this->prio * LAYER_DEPTH;

        tmat3_load_identity(this->N);
        if (this->size == TYPE_MULTI) {
            tmat3_load_identity(this->frame_entity->N);
            tmat4_copy(this->frame_entity->M, this->M);
            tmat4_translate(this->frame_entity->M, 0.f, this->emitter_h+.125f, 0.f);
        }

        this->M[14] = this->prio * LAYER_DEPTH-.5f;
        tmat4_scale(this->M, this->emitter_w*2.f, this->emitter_h*2.f, .2f);
    } else {
        tmat4_load_identity(this->M);
        tmat4_translate(this->M, this->_pos.x, this->_pos.y, 0);
        tmat4_rotate(this->M, this->_angle * (180.f/M_PI), 0, 0, -1);
        tmat3_copy_mat4_sub3x3(this->N, this->M);
    }
}

void
emitter::recreate_multiemitter_shape()
{
    if (this->properties[6].v.f < 2.0f) this->properties[6].v.f = 2.0f;
    if (this->properties[7].v.f < 2.0f) this->properties[7].v.f = 2.0f;

    this->emitter_w = this->properties[6].v.f / 2.f;
    this->emitter_h = this->properties[7].v.f / 2.f;
    this->frame_w   = 2.2f/2.f;
    this->frame_h   = 0.1f;

    this->s_in[0].lpos  = b2Vec2( .15f, +this->emitter_h+this->frame_h);
    this->s_out[0].lpos = b2Vec2(-.15f, +this->emitter_h+this->frame_h);

    if (f_frame) this->body->DestroyFixture(f_frame);
    if (f) this->body->DestroyFixture(f);

    /* multi emitter */
    b2PolygonShape box;
    box.SetAsBox(this->emitter_w, this->emitter_h);

    b2FixtureDef fd;
    fd.shape = &box;
    fd.density = 1.f;
    fd.friction = .5f;
    fd.restitution = .3f;
    fd.isSensor = true;
    fd.filter = world::get_filter_for_multilayer(15, 15, 15);

    b2PolygonShape frame_shape;
    frame_shape.SetAsBox(this->frame_w, this->frame_h, b2Vec2(0.f, +this->emitter_h+this->frame_h), 0.f);

    b2FixtureDef frame_fd;
    frame_fd.shape = &frame_shape;
    frame_fd.density = 1.0f;
    frame_fd.friction = .5f;
    frame_fd.restitution = .3f;
    frame_fd.filter = world::get_filter_for_layer(this->get_layer(), 15);

    (f_frame = this->body->CreateFixture(&frame_fd))->SetUserData(this);
    (f = this->body->CreateFixture(&fd))->SetUserData(this);

}

void
emitter::update_effects()
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

void
emitter::on_load(bool created, bool has_state)
{
    if (this->size != 2)
        this->load_properties();
}

void
emitter::setup()
{
    this->did_emit = false;
    this->do_accumulate = false;
    this->time = this->emit_interval;
    this->state = 0;
}

void
emitter::init()
{
    this->emit_interval = this->properties[0].v.i * 1000;
    this->num_in_field = 0;
}

void
emitter::step()
{
    int g_id = this->properties[1].v.i;
    if (g_id == O_PLANK && this->size != 2) return;

    this->time += G->timemul(WORLD_STEP);

    if (!this->do_accumulate && this->time >= this->emit_interval) {
        this->time = this->emit_interval;
    }

    switch (this->state) {
        case 0:
            /* waiting for signal */
            break;

        case 1:
            /* check cooldown timer */
            if (this->time >= this->emit_interval) {
                this->time -= this->emit_interval;
                this->state = 2;
            } else {
                this->state = 0;
            }
            break;

        case 2:
            if (!W->level.flag_active(LVL_DISABLE_PHYSICS)) {
                if (this->num_in_field > 0) {
                    this->state = 1;
                    break;
                }

                b2Vec2 cur_loc = this->get_position();

                if (this->size != 2) {
                    entity *e = of::create(g_id);

                    cur_loc += b2Vec2(-.005f+rand()%100 / 100.f * .01f, -.005f+rand()%100 / 100.f * .01f);

                    e->_angle = this->get_angle();

                    if (this->size == TYPE_MINI) {
                        e->_angle += M_PI;
                    }

                    e->set_position(cur_loc.x, cur_loc.y);
                    e->set_layer(this->get_layer());

                    if (this->properties[4].v.i > 0) {
                        if (this->properties[4].v.i <= e->num_properties) {
                            memcpy(e->properties, this->emit_properties, this->properties[4].v.i*sizeof(property));
                        } else {
                            tms_errorf("Unable to copy properties, they don't match!");
                        }
                    }

                    this->field_life = 1.1f;

                    b2Vec2 v;

                    if (this->size == TYPE_MINI) {
                        v = b2Vec2(
                                cosf(this->get_angle()-M_PI/2.f)*this->properties[3].v.f,
                                sinf(this->get_angle()-M_PI/2.f)*this->properties[3].v.f
                                );
                    } else {
                        v = b2Vec2(0.f, 0.f);
                    }

                    b2Vec2 rel = this->get_body(0)->GetLinearVelocityFromLocalPoint(this->local_to_body(b2Vec2(0.f, 0.f), 0));
                    v += rel;

                    G->emit(e, 0, v);

                    if (this->size != TYPE_MULTI) {
                        if (this->properties[6].v.f > 1.f) {
                            G->timed_absorb(e, this->properties[6].v.f);
                        }
                    }

                    b2Vec2 pos = this->get_position();

                    if (e->flag_active(ENTITY_IS_EXPLOSIVE)) {
                        G->play_sound(SND_ROBOT_BOMB, pos.x, pos.y, rand(), 1.f);
                    } else {
                        G->play_sound(SND_EMIT, pos.x, pos.y, rand(), 1.f);
                    }
                } else {
                    if (this->properties[5].v.s.len) {
                        G->emit_partial_from_buffer(this->properties[5].v.s.buf, this->properties[5].v.s.len, cur_loc);
                    }
                }
            }
            this->did_emit = true;
            this->state = 0;
            break;

        default:
            this->state = 0;
            break;
    }
}

float
emitter::get_slider_snap(int s)
{
    if (s == 0) {
        return 1.f / 19.f;
    }

    /* s == 1 */

    return .05f;
}

float
emitter::get_slider_value(int s)
{
    if (s == 0) {
        float v = (((float)this->properties[0].v.i) / 100.f) - 1.f;
        return v / 19.f;
    }

    /* s == 1 */
    return tclampf(this->properties[3].v.f / VELOCITY_MAX, 0.f, 1.f);
}

void
emitter::on_slider_change(int s, float value)
{
    if (s == 0) {
        uint32_t ei = (uint32_t)((1.f + (value * 19.f)) * 100.f);
        this->set_property(0, ei);

        G->show_numfeed((float)ei / 1000.f);
        return;
    }

    /* s == 1 */

    this->set_property(3, value*VELOCITY_MAX);
    G->show_numfeed(value*VELOCITY_MAX);
}

edevice*
emitter::solve_electronics()
{
    if (!this->s_in[0].is_ready()) {
        return this->s_in[0].get_connected_edevice();
    }

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
    }

    this->s_out[0].write(this->did_emit ? 1.f : 0.f);
    this->did_emit = false;

    return 0;
}

bool
emitter::can_handle(entity *e) const
{
    if (this->size == TYPE_MINI) { /* mini emitter */
        switch (e->g_id) {
            case O_BALL:
            case O_METAL_BALL:
            case O_CORNER:
            case O_LAND_MINE:
            case O_BOMB:
            case O_INTERACTIVE_BALL:

            /* special case for boxes and cylinders, we force their size to the smallest size */
            case O_INTERACTIVE_BOX:
            case O_BOX:
            case O_CYLINDER:
            case O_INTERACTIVE_CYLINDER:
                return true;

            case O_PLASTIC_BOX:
                return W->level.version >= LEVEL_VERSION_1_5;

            case O_RESOURCE:
                return W->level.version >= LEVEL_VERSION_1_5;
        }
    } else { /* emitter */
        switch (e->g_id) {
            case O_BALL:
            case O_METAL_BALL:
            case O_CYLINDER:
            case O_INTERACTIVE_CYLINDER:
            case O_INTERACTIVE_BOX:
            case O_INTERACTIVE_BALL:
            case O_ROBOT:
            case O_ANIMAL:
            case O_CORNER:
            case O_DUMMY:
            case O_LAND_MINE:
            case O_BOMB:
            case O_BOX:
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

void
emitter::set_partial(uint32_t id)
{
    lvledit lvl;
    if (lvl.open(LEVEL_PARTIAL, id)) {
        if (lvl.lvl.type != LCAT_PARTIAL) {
            tms_errorf("could not load partial");
            return;
        }

        this->set_property(6, lvl.lvl.max_x - lvl.lvl.min_x + .5f);
        this->set_property(7, lvl.lvl.max_y - lvl.lvl.min_y + .5f);
        this->recreate_multiemitter_shape();

        tms_infof("set emitter partial %u: %.*s", id, lvl.lvl.name_len, lvl.lvl.name);
        this->set_propertyi8(4, 0);
        this->set_property(5, (const char*)lvl.lb.buf, (uint16_t)lvl.lb.size);
    } else {
        tms_errorf("could not open partial");
    }
}

void
emitter::copy_properties(entity *e)
{
    static char properties_str[4096] = {0};

    properties_str[0] = '\0';

    for (int x=0; x<e->num_properties; ++x) {
        char *s;

        /* special case for boxes and mini emitter, accept boxes but force their size to 0 */
        if (this->size == TYPE_MINI &&
                (e->g_id == O_INTERACTIVE_BOX || e->g_id == O_BOX || e->g_id == O_CYLINDER || e->g_id == O_INTERACTIVE_CYLINDER || e->g_id == O_PLASTIC_BOX)
                && x == 0) {
            uint32_t i_saved = e->properties[x].v.i;
            e->properties[x].v.i = 0;
            s = e->properties[x].stringify();
            e->properties[x].v.i = i_saved;
        } else
            s = e->properties[x].stringify();

        /* XXX: This will definitely break if one of the properties contains a newline */
        if (x != 0) strcat(properties_str, "\n");
        strcat(properties_str, s);

        free(s);
    }

    this->set_property(4, e->num_properties);
    this->set_property(5, properties_str);

    this->load_properties();
}

void
emitter::load_properties()
{
    if (this->properties[4].v.i > 0 && this->properties[5].v.s.len > 0 && this->properties[5].v.s.buf) {
        if (this->emit_properties)
            delete [] this->emit_properties;

        this->emit_properties = new property[this->properties[4].v.i];

        char *tmp = strdup(this->properties[5].v.s.buf);
        /* XXX: This will definitely break if one of the properties contains a newline */
        char *pch = strtok(tmp, "\n");
        bool first = true;
        for (int x=0; x<this->properties[4].v.i; ++x) {
            if (pch == NULL) {
                tms_errorf("Mismatching amount of properties");
                break;
            }

            this->emit_properties[x].parse(pch);

            /* XXX: This will definitely break if one of the properties contains a newline */
            pch = strtok(NULL, "\n");
        }
        free(tmp);
    }
}
