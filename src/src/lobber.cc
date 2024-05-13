#include "lobber.hh"
#include "model.hh"
#include "game.hh"
#include "explosive.hh"

#define SHOOT_VELOCITY 10.f

lobber::lobber()
{
    this->robot_type = ROBOT_TYPE_BOMBER;
    this->damage_multiplier = 0.5f;

    this->set_mesh(mesh_factory::get_mesh(MODEL_LOBBER));
    this->set_material(&m_edev_dark);

    this->properties[0].v.f = 10.f;
    this->properties[1].v.i8 = CREATURE_IDLE;
    this->properties[3].v.i = 1500;
}

void
lobber::roam_aim()
{
    b2Vec2 r = this->get_position();
    b2Vec2 o = this->roam_target->get_position();
    o -= r;

    float a = atan2f(o.y, o.x);

    a -= this->get_angle();

    float d = this->target_dist;
    if (d < 3.f) d = 3.f;
    if (d > 6.f) d = 6.f;
    if (this->look_dir == 1) {
        a += d * 0.225f;
    } else {
        a -= d * 0.225f;
    }

    this->roam_target_aim = a;

    /*

    if (a < M_PI/2.f && a > -M_PI/2.f) {
        if (this->look_dir == 1) {
            this->aim((a + M_PI/2.f)/M_PI);
        } else {
            if (a < 0.f) a+= M_PI*2.f;
            a -= M_PI/2.f;
            this->aim((M_PI-a)/M_PI);
        }
    } else {
        if (this->look_dir == -1) {
            if (a < 0.f) a+= M_PI*2.f;
            a -= M_PI/2.f;
            this->aim((M_PI-a)/M_PI);
        } else {
            this->aim((a + M_PI/2.f)/M_PI);
        }
    }
    */
}

void
lobber::attack(int add_cooldown)
{
    if (this->finished) return;
    if (roundf(this->i_dir) == 0.f) return;
    if (this->attack_timer > 0) return;

    float dir = tclampf(roundf(this->i_dir), -1.f, 1.f);

    b2Vec2 v = b2Vec2(.5f, 1.f);
    v.Normalize();
    v = this->get_body(0)->GetWorldVector(b2Vec2(dir*v.x, v.y));
    v *= -1.0f;
    this->body->ApplyLinearImpulse(v, this->body->GetWorldPoint(b2Vec2(0,.0f)));

    float a = atan2f(v.y, v.x);

    explosive *expl = (explosive*)of::create(O_BOMB);
    expl->set_layer(this->get_layer());
    expl->_pos = this->local_to_world(b2Vec2(dir*.5f, 0.6f), 0);
    expl->_angle = a+M_PI/2.f;
    expl->properties[0].v.i = 1500; /* fuse timer */

    v *= -SHOOT_VELOCITY;
    b2Vec2 p = this->get_position();

    G->lock();
    G->emit(expl, this, v+this->body->GetLinearVelocity());
    G->play_sound(SND_ROBOT_BOMB, p.x, p.y, 0, 1.f);
    G->unlock();

    this->attack_timer = LOBBER_RELOAD_TIME;
    this->attack_timer += add_cooldown;
}
