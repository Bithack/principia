#include "fxemitter.hh"
#include "game.hh"
#include "world.hh"
#include "spritebuffer.hh"
#include "linebuffer.hh"
#include "soundmanager.hh"
#include "ledbuffer.hh"
#include "ui.hh"
#include "item.hh"

#define DEBRIS_FORCE 300.f

fxemitter::fxemitter()
{
    this->set_flag(ENTITY_DO_UPDATE_EFFECTS,    true);
    this->set_flag(ENTITY_DO_STEP,              true);
    this->set_flag(ENTITY_HAS_CONFIG,           true);

    this->dialog_id = DIALOG_FXEMITTER;

    this->menu_scale = 1.0f;

    if (W->level.version < LEVEL_VERSION_1_1_6) {
        this->set_as_rect(.15f, .375f);
    }

    this->conns = 0;

    /** 
     * radius
     * count
     * interval
     **/
    this->set_num_properties(9);
    this->properties[0].type = P_FLT; /* radius */
    this->properties[0].v.f = 1.f;
    this->properties[1].type = P_INT; /* count */
    this->properties[1].v.i = 5;
    this->properties[2].type = P_FLT; /* interval */
    this->properties[2].v.f = .3f;

    this->properties[3].type = P_INT; /* effect type */
    this->properties[3].v.i = FX_EXPLOSION;

    this->properties[4].type = P_INT; /* effect type */
    this->properties[4].v.i = FX_INVALID;

    this->properties[5].type = P_INT; /* effect type */
    this->properties[5].v.i = FX_INVALID;

    this->properties[6].type = P_INT; /* effect type */
    this->properties[6].v.i = FX_INVALID;

    this->properties[7].type = P_FLT; /* rel offs x */
    this->properties[7].v.f = 0.f;

    this->properties[8].type = P_FLT; /* rel offs y */
    this->properties[8].v.f = 0.f;

    for (int n=0; n<4; ++n) {
        this->num_emitted[n] = 0;
        this->next[n] = 0;
    }
}

void
fxemitter::step()
{
    if (this->activated) {
        this->time += G->timemul(WORLD_STEP);

        uint32_t mask = 0; /* mask to prevent the same effect from being run twice */

        int done = 0;

        for (int n=0; n<4; n++) {
            if (this->properties[3+n].v.i == FX_INVALID) { done++; continue; }
            int m = (1u << this->properties[3+n].v.i);
            if (mask & m) {done++;continue;}
            mask |= m;

            switch (this->properties[3+n].v.i) {
                case FX_EXPLOSION: 
                case FX_SMOKE:
                case FX_MAGIC:
                case FX_BREAK:
                    {
                        if (num_emitted[n] < this->properties[1].v.i) {
                            if (this->time >= this->next[n]) {
                                float a = (rand()%100)/100.f * M_PI*2.f;
                                float r = (rand()%100)/100.f * this->properties[0].v.f;
                                b2Vec2 p = this->get_position();

                                float cs, sn;
                                tmath_sincos(a, &sn, &cs);
                                p.x += cs*r;
                                p.y += sn*r;

                                entity *e = 0;
                                if (this->properties[3+n].v.i == FX_EXPLOSION) e = new explosion_effect(p, this->get_layer(), false);
                                else if (this->properties[3+n].v.i == FX_SMOKE) e = new smoke_effect(p, this->get_layer());
                                else if (this->properties[3+n].v.i == FX_MAGIC) e = new magic_effect(p, this->get_layer());
                                else if (this->properties[3+n].v.i == FX_BREAK) e = new break_effect(p, this->get_layer());

                                G->emit(e, this, b2Vec2(0,0));

                                this->next[n] = this->get_next_time();

                                num_emitted[n] ++ ;
                            }
                        } else {
                            done ++;
                        }
                    }
                    break;

                case FX_HIGHLIGHT: /* highlight */
                    {
                        if (this->time >= 100000)
                            done ++;
                    }
                    break;

                case FX_DESTROYCONN: /* connection destroy */
                    {
                        size_t sz;
                        if (this->conns && num_emitted[n] < this->properties[1].v.i && (sz = this->conns->size())) {
                            if (this->time >= this->next[n]) {
                                std::set<connection *>::iterator i = this->conns->begin();

                                size_t adv;
                                if (sz > 1) adv = rand()%(sz-1);
                                else adv = 0;
                                std::advance(i, adv);

                                W->destroy_connection_joint(*i);
                                this->conns->erase(i);

                                this->next[n] = this->get_next_time();
                                this->num_emitted[n] ++;
                            }
                        } else {
                            if (this->conns) {
                                delete this->conns;
                                this->conns = 0;
                            }
                            done ++;
                        }
                    }
                    break;
            }
        }

        if (done == 4) {
            /* all effects done */
            this->activated = false;
            this->completed = true;
        }
    }
}

void
fxemitter::update_effects()
{
    float z = this->get_layer() * LAYER_DEPTH + LED_Z_OFFSET;
    b2Vec2 p = this->get_position();
    ledbuffer::add(p.x, p.y, z, this->activated?1.f:0.f);
}

edevice*
fxemitter::solve_electronics()
{
    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();

    if (!this->activated) {
        if ((bool)roundf(this->s_in[0].get_value())) {
            this->time = 0;

            uint32_t mask = 0; /* mask to prevent the same effect from being run twice */

            for (int n=0; n<4; n++) {
                if (this->properties[3+n].v.i == FX_INVALID) {continue;};
                int m = (1u << this->properties[3+n].v.i);
                if (mask & m) continue;
                mask |= m;

                switch (this->properties[3+n].v.i) {
                    case FX_EXPLOSION: case FX_SMOKE: case FX_MAGIC: case FX_BREAK:
                        this->num_emitted[n] = 0;
                        this->next[n] = 0;
                        break;

                    case FX_HIGHLIGHT:
                        {
                            std::set<entity*> *loop = new std::set<entity*>();
                            this->gather_connected_entities(loop);
                            G->add_highlight_multi(loop);
                        }
                        break;

                    case FX_DESTROYCONN:
                        {
                            std::set<connection *> *loop = new std::set<connection*>();

                            this->gather_connections(loop);

                            for (std::set<connection*>::iterator i = loop->begin();
                                    i != loop->end();) {

                                connection *c = *i;
                                if ((c->e->local_to_world(c->p, c->f[0])-this->get_position()).Length() > this->properties[0].v.f
                                    || (c->type != CONN_PLATE && c->type != CONN_PIVOT)) {
                                    loop->erase(i++);
                                } else
                                    i++;
                            }

                            this->time = 0;
                            this->num_emitted[n] = 0;
                            this->next[n] = 0;
                            this->conns = loop;
                        }
                        break;
                }
            }
            this->activated = true;
        }
    }

    if (this->completed) {
        this->s_out[0].write(1.f);
        this->completed = false;
    } else
        this->s_out[0].write(0.f);

    return 0;
}

void
fxemitter::setup()
{
    this->completed = false;
    this->conns = 0;
    this->activated = false;
}

void
fxemitter::on_pause()
{
    if (this->conns)
        delete this->conns;
    this->conns = 0;
    this->completed = false;
    this->activated = false;
}

debris::debris(b2Vec2 force, b2Vec2 pos)
{
    this->set_flag(ENTITY_FADE_ON_ABSORB,   false);
    this->set_flag(ENTITY_DO_STEP,          true);

    this->life = 800000 + rand()%700000;
    this->g_id = 0;
    this->_pos = pos;
    this->_angle = 0;
    this->set_mesh(mesh_factory::get_mesh(MODEL_DEBRIS));
    this->set_material(&m_wmotor);
    this->initial_force = force;
}

void
debris::step(void)
{
    this->life -= G->timemul(WORLD_STEP);
    if (this->life <= 0)
        G->absorb(this);
}

void
debris::add_to_world()
{
    this->create_circle(this->get_dynamic_type(), .05f, this->material);
    this->body->ApplyForce(this->initial_force, this->_pos);
}

spark_effect::spark_effect(b2Vec2 pos, int layer)
{
    this->_pos = pos;
    this->life = 1;
    this->trigger_point = pos;
    this->set_layer(layer);
    this->cull_effects_method = CULL_EFFECTS_BY_POSITION;
    this->update_method = ENTITY_UPDATE_NULL;

    for (int x=0; x<NUM_SPARKS; x++) {
        struct piece *f = &this->pieces[x];
        f->life = .5f + (rand()%100) / 100.f;
        f->x = trigger_point.x;
        f->y = trigger_point.y;

        float a = (rand()%100) / 100.f * M_PI*2.f;
        f->vx = 2.f * cosf(a);
        f->vy = 2.f * sinf(a);
    }

    this->played_sound = false;
}

void
spark_effect::mstep()
{
    int num_active = 0;
    float dt = G->timemul(WORLD_STEP) * 0.000001f;

    for (int x=0; x<NUM_SPARKS; x++) {
        struct piece *f = &this->pieces[x];
        if (f->life > 0.f) {
            f->life -= dt * SPARK_DECAY_RATE;
            f->vx += W->get_gravity().x * dt;
            f->vy += W->get_gravity().y * dt;
            f->x += f->vx * dt;
            f->y += f->vy * dt;

            ++num_active;
        }
    }

    this->life -= dt*SPARK_DECAY_RATE*3.f;

    if (num_active == 0) {
        G->lock();
        G->absorb(this);
        G->unlock();
    }
}

void
spark_effect::update_effects()
{
    if (!played_sound) {
        //sm::play(&sm::explosion, this->trigger_point.x, this->trigger_point.y, rand(), 1.f);
        played_sound = true;
    }

    for (int x=0; x<NUM_SPARKS; ++x) {
        struct piece *f = &this->pieces[x];
        if (f->life > 0.f) {
            spritebuffer::add2(f->x, f->y, this->get_layer()*LAYER_DEPTH+.5f,
                    2*2.f*f->life, 2*2.f*f->life, 2*1.5f*f->life, 2.f,
                    .05f, .05f, 1);
        }
    }

    if (this->life > 0.5f) {
        spritebuffer::add2(this->trigger_point.x, this->trigger_point.y, this->get_layer()*LAYER_DEPTH,
                1.f, 1.f, 1.f, .125f,
                1.0f, 1.0f, 2);
        this->life = 0;
    }
}

explosion_effect::explosion_effect(b2Vec2 pos, int layer, bool with_debris, float scale) : base_effect()
{
    this->_pos = pos;
    this->trigger_point = pos;
    this->set_layer(layer);
    this->cull_effects_method = CULL_EFFECTS_BY_POSITION;
    this->update_method = ENTITY_UPDATE_NULL;
    this->scale = scale;

    for (int x=0; x<NUM_FIRES; x++) {
        struct particle *f = &this->particles[x];
        f->a = (float)(rand()%100) / 100.f * M_PI*2.f;
        f->life = (.5f + (rand()%100) / 100.f)*scale;
        f->x = trigger_point.x + (-.75f + (rand()%100) / 100.f * 1.5f)*scale;
        f->y = trigger_point.y + (-.75f + (rand()%100) / 100.f * 1.5f)*scale;
        f->z = this->get_layer()*LAYER_DEPTH + (rand()%100) / 150.f + (1.f-scale);
        f->s = (.75f + rand()%100 / 50.f)*scale;
    }

    /*
    for (int x=0; x<5; x++) {
        struct piece *p = &this->pieces[x];

        p->x = trigger_point.x;
        p->y = trigger_point.y;

        float a = (rand()%100) / 100.f * M_PI*2.f;

        p->vx = 15.f * cosf(a);
        p->vy = 15.f * sinf(a);
        p->life = 0.1f + (rand()%100)/100.f * .25f;
    }
    */

    if (with_debris) {
        float rr = 0.f;
        for (int x=0; x<3; x++) {

            rr += .25f + (rand()%100) / 100.f;
            //float a = ((M_PI*2.f)/(float)100.f) * (rand()%100);
            float a = rr;
            b2Vec2 r = b2Vec2(cosf(a), sinf(a));

            r.x*=DEBRIS_FORCE/12.f;
            r.y*=DEBRIS_FORCE/12.f;
            debris *d = new debris(r, this->trigger_point);
            d->set_layer(layer);
            G->emit(d, this);
        }
    }

    this->life = 1.f;
    this->played_sound = false;
}

void
explosion_effect::mstep()
{
    int num_active = 0;
    for (int x=0; x<NUM_FIRES; x++) {
        struct particle *f = &this->particles[x];
        float s_life = .2f;
        if (f->life > -s_life) {
            f->life -= G->timemul(WORLD_STEP) * 0.000001f * EXPLOSION_FIRE_DECAY_RATE;

            ++num_active;
        }
    }

    if (life > 0.f) {
        this->life -= G->timemul(WORLD_STEP) *  0.000001f * EXPLOSION_DECAY_RATE;
    }

    if (num_active == 0) {
        G->lock();
        G->absorb(this);
        G->unlock();
    }
}

void
explosion_effect::update_effects()
{
    if (!played_sound) {
        if (scale < .75f) {
            G->play_sound(SND_EXPLOSION_LIGHT, this->trigger_point.x, this->trigger_point.y, rand(), 1.f);
        } else {
            G->play_sound(SND_EXPLOSION, this->trigger_point.x, this->trigger_point.y, rand(), 1.f);
        }
        played_sound = true;
    }

    for (int x=0; x<NUM_FIRES; ++x) {
        struct particle *f = &this->particles[x];
        float s_life = .2f;
        if (f->life > -s_life) {
            if (x > NUM_FIRES/4)
                spritebuffer::add(f->x, f->y, f->z,
                        2.f*f->life, 2.f*f->life, 1.f*f->life, 1.f*(f->life+s_life),
                        f->s, f->s, 0, f->a);
            else
                spritebuffer::add2(f->x, f->y, f->z,
                        2.f*f->life, 2.f*f->life, 1.f*f->life, 1.f*(f->life+s_life),
                        f->s, f->s, 0, f->a);
        }
    }

    /*
    // emit shrapnel
    for (int x=0; x<5; x++) {
        struct piece *f = &this->pieces[x];
        if (f->life > -0.1f) {
            spritebuffer::add(f->x, f->y, this->prio*LAYER_DEPTH+1.f,
                    5.0f*f->life, 5.0f*f->life, 4.0f*f->life, f->life+.1f,
                    .18f, .18f, 0);
            f->life-=_tms.dt;

            f->x += f->vx * _tms.dt;
            f->y += f->vy * _tms.dt;
            num_active ++;
        }
    }
    */

    if (this->life > 0.f && this->scale > .75f) {
        spritebuffer::add(this->trigger_point.x, this->trigger_point.y, this->get_layer()*LAYER_DEPTH+0.5f,
                1.f, 1.f, 1.f, .6f*life,
                7.f, 7.f, 1);
    }
}

smoke_effect::smoke_effect(b2Vec2 pos, int layer, float col, float scale)
{
    this->_pos = pos;
    this->scale = scale;
    this->col = col;
    this->trigger_point = pos;
    this->set_layer(layer);
    this->update_method = ENTITY_UPDATE_NULL;
    this->cull_effects_method = CULL_EFFECTS_BY_POSITION;

    for (int x=0; x<NUM_SMOKE_PARTICLES; x++) {
        struct particle *f = &this->particles[x];
        f->a = (float)(rand()%100) / 100.f * M_PI*2.f;
        f->life = .5f + (rand()%100) / 100.f;
        f->x = trigger_point.x + (-.5f + (rand()%100) / 100.f)*scale*.5f;
        f->y = trigger_point.y + (-.5f + (rand()%100) / 100.f)*scale*.5f;
        f->z = this->get_layer()*LAYER_DEPTH;// + (rand()%100) / 200.f;
        f->s = .05f + (rand()%100 / 150.f)*scale;
    }
}

void
smoke_effect::mstep()
{
    int num_active = 0;
    for (int x=0; x<NUM_SMOKE_PARTICLES; ++x) {
        struct particle *f = &this->particles[x];
        if (f->life > 0.f) {
            f->s += G->timemul(WORLD_STEP) * 0.000001f*scale;
            f->life -= G->timemul(WORLD_STEP) * 0.000001f;
            f->y += G->timemul(WORLD_STEP) * 0.000001f*scale;
            f->a += G->timemul(WORLD_STEP) * 0.000001f;

            ++ num_active;
        }
    }

    if (num_active == 0) {
        G->lock();
        G->absorb(this);
        G->unlock();
    }
}

void
smoke_effect::update_effects()
{
    for (int x=0; x<NUM_SMOKE_PARTICLES; ++x) {
        struct particle *f = &this->particles[x];
        if (f->life > 0.f) {
            if (this->col > 0.f) {
                spritebuffer::add2(
                        f->x, f->y, f->z,
                        0.1f+this->col, 0.1f+this->col, 0.1f+this->col, 0.25f*f->life,
                        f->s, f->s, 0, f->a);
            } else
                spritebuffer::add(
                        f->x, f->y, f->z,
                        0.1f+this->col, 0.1f+this->col, 0.1f+this->col, 0.25f*f->life,
                        f->s, f->s, 0, f->a);
        }
    }
}

magic_effect::magic_effect(b2Vec2 pos, int layer, int num_particles/*=3*/)
{
    this->num_particles = num_particles;
    this->particles = (struct particle*)calloc(num_particles, sizeof(struct particle));
    this->_pos = pos;
    this->update_method = ENTITY_UPDATE_NULL;
    this->cull_effects_method = CULL_EFFECTS_BY_POSITION;

    this->continuous = false;

    this->color = tvec4f(.9f, .9f, 1.f, 1.f);
    this->speed_mod = 1.f;

    this->update_pos(pos, layer);

    for (int x=0; x<this->num_particles; x++) {
        this->create_particle(x);
    }
}

magic_effect::~magic_effect()
{
    free(this->particles);
}

void
magic_effect::create_particle(int slot)
{
    struct particle *f = &this->particles[slot];
    f->life = 1.f + trandf(0.f, 0.5f);
    f->x = this->_pos.x + trandf(-0.5f, 0.5f);
    f->y = this->_pos.y + trandf(-0.5f, 0.5f);
    f->a = .5f+trandf(0.f, 2.f);
    f->z = (this->get_layer()*LAYER_DEPTH) + trandf(0.f, 0.5f);
    f->s = .08f + trandf(0.f, 0.1f);
}

void
magic_effect::update_pos(b2Vec2 pos, int layer)
{
    this->_pos = pos;
    this->set_layer(layer);
}

void
magic_effect::mstep()
{
    int num_active = 0;
    for (int x=0; x<this->num_particles; x++) {
        struct particle *f = &this->particles[x];
        if (f->life > 0.f) {
            f->life -= (G->timemul(WORLD_STEP) * 0.000001f) * this->speed_mod;

            ++ num_active;
        }
    }

    if (this->continuous) {
        for (int x=0; x<this->num_particles; x++) {
            struct particle *f = &this->particles[x];
            if (f->life <= 0.f) {
                this->create_particle(x);
            }
        }
    } else if (num_active == 0) {
        G->lock();
        G->absorb(this);
        G->unlock();
    }
}

void
magic_effect::update_effects()
{
    for (int x=0; x<this->num_particles; x++) {
        struct particle *f = &this->particles[x];
        if (f->life > 0.f) {
            spritebuffer::add2(
                    f->x, f->y, f->z,
                    this->color.r, this->color.g, this->color.b, this->color.a-(fabsf(f->life-.5f)*2.f),
                    f->s, f->s, 4);
        }
    }
}

break_effect::break_effect(b2Vec2 pos, int layer)
{
    this->_pos = pos;
    this->trigger_point = pos;
    this->cull_effects_method = CULL_EFFECTS_BY_POSITION;
    this->update_method = ENTITY_UPDATE_NULL;
    this->prio = layer;

    for (int x=0; x<NUM_BREAK_PARTICLES; x++) {
        struct piece *p = &this->pieces[x];

        p->x = trigger_point.x;
        p->y = trigger_point.y;

        float a = (rand()%100) / 100.f * M_PI*2.f;

        p->vx = 5.f * cosf(a);
        p->vy = 5.f * sinf(a);
        p->life = 0.5f + (rand()%100)/100.f * .5f;
    }
}

void
break_effect::mstep()
{
    int num_active = 0;
    for (int x=0; x<NUM_BREAK_PARTICLES; x++) {
        struct piece *f = &this->pieces[x];
        if (f->life > 0.f) {
            f->life -= G->timemul(WORLD_STEP) * 0.000001f * BREAK_DECAY_RATE;

            f->x += G->timemul(WORLD_STEP) * 0.000001f * f->vx;
            f->y += G->timemul(WORLD_STEP) * 0.000001f * f->vy;

            ++ num_active;
        }
    }

    if (num_active == 0) {
        G->lock();
        G->absorb(this);
        G->unlock();
    }
}

void
break_effect::update_effects()
{
    for (int x=0; x<NUM_BREAK_PARTICLES; x++) {
        struct piece *f = &this->pieces[x];
        if (f->life > 0.f) {
            spritebuffer::add(f->x, f->y, this->prio*LAYER_DEPTH+1.f,
                    0.2f, 0.2f, 0.2f, f->life,
                    .08f, .08f, 0);
        }
    }
}

discharge_effect::discharge_effect(
        b2Vec2 start,
        b2Vec2 end,
        float start_z,
        float end_z,
        int num_points,
        float life)
{
    this->_pos = start;

    if (num_points > DISCHARGE_MAX_POINTS)
        num_points = DISCHARGE_MAX_POINTS;
    if (num_points < 3)
        num_points = 3;

    this->num_points = num_points;

    this->start_z = start_z;
    this->end_z = end_z;
    this->p[0] = start;
    this->p[1] = end;
    this->life = life;
    this->shift_dir = (rand()%2) == 0 ? 1.f : -1.f;
    this->line_width = .025f;

    this->cull_effects_method = CULL_EFFECTS_BY_POSITION;
    this->update_method = ENTITY_UPDATE_NULL;
    this->prio = 0;

    for (int x=0; x<this->num_points; x++) {
        this->displ[x] = 0.f;
    }
}

void
discharge_effect::set_points(b2Vec2 start, b2Vec2 end, float start_z, float end_z)
{
    this->p[0] = start;
    this->p[1] = end;
    this->start_z = start_z;
    this->end_z = end_z;
}

void
discharge_effect::mstep()
{
    if (this->life > 0.f) {
        this->life -= G->timemul(WORLD_STEP) *  0.000001f;
    } else {
        G->lock();
        G->absorb(this);
        G->unlock();
    }
}

void
discharge_effect::update_effects()
{
    if (this->life > 0.f) {
        b2Vec2 tangent = this->p[1]-this->p[0];
        float dist = tangent.Length();
        tangent *= 1. / dist;

        b2Vec2 normal = b2Vec2(-tangent.y, tangent.x);

        b2Vec2 last = this->p[0];
        b2Vec2 p;
        float dd = dist / this->num_points;

        float z = this->start_z;
        float shift = cosf(this->life / 2.f) * .25f * this->shift_dir;

        for (int x=0; x<this->num_points+1; x++) {

            if (x == this->num_points) {
                p = this->p[1];
            } else {
                float offs = cosf(((float)(x - this->num_points/2.f) / ((float)this->num_points/2.f)) * M_PI/2.f) * shift;
                float displ = (-.1f + (rand()%100) / 100.f * .2f);
                p = this->p[0] + (float)x * dd * tangent + (displ+offs) * normal;
            }

            linebuffer::add2(last.x, last.y, z, p.x, p.y, z,
                    0.95f, 0.95f, 4.0f, 1.f,
                    0.95f, 0.95f, 4.0f, 1.f,
                    this->line_width, this->line_width);

            last = p;
        }
    }
}

flame_effect::flame_effect(b2Vec2 pos, int layer, int f_type, bool _disable_sound/*=false*/)
    : base_effect()
    , disable_sound(_disable_sound)
{
    this->_pos = pos;
    this->set_layer(layer);
    this->thrustmul = 0.f;
    this->done = false;
    this->flame_n = 0;
    this->f_type = f_type;
    this->thrustmul = 1.f;
    this->z_offset = 0.f;
    this->sep = 0.f;

    this->cull_effects_method = CULL_EFFECTS_DISABLE;
    this->update_method = ENTITY_UPDATE_NULL;

    this->set_flag(ENTITY_DO_MSTEP, false);
    this->set_flag(ENTITY_DO_STEP, true);

    memset(this->flames, 0, NUM_FLAMES*sizeof(struct flame));
}

void
flame_effect::update_pos(b2Vec2 pos, b2Vec2 v)
{
    this->sep = b2Distance(this->_pos, pos);

    this->_pos.x = pos.x;
    this->_pos.y = pos.y;
    this->v.x = v.x;
    this->v.y = v.y;
}

void
flame_effect::step()
{
    bool dead = true;
    for (int x=0; x<NUM_FLAMES; ++x) {
        if (this->flames[x].life > 0.f) {
            dead = false;
            break;
        }
    }

    if (this->done && dead) {
        if (!this->disable_sound) {
            if (this->f_type == 0) {
                sm::stop(&sm::thruster, this);
            } else {
                sm::stop(&sm::rocket, this);
            }
        }

        G->lock();
        G->absorb(this);
        G->unlock();
    } else {
        if (this->flames[this->flame_n%NUM_FLAMES].life <= 0.f && this->thrustmul > 0.f && !this->done) {
            this->flames[this->flame_n%NUM_FLAMES].a = (float)(rand()%100) / 100.f * M_PI*2.f;
            this->flames[this->flame_n%NUM_FLAMES].x = this->_pos.x;
            this->flames[this->flame_n%NUM_FLAMES].y = this->_pos.y;
            this->flames[this->flame_n%NUM_FLAMES].vx = this->v.x;
            this->flames[this->flame_n%NUM_FLAMES].vy = this->v.y;
            if (this->f_type == 0) {
                this->flames[this->flame_n%NUM_FLAMES].s = .1f + ((rand()%100) / 100.f)*.1f;
            } else {
                this->flames[this->flame_n%NUM_FLAMES].s = .5f + ((rand()%100) / 100.f)*.5f;
            }

            this->flames[this->flame_n%NUM_FLAMES].life = 1.f * std::max(1.f-(this->sep / 2.f), 0.1f);

            this->flame_n++;
        }

        if (!this->disable_sound) {
            b2Vec2 pos = this->get_position();

            if (this->thrustmul > 0.f) {
                const float vol = tclampf(this->thrustmul+.3f, 0.f, 1.f);

                if (this->f_type == 0) {
                    G->play_sound(SND_THRUSTER, pos.x, pos.y, 0, vol, true, this);
                } else {
                    G->play_sound(SND_ROCKET, pos.x, pos.y, 0, vol, true, this);
                }
            } else {
                if (this->f_type == 0) {
                    sm::stop(&sm::thruster, this);
                } else {
                    sm::stop(&sm::rocket, this);
                }
            }
        }
    }
}

void
flame_effect::update_effects()
{
    for (int x=0; x<NUM_FLAMES; x++) {
        struct flame *f = &this->flames[x];
        if (f->life > 0.f) {
            spritebuffer::add2(f->x, f->y, this->get_layer()*LAYER_DEPTH+this->z_offset,
                    2.f*f->life, 2.f*f->life, 1.f*f->life, 1.f*f->life,
                    f->s, f->s, 0, f->a);
            //f->x+=f->vx *_tms.dt*4.f * G->get_time_mul();
            //f->y+=f->vy *_tms.dt*4.f * G->get_time_mul();
            f->life-=_tms.dt*5.f;
        }
    }

    if (this->thrustmul > 0.f) {
        b2Vec2 p = this->local_to_world(b2Vec2(0.f,0.f), 0.f);
        float s = 1.8f+(rand()%100)/100.f * .2f;
        s*= (this->f_type == 0 ? .25f : 1.f);
        spritebuffer::add(p.x, p.y, this->get_layer()*LAYER_DEPTH+this->z_offset,
                1.f, 1.f, 1.f, .25f,//.25f+(rand()%100)/100.f * .5f,
                s,s, 2);

        if (this->done) {
            this->thrustmul *= .95f;
        }
    }
}

void
flame_effect::set_thrustmul(float thrustmul)
{
    this->thrustmul = thrustmul;
}

void
flame_effect::set_z_offset(float z_offset)
{
    this->z_offset = z_offset;
}

/** 
 * TESLA
 **/

tesla_effect::tesla_effect(entity *source, b2Vec2 pos, int layer)
    : base_effect()
{
    this->min_angle = 0.f;
    this->max_angle = M_PI*2.f;
    this->_pos = pos;
    this->ignore = source;
    if (source) {
        this->ignore_id = source->id;
    } else {
        this->ignore_id = 0;
    }
    this->prio = layer;
    this->range = 3.f;
    this->cull_effects_method = CULL_EFFECTS_DISABLE;

    this->set_flag(ENTITY_DO_STEP, true);
    this->set_flag(ENTITY_DO_MSTEP, false);

    for (int x=0; x<TESLA_NUM_RAYS; x++) {
        this->rnd[x] = x;
    }
    for (int x=0; x<TESLA_MAX_PATHS; x++) {
        this->paths[x] = new discharge_effect(b2Vec2(0.f,0.f), b2Vec2(0.f, 0.f), 0, 0, 5, 1.f);
        this->paths[x]->line_width = .075f;
    }

    this->num_paths = 0;
}

float32
tesla_effect::ReportFixture(b2Fixture *f, const b2Vec2 &pt, const b2Vec2 &nor, float32 fraction)
{
    entity *e = static_cast<entity*>(f->GetUserData());

    if (f->IsSensor()) return -1;

    if (e) {
        if (e->get_layer() != this->get_layer()) return -1;

        if (e->flag_active(ENTITY_IS_CREATURE) && static_cast<creature*>(e)->is_foot_fixture(f)) {
            return -1;
        }

        if (e->flag_active(ENTITY_IS_MAGNETIC)) {

            if (this->charged_entities.find(e) != this->charged_entities.end()) {
                /* this entity has already been charged */
                return -1;
            }

            this->res_fx = f;
            this->res_pt = pt;
        }
    }

    return fraction;
}

void
tesla_effect::search(b2Vec2 pos)
{
    int num_found = 0;

    if (this->num_paths >= TESLA_MAX_PATHS)
        return;

    float amin;
    float amax;

    if (this->num_paths == 0) {
        amin = this->min_angle;
        amax = this->max_angle;
    } else {
        amin = 0.f;
        amax = M_PI*2.f;
    }
    float step = (amax-amin) / TESLA_NUM_RAYS;


    for (int x=0; x<TESLA_NUM_RAYS; x++) {
        float a = amin + this->rnd[x] * step;

        this->res_fx = 0;
        W->b2->RayCast(this, pos, pos + b2Vec2(this->range * cosf(a), this->range * sinf(a)));

        if (this->res_fx) {
            entity *found = static_cast<entity*>(this->res_fx->GetUserData());
            this->charged_entities.insert(found);

            this->paths[this->num_paths]->set_points(pos, this->res_pt, this->get_layer()*LAYER_DEPTH, this->get_layer()*LAYER_DEPTH);
            this->num_paths++;

            if (rand()%50 == 0) {
                G->emit(new spark_effect(
                            this->res_pt,
                            this->get_layer()
                            ), 0);
                G->emit(new smoke_effect(
                            this->res_pt,
                            this->get_layer(),
                            .5f, .5f
                            ), 0);
            }

            this->search(this->res_pt);
            
            num_found ++;

            if (num_found >= 3 || this->num_paths >= TESLA_MAX_PATHS)
                break;
        }
    }
}


void
tesla_effect::step()
{
    this->num_paths = 0;

    if (rand()%20 == 0) {
        /* randomize order */
        for (int x=0; x<TESLA_NUM_RAYS-1; x++) {
            int y = x + rand() / (RAND_MAX / (TESLA_NUM_RAYS - x) + 1);
            int t = this->rnd[y];
            this->rnd[y] = this->rnd[x];
            this->rnd[x] = t;
        }
    }

    this->search(this->_pos);

    /* apply damage and stuff to effected entities */
    for (std::set<entity*>::iterator i = this->charged_entities.begin();
            i != this->charged_entities.end(); i++) {
        entity *e = (*i);

        if (e == this->ignore)
            continue;

        if (e->flag_active(ENTITY_IS_CREATURE)) {
            creature *c = static_cast<creature*>(e);
            c->damage(.5f, 0, DAMAGE_TYPE_ELECTRICITY, DAMAGE_SOURCE_BULLET, ignore_id);
            G->add_highlight(c, false, 0.5f);
        }

        if (e->g_id == O_ITEM) {
            item *i = static_cast<item*>(e);
            if (i->get_item_type() == ITEM_BULLET) {
                G->timed_absorb(i, .05f);
            }
        }
    }

    /* clear things for the next frame */
    this->charged_entities.clear();
    this->charged_entities.insert(this->ignore);
}

tesla_effect::~tesla_effect()
{
    for (int x=0; x<TESLA_MAX_PATHS; x++) {
        delete this->paths[x];
    }
}

void
tesla_effect::update_effects()
{
    for (int x=0; x<this->num_paths; x++) {
        this->paths[x]->update_effects();
    }
}

/* PLASMA EXPLOSION EFFECT */

plasma_explosion_effect::plasma_explosion_effect(b2Vec2 pos, int layer, float col, float scale) : base_effect()
{
    this->_pos = pos;
    this->scale = scale;
    this->col = col;
    this->trigger_point = pos;
    this->set_layer(layer);
    this->update_method = ENTITY_UPDATE_NULL;
    this->cull_effects_method = CULL_EFFECTS_BY_POSITION;
    struct particle *f = &this->particle;
    f->a = 1.f;
    f->life = 1.f;
    f->x = trigger_point.x;
    f->y = trigger_point.y;
    f->z = this->get_layer()*LAYER_DEPTH;// + (rand()%100) / 200.f;
    f->s = .5f;
}

void
plasma_explosion_effect::mstep()
{
    int num_active = 0;

    struct particle *f = &this->particle;
    if (f->life > 0.f) {
        f->s -= G->timemul(WORLD_STEP) * 0.000004f*scale;
        f->life -= G->timemul(WORLD_STEP) * 0.000004f;

        ++ num_active;
    } else {
        G->lock();
        G->absorb(this);
        G->unlock();
    }
}

void
plasma_explosion_effect::update_effects()
{
    struct particle *f = &this->particle;

    float b = f->life;

    float intensity = .5f;

    if (f->life > 0.f) {
        spritebuffer::add2(
            f->x, f->y, f->z,
            intensity,  1.f*(1.f-b) + intensity*b, intensity*b + (1.f-b)*1.f, f->life,
            f->s*.7f, f->s*.7f, 3, 0.f);
    }
}
