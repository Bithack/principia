#include "command.hh"
#include "robot_base.hh"
#include "material.hh"
#include "world.hh"
#include "model.hh"
#include "game.hh"
#include "ui.hh"

#define COL .2f
#define SPEED_FACTOR 20.f

command::command()
    : last_apply(0.f)
    , do_apply(false)
    , active(false)
    , target(0)
{
    this->set_flag(ENTITY_IS_STATIC,            true);
    this->set_flag(ENTITY_DO_STEP,              true);
    this->set_flag(ENTITY_ALLOW_CONNECTIONS,    false);
    this->set_flag(ENTITY_HAS_CONFIG,           true);

    this->dialog_id = DIALOG_SET_COMMAND;

    this->update_method = ENTITY_UPDATE_STATIC;

    this->set_mesh(mesh_factory::get_mesh(MODEL_CPAD));
    this->set_material(&m_cpad);

    tmat4_load_identity(this->M);
    tmat3_load_identity(this->N);

    this->set_num_properties(2);
    this->properties[1].type = P_FLT;
    this->set_property(0, (uint32_t)COMMAND_LEFTRIGHT); /* command type, i.e. COMMAND_STOP, COMMAND_JUMP */
    this->set_property(1, .5f); /* command variable, i.e. jump height. aim angle, speed increase */

    this->num_s_in = 1;

    this->s_in[0].lpos = b2Vec2(.125f, -0.375f);

    this->do_apply = false;
    this->target = 0;

    this->set_command(this->properties[0].v.i, false);
}

void
command::on_load(bool created, bool has_state)
{
    this->set_command(this->properties[0].v.i, false);
}

void
command::step()
{
    if (this->last_apply > 0.f)
        this->last_apply -= .02f;

    if (this->do_apply && this->target && this->apply_command(this->target)) {
        this->do_apply = false;
        this->target = 0;
    }

    entity::step();
}

edevice*
command::solve_electronics()
{
    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();

    if (this->s_in[0].p == 0) {
        /* cable is unplugged, default to ACTIVE */
        this->active = true;
    } else {
        this->active = (bool)roundf(this->s_in[0].get_value());
    }

    return 0;
}

bool
command::apply_command(robot_base *r)
{
    switch (this->cmd) {
        case COMMAND_STARTSTOP: if (this->active) r->go(); else r->stop(); return false;
    }

    if (this->last_apply <= 0.f && this->active) {
        switch (this->cmd) {
            case COMMAND_STOP: r->stop(); break;
            //case COMMAND_STARTSTOP: if (r->stopped) r->go(); else r->stop(); break;
            case COMMAND_LEFT: r->dir = DIR_LEFT; r->look(r->dir); break;
            case COMMAND_RIGHT: r->dir = DIR_RIGHT; r->look(r->dir); break;
            case COMMAND_LEFTRIGHT: r->dir = (r->dir == DIR_LEFT ? DIR_RIGHT : DIR_LEFT); r->look(r->dir); break;
            case COMMAND_JUMP: r->jump(false, 0.5f + this->properties[1].v.f); break;
            case COMMAND_AIM:
                {
                    float a = this->properties[1].v.f;
                    /* offset and wrap the angle */
                    a = twrapf(a - 1.00f, -1.f, 1.f);

                    /* convert a value between -1 and 1 to a value the robot can work with */
                    a *= (M_PI*2.f);

                    a -= r->get_angle();

                    r->aim(a);
                }
                break;
            case COMMAND_ATTACK: r->attack(); break;
            case COMMAND_LAYERUP: r->layermove(+1); break;
            case COMMAND_LAYERDOWN: r->layermove(-1); break;
            case COMMAND_INCRSPEED: r->set_speed(r->get_speed() + (this->properties[1].v.f * SPEED_FACTOR)); break;
            case COMMAND_DECRSPEED: r->set_speed(r->get_speed() - (this->properties[1].v.f * SPEED_FACTOR)); break;
            case COMMAND_SETSPEED: r->set_speed(this->properties[1].v.f); break;
            case COMMAND_HEALTH: r->set_hp(r->get_max_hp()); break;
        }

        this->last_apply = 1.f;

        G->add_highlight(this, false);

        return true;
    }

    return false;
}

void
command::on_touch(b2Fixture *my, b2Fixture *other)
{
    entity *o = static_cast<entity*>(other->GetUserData());

    if (o && o->flag_active(ENTITY_IS_ROBOT)) {
        robot_base *r = static_cast<robot_base*>(o);
        if (other == r->get_sensor_fixture()) {
            this->do_apply = true;
            this->target = r;
        }
    }
}

void
command::on_untouch(b2Fixture *my, b2Fixture *other)
{
    entity *o = static_cast<entity*>(other->GetUserData());

    if (o && o->flag_active(ENTITY_IS_ROBOT)) {
        robot_base *r = static_cast<robot_base*>(o);
        if (other == r->get_sensor_fixture()) {
            this->do_apply = false;
            this->target = 0;
        }
    }
}

void
command::add_to_world()
{
    this->last_apply = 0.f;

    b2BodyDef bd;
    bd.type = b2_staticBody;
    bd.position = _pos;
    bd.angle = _angle;

    b2PolygonShape box;
    box.SetAsBox(.5f, .25f);

    b2FixtureDef fd;
    fd.shape = &box;
    fd.density = 0.f;
    fd.friction = .5f;
    fd.restitution = .3f;
    fd.filter = world::get_filter_for_layer(this->get_layer(), 15);

    b2Body *b = W->b2->CreateBody(&bd);
    (b->CreateFixture(&fd))->SetUserData(this);


    box.SetAsBox(.25f, .125f, b2Vec2(0.f, -.375f), 0.f);
    (b->CreateFixture(&fd))->SetUserData(this);

    if (!W->is_paused()) {
        b2PolygonShape sensor_shape;
        sensor_shape.SetAsBox(.1f, .15f, b2Vec2(0, .55f), 0);

        b2FixtureDef sfd;
        sfd.shape = &sensor_shape;
        sfd.density = .0f;
        sfd.isSensor = true;
        sfd.filter = world::get_filter_for_layer(this->get_layer(), 15);

        (b->CreateFixture(&sfd))->SetUserData(this);
    }

    this->body = b;
}

void
command::set_command(int cmd, bool reset_property /*=true*/)
{
    this->cmd = cmd;
    if (this->cmd > 16) this->cmd = 16;
    this->set_property(0, (uint32_t)cmd);

    if (reset_property) {
        this->properties[1].v.f = 0.5f;
    }

    switch (this->cmd) {
        case COMMAND_STOP:
        case COMMAND_STARTSTOP:
        case COMMAND_LEFT:
        case COMMAND_RIGHT:
        case COMMAND_LEFTRIGHT:
        case COMMAND_LAYERUP:
        case COMMAND_LAYERDOWN:
        case COMMAND_HEALTH:
        case COMMAND_ATTACK:
            this->num_sliders = 0;
            break;

        case COMMAND_AIM:
        case COMMAND_JUMP:
        case COMMAND_INCRSPEED:
        case COMMAND_DECRSPEED:
        case COMMAND_SETSPEED:
            this->num_sliders = 1;
            break;
    }

    this->set_mesh(mesh_factory::get_mesh(MODEL_CPAD+this->cmd));
}

float
command::get_slider_snap(int s)
{
    switch (this->cmd) {
        case COMMAND_AIM:       return 1.f / 20.f;
        case COMMAND_JUMP:      return 1.f / 10.f;
        case COMMAND_INCRSPEED: return 1.f / 20.f;
        case COMMAND_DECRSPEED: return 1.f / 20.f;
        case COMMAND_SETSPEED:  return 1.f / CREATURE_MAX_SPEED;
    }
    return 0.f;
}

float
command::get_slider_value(int s)
{
    switch (this->cmd) {
        case COMMAND_AIM:       return this->properties[1].v.f;
        case COMMAND_JUMP:      return this->properties[1].v.f;
        case COMMAND_INCRSPEED: return this->properties[1].v.f;
        case COMMAND_DECRSPEED: return this->properties[1].v.f;
        case COMMAND_SETSPEED:  return this->properties[1].v.f / CREATURE_MAX_SPEED;
    }

    return .0f;
}

void
command::on_slider_change(int s, float value)
{
    switch (this->cmd) {
        case COMMAND_AIM:
            this->set_property(1, value);
            G->show_numfeed(value);
            break;

        case COMMAND_JUMP:
            this->set_property(1, value);
            G->show_numfeed(value + 0.5f);
            break;

        case COMMAND_INCRSPEED:
        case COMMAND_DECRSPEED:
            this->set_property(1, value);
            G->show_numfeed(value * SPEED_FACTOR);
            break;
        case COMMAND_SETSPEED:
            this->set_property(1, value * CREATURE_MAX_SPEED);
            G->show_numfeed(value * CREATURE_MAX_SPEED);
            break;

        default:
            tms_fatalf("Command pad not implemented (%d)", this->cmd);
            break;
    }
}

const char *command_strings[NUM_COMMANDS] = {
    "Stop",
    "Start/Stop",
    "Left",
    "Right",
    "Left/Right",
    "Jump",
    "Aim",
    "Attack",
    "Layer up",
    "Layer down",
    "Increase speed",
    "Decrease speed",
    "Set speed",
    "Full health",
};

void
command::write_quickinfo(char *out)
{
    if (this->cmd < NUM_COMMANDS) {
        if (this->cmd == COMMAND_JUMP)
            sprintf(out, "%s (%s)", this->get_name(), command_strings[this->cmd]);
        else
            sprintf(out, "%s (%s %.2f)", this->get_name(), command_strings[this->cmd], .5f+this->properties[1].v.f);
    } else
        sprintf(out, "%s", this->get_name());
}
