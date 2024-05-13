#include "bomber.hh"
#include "model.hh"
#include "game.hh"
#include "explosive.hh"

#define SHOOT_VELOCITY 25.f

bomber::bomber()
{
    this->f_outer = 0;

    this->robot_type = ROBOT_TYPE_BOMBER;
    this->damage_multiplier = 0.75f;

    this->set_mesh(mesh_factory::get_mesh(MODEL_BOMBER));
    this->set_material(&m_robot2);

    this->properties[0].v.f = 10.f;
    this->properties[1].v.i8 = CREATURE_IDLE;
    this->properties[3].v.i = 1500;

    this->properties[ROBOT_PROPERTY_FEET].v.i8 = FEET_MINIWHEELS;

    this->body_shape = static_cast<b2Shape*>(new b2PolygonShape());
    ((b2PolygonShape*)this->body_shape)->SetAsBox(1.25f/2.f-.1f, 1.25f/2.f, b2Vec2(0.f, .4f), 0.f);
    this->width = 1.25f/2.f;

    this->attack_timer = 0;
}

void
bomber::set_layer(int l)
{
    robot_base::set_layer(l);

    if (this->f_body)
        this->f_body->SetFilterData(world::get_filter_for_layer(l, 2+4));
    if (this->f_outer)
        this->f_outer->SetFilterData(world::get_filter_for_layer(l, 1+8));
}

void
bomber::roam_aim()
{
    b2Vec2 r = this->get_position();
    b2Vec2 o = (this->roam_target_type == TARGET_POSITION ? this->roam_target_pos : this->roam_target->get_position());
    o -= r;

    float a = atan2f(o.y, o.x);

    a -= this->get_angle();

    /* XXX: This needs to take gravity into account */
    if (this->look_dir == 1)
        a += (this->target_dist * 0.025f);
    else
        a -= (this->target_dist * 0.025f);

    this->roam_target_aim = a;
}

void
bomber::roam_attack()
{
    if (this->target_dist < 3.f) {
        this->set_speed(this->properties[0].v.f + 7.5f);
    } else {
        this->set_speed(this->properties[0].v.f);
    }

    b2Vec2 r = this->get_position();
    b2Vec2 target_pos = (this->roam_target_type == TARGET_POSITION ? this->roam_target_pos : this->roam_target->get_position());

    if (this->get_layer() == target_layer
            && this->target_dist < 9.f) {
        this->shoot_target = false;
        W->b2->RayCast(this->handler, r, target_pos);
        if (this->shoot_target) {
            this->attack((this->properties[3].v.i + rand()%50)*1000);
        }
    }
}

void
bomber::step()
{
    robot_base::step();

    if (this->attack_timer > 0) {
        this->attack_timer -= G->timemul(WORLD_STEP);
    } else {
        this->attack_timer = 0;
    }
}

void
bomber::attack(int add_cooldown)
{
    if (this->finished) return;
    if (roundf(this->i_dir) == 0.f) return;
    if (this->attack_timer > 0) return;

    float dir = tclampf(roundf(this->i_dir), -1.f, 1.f);

    b2Vec2 v = this->get_tangent_vector(dir);
    v *= -1.0f;
    this->body->ApplyLinearImpulse(v, this->body->GetWorldPoint(b2Vec2(0,.25f)));

    float a = atan2f(v.y, v.x);

    explosive *expl = (explosive*)of::create(O_LAND_MINE);
    expl->set_layer(this->get_layer());
    expl->_pos = this->local_to_world(b2Vec2(dir*.5f, 0.6f), 0);
    expl->_angle = a+M_PI/2.f;

    v *= -SHOOT_VELOCITY;
    b2Vec2 p = this->get_position();

    G->lock();
    G->emit(expl, this, v+this->body->GetLinearVelocity());
    G->play_sound(SND_ROBOT_BOMB, p.x, p.y, 0, 1.f);
    G->unlock();

    this->attack_timer = BOMBER_RELOAD_TIME;
    this->attack_timer += add_cooldown;
}

