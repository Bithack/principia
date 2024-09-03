#include "scanner.hh"
#include "model.hh"
#include "material.hh"
#include "game.hh"
#include "linebuffer.hh"
#include "spritebuffer.hh"
#include "explosive.hh"
#include "creature.hh"

mirror::mirror()
{
    this->type = ENTITY_PLANK; /* enabnle plank cross-layer conns */
    this->set_mesh(mesh_factory::get_mesh(MODEL_MIRROR));
    this->set_material(&m_gear);
    this->set_as_rect(.5f, .075f);

    this->layer_mask = 1+2+4+8;

    this->query_len = .25f;
}

laser_sensor::laser_sensor()
{
    this->set_mesh(mesh_factory::get_mesh(MODEL_LASERSENSOR));
    this->set_material(&m_edev);
    this->set_as_rect(.125f, .25f);

    this->num_s_out = 1;
    this->num_s_in = 0;

    this->s_out[0].ctype = CABLE_RED;
    this->s_out[0].lpos = b2Vec2(0.f, .07f);

    this->query_vec = b2Vec2(0.f, .35f);
    this->laserhit = false;

    this->set_num_properties(2);
    this->properties[0].type = P_FLT;
    this->properties[0].v.f = 0.f;

    this->properties[1].type = P_INT;
    this->properties[1].v.i = 0;

    this->num_sliders = 2;
}

edevice*
laser_sensor::solve_electronics()
{
    this->s_out[0].write(this->laserhit ? 1.f : 0.f);
    this->laserhit = false;

    return 0;
}

scanner::scanner()
{
    this->set_flag(ENTITY_DO_STEP,              true);
    this->set_flag(ENTITY_DO_TICK,              true);
    this->set_flag(ENTITY_DO_UPDATE_EFFECTS,    true);

    for (int x = 0; x < SCANNER_MAX_POINTS; ++x) {
        this->points[x].Set(0,0);
    }
    this->set_mesh(mesh_factory::get_mesh(MODEL_SCANNER));
    this->set_material(&m_metal);

    this->num_sliders = 1;

    this->num_points = 0;

    this->cull_effects_method = CULL_EFFECTS_DISABLE;

    this->num_s_in = 1;

    this->handler = new scanner::cb_handler(this);
    this->s_in[0].ctype = CABLE_RED;
    this->s_in[0].lpos = b2Vec2(0.f, .2f);

    this->set_num_properties(1);
    this->properties[0].type = P_FLT; /* Wavelength */
    this->properties[0].v.i = 0.f;

    this->set_as_rect(.1f, .335f);
    this->query_sides[2].SetZero(); /* down */
}

scanner::~scanner()
{
    delete this->handler;
}

void
scanner::init()
{
    this->active = (this->s_in[0].p == 0);
}

void
scanner::step()
{
    b2Vec2 pt1 = this->local_to_world(b2Vec2(0.f, -.35f), 0);
    b2Vec2 pt2 = this->local_to_world(b2Vec2(0.f, -SCANNER_REACH), 0);
    b2Vec2 dir = pt2-pt1;
    dir *= 1.f/(dir.Length());

    this->num_points = 0;

    if (W->is_paused() || this->active) {
        while (this->num_points < SCANNER_MAX_POINTS) {
            this->result_fx = 0;

            W->b2->RayCast(this->handler, pt1, pt2);

            if (result_fx) {
                entity *e = static_cast<entity*>(result_fx->GetUserData());

                this->points[num_points] = result_pt;
                this->num_points ++;

                if (e) {
                    if (e->g_id == O_LASER_BOUNCER) {

                        b2Vec2 reflected = dir - b2Dot(dir, result_nor)*2.f*result_nor;

                        pt1 = result_pt;
                        pt2 = pt1 + SCANNER_REACH*reflected;
                        dir = reflected;
                        continue;
                    }

                    if (!W->is_paused()) {
                        float dmg = this->properties[0].v.f * G->get_time_mul();

                        if (e->g_id == O_LASER_SENSOR && (e->properties[1].v.i != 0 || e->world_to_local(result_pt, 0).y < -.1f)
                            && e->properties[0].v.f == this->properties[0].v.f) {
                            ((laser_sensor*)e)->laserhit = true;
                        } else if (e->is_creature()) {
                            if (this->properties[0].v.f > 0.f) {
                                creature *c = static_cast<creature*>(e);
                                c->damage(dmg, 0, DAMAGE_TYPE_PLASMA, DAMAGE_SOURCE_WORLD, 0);
                            }
                        } else if (e->g_id == O_LAND_MINE || e->g_id == O_BOMB) {
                            ((explosive*)e)->damage(dmg);
                        }

                        /* else, handle hit */
                    }
                }
            } else {
                this->points[num_points] = pt2;
                this->num_points ++;
            }

            break;
        }
    }
}

void
scanner::tick()
{
    step();
}

float32
scanner::cb_handler::ReportFixture(b2Fixture *f, const b2Vec2 &pt, const b2Vec2 &nor, float32 fraction)
{
    entity *r = static_cast<entity*>(f->GetUserData());

    if (f->IsSensor()) {
        return -1.f;
    }

    if (r) {
        /* if the layer of the scanner and entity are different, continue scanning */
        if (self->get_layer() != r->get_layer()) return -1.f;
        /* ignore stuff in the outer sublayers */
        if ((r->layer_mask & 6) == 0) return -1.f;
    }

    self->result_nor = nor;
    self->result_pt = pt;
    self->result_fx = f;

    return fraction;
}

void
scanner::update_effects()
{
    float z = this->get_layer() * LAYER_DEPTH;

    b2Vec2 last = this->local_to_world(b2Vec2(0.f, -.35f), 0);

    tvec3 rgb = (tvec3){0.f, 1.f-this->properties[0].v.f, this->properties[0].v.f};

    for (int x=0; x<this->num_points; x++) {
        linebuffer::add2(
                last.x, last.y, z,
                this->points[x].x, this->points[x].y, z,
                rgb.r*3.f+1.f, rgb.g*3.f+1.f, rgb.b*3.f+1.f, 1.f,
                rgb.r*3.f+1.f, rgb.g*3.f+1.f, rgb.b*3.f+1.f, 1.f,
                .03f, .03f);

        last = this->points[x];
    }

    if (this->num_points > 0) {
        spritebuffer::add(last.x, last.y, z, 1.f, 1.f, 1.f, 1.f, .2f, .2f, 1, cos((double)(_tms.last_time + rand()%100000)/100000.) * .25f);
    }
}

edevice*
scanner::solve_electronics()
{
    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();

    this->active = !this->s_in[0].p || (bool)roundf(this->s_in[0].get_value());

    return 0;
}

void
laser_sensor::on_slider_change(int s, float value)
{
    if (s == 0) {
        this->properties[0].v.f = value;
        G->show_numfeed(value);
    } else {
        this->properties[1].v.i = (int)value;
        this->on_load(false, false);
    }
}

void
laser_sensor::on_load(bool created, bool has_state)
{
    if (this->properties[1].v.i == 0) {
        this->set_as_rect(.125f, .25f);
        this->query_vec = b2Vec2(0.f, .35f);
        this->set_mesh(mesh_factory::get_mesh(MODEL_LASERSENSOR));
        this->s_out[0].lpos = b2Vec2(0.f, .07f);
    } else {
        this->set_as_rect(.5f, .2f);
        this->query_vec = b2Vec2(.8f, .0f);
        this->set_mesh(mesh_factory::get_mesh(MODEL_IMPACT0));
        this->s_out[0].lpos = b2Vec2(.25f, .00f);
    }

    this->recreate_shape();
}

void
scanner::on_slider_change(int s, float value)
{
    this->properties[0].v.f = value;
    G->show_numfeed(value);
}


void
scanner::write_quickinfo(char *out)
{
    sprintf(out, "%s (wavelength: %f)", this->get_name(), this->properties[0].v.f);
}
