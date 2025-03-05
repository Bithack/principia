#include "spikebot.hh"
#include "model.hh"
#include "game.hh"

static const float SPIKEBOT_BASE_RADIUS = 0.65f;

spikebot::spikebot(float scale/*=1.f*/)
{
    this->m_scale = scale;

    this->body_angle = 0.f;
    this->angry = false;
    this->width  = SPIKEBOT_BASE_RADIUS * this->get_scale();
    this->height = SPIKEBOT_BASE_RADIUS * this->get_scale();
    this->robot_type = ROBOT_TYPE_SPIKEBOT;
    this->damage_multiplier = 3.f + (6.f * (1.f-this->get_scale()));

    this->properties[ROBOT_PROPERTY_FEET].v.i8 = FEET_BIPED;
    this->properties[ROBOT_PROPERTY_SPEED].v.f = ROBOT_DEFAULT_SPEED; /* Robot speed */
    this->properties[ROBOT_PROPERTY_STATE].v.i8 = CREATURE_IDLE; /* Robot state */
    this->properties[ROBOT_PROPERTY_ROAMING].v.i8 = 1; /* Roaming */

    this->body_shape = static_cast<b2Shape*>(new b2CircleShape());
    ((b2CircleShape*)this->body_shape)->m_radius = SPIKEBOT_BASE_RADIUS;

    ((b2CircleShape*)this->body_shape)->Scale(this->get_scale());

    this->feet_offset *= this->get_scale(); // ? :-)

    this->e_body = new tms::entity();
    this->e_body->set_mesh(mesh_factory::get_mesh(MODEL_SPIKEBOT));
    this->e_body->set_material(&m_item);
    this->add_child(e_body);

    this->circuits_compat = CREATURE_CIRCUIT_RIDING;
}

void
spikebot::setup()
{
    robot_base::setup();

    this->body_angle = 0.f;
    this->angry = false;
}

void
spikebot::action_on()
{
    robot_base::action_on();

    this->angry = true;
    G->add_highlight(this, false, 0.4f);
}

void
spikebot::action_off()
{
    robot_base::action_off();

    this->angry = false;
    G->add_highlight(this, false, 0.4f);
}

void
spikebot::step()
{
    robot_base::step();

    if (!this->is_dead()) {
        if (this->angry) {
            this->body_angle += 0.1f;
        } else {
            this->body_angle += 0.025f;
        }
    }
}

void
spikebot::update()
{
    b2Transform t;
    if (this->body != 0) {
        t = this->body->GetTransform();
    } else {
        t.p.x = this->_pos.x;
        t.p.y = this->_pos.y;
    }

    float tilt = 0.f;
    if (this->feet) tilt = this->feet->get_tilt();

    tmat4_load_identity(this->M);
    tmat4_translate(this->M, t.p.x, t.p.y, this->get_z());
    tmat4_rotate(this->M, this->get_angle()*(180.f/M_PI), 0, 0, -1);
    tmat4_rotate(this->M, -90 * this->i_dir + tilt * (180.f/M_PI), 0, 1, 0);
    tmat3_copy_mat4_sub3x3(this->N, this->M);

    tmat4_copy(this->e_body->M, this->M);
    tmat4_rotate(this->e_body->M, this->body_angle*(180.f/M_PI), 1, 0, 0);
    tmat3_copy_mat4_sub3x3(this->e_body->N, this->e_body->M);

    if (this->get_scale() != 1.f) {
        tmat4_scale(this->M, this->get_scale(), this->get_scale(), this->get_scale());
        tmat4_scale(this->e_body->M, this->get_scale(), this->get_scale(), this->get_scale());
    }

    creature::update();
}

void
spikebot::on_death()
{
    robot_base::on_death();
}

void
spikebot::roam_attack()
{
    if (this->target_dist < 4.f) {
        this->set_speed(this->properties[0].v.f+10.f);
        this->angry = true;
    }
}

void
spikebot::roam_update_dir()
{
    b2Vec2 r = this->get_position();
    b2Vec2 target_pos = this->roam_target->get_position();

    /* move toward the target */
    if (this->get_tangent_distance(target_pos) < 0.f) {
        new_dir = DIR_LEFT;
    } else {
        new_dir = DIR_RIGHT;
    }
}

float
spikebot::get_damage()
{
    return 0.25f * G->get_time_mul() * this->get_scale();
}
