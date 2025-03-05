#include "fan.hh"
#include "material.hh"
#include "model.hh"
#include "game.hh"

#define NUM_RAYS    5
#define RAY_LENGTH  10.f
#define FAN_WIDTH   1.f

#define FORCE 2.f

fan::fan()
    : blade_rot(0.f)
    , blade_rot_speed(0.f)
    , force(0.f)
{
    this->set_flag(ENTITY_DO_STEP, true);
    this->set_flag(ENTITY_IS_MAGNETIC, true);

    this->set_mesh(mesh_factory::get_mesh(MODEL_FAN));
    this->set_material(&m_pv_colored);

    this->set_uniform("~color", .7f, .2f, .2f, 1.f);

    this->update_method = ENTITY_UPDATE_CUSTOM;

    this->num_s_in = 1;
    this->num_s_out = 0;

    this->s_in[0].ctype = CABLE_BLACK;
    this->s_in[0].lpos = b2Vec2(.0f, .10f);

    this->handler = new fan::cb_handler(this);

    this->set_as_rect(FAN_WIDTH/2.f, .25f);

    this->query_sides[0].SetZero(); /* disable up */

    tms_entity_init(&this->blades);
    tms_entity_set_mesh(&this->blades, mesh_factory::get_mesh(MODEL_FAN_BLADES));
    tms_entity_set_material(&this->blades, &m_pv_colored);
    tms_entity_set_uniform4f(&this->blades, "~color", .7f, .2f, .2f, 1.f);

    tms_entity_add_child(this, &this->blades);
}

void
fan::step()
{
    if (this->force == 0.f) return;

    b2Vec2 fan_position = this->get_position();

    float a = this->get_angle();
    float a_processed = this->get_angle() + M_PI/2.f;

    b2Vec2 angle(cos(a), sin(a));
    b2Vec2 angle_processed(cos(a_processed), sin(a_processed));

    for (int x=0; x<NUM_RAYS; x++) {
        b2Vec2 ray_position = fan_position;
        b2Vec2 dir = this->local_to_world(b2Vec2(0.f, RAY_LENGTH), 0);

        ray_position.x  += angle.x * (((x / ((float)NUM_RAYS - 1.f)) * FAN_WIDTH) - (FAN_WIDTH / 2.f));
        ray_position.y  += angle.y * (((x / ((float)NUM_RAYS - 1.f)) * FAN_WIDTH) - (FAN_WIDTH / 2.f));
        dir.x           += angle.x * (((x / ((float)NUM_RAYS - 1.f)) * FAN_WIDTH) - (FAN_WIDTH / 2.f));
        dir.y           += angle.y * (((x / ((float)NUM_RAYS - 1.f)) * FAN_WIDTH) - (FAN_WIDTH / 2.f));

        this->found = 0;

        W->b2->RayCast(this->handler, ray_position, dir);

        if (this->found) {
            float dist = (this->found_pt - ray_position).Length();
            b2Body *b = this->found->GetBody();

            if (dist < 0.001f) {
                dist = 0.001f;
            }

            b2Vec2 rr = angle_processed;
            //b2Vec2 rr = dir;

            rr.x *= this->force * fminf(1.f, 1.f/dist);
            rr.y *= this->force * fminf(1.f, 1.f/dist);

            rr.x = rr.x;
            rr.y = rr.y;

            if (b->GetType() == b2_dynamicBody) {
                b->ApplyForce(rr, this->found_pt);
            }
        }
    }

    b2Vec2 dir = angle_processed;
    dir.x *= this->force*NUM_RAYS / 2.f;
    dir.y *= this->force*NUM_RAYS / 2.f;

    dir.x = -dir.x;
    dir.y = -dir.y;

    //tms_infof("center: %.2f/%.2f.   pos: %.2f/%.2f", c.x, c.y, x.x, x.y);
    //tms_infof("applying force: %.2f/%.2f", dir.x, dir.y);

    //this->get_body(0)->ApplyForceToCenter(dir);
    this->get_body(0)->ApplyForce(dir, this->local_to_world(b2Vec2(0.f, 0.f), 0), false);

    /* apply inverted force to self */
}

void
fan::update()
{
    this->easy_update();

    tmat4_copy(this->blades.M, this->M);
    tmat3_copy(this->blades.N, this->N);

    /* Blade rotation */
    tmat4_rotate(this->blades.M, this->blade_rot, 0, 1, 0);

    static const float fblend = .01f;
    static const float slowdown = .99f;

    /* XXX: We can add a max to the force*fblend with a "max accelration" to make it accelrate slower. */
    this->blade_rot_speed = this->blade_rot_speed + this->force * fblend;
    this->blade_rot_speed *= slowdown;

    this->blade_rot += this->blade_rot_speed;
}

edevice*
fan::solve_electronics()
{
    if (!this->s_in[0].is_ready()) {
        return this->s_in[0].get_connected_edevice();
    }

    this->force = (this->s_in[0].get_value() * FORCE) / (float)NUM_RAYS;

    return 0;
}

float32
fan::cb_handler::ReportFixture(b2Fixture *f, const b2Vec2 &pt, const b2Vec2 &nor, float32 fraction)
{
    if (f->IsSensor()) {
        return -1.f;
    }

    if (f != this->f->fx
            && (f->GetFilterData().categoryBits & (15 << (this->f->get_layer()*4)))) {
        this->f->found = f;
        this->f->found_pt = pt;

        return fraction;
    }

    return -1.f;
}
