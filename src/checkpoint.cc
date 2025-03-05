#include "checkpoint.hh"
#include "adventure.hh"
#include "game.hh"
#include "material.hh"
#include "model.hh"

checkpoint::checkpoint()
    : spawned(false)
    , activated(false)
{
    this->set_mesh(mesh_factory::get_mesh(MODEL_CHECKPOINT));
    this->set_material(&m_pv_colored);
    this->set_uniform("~color", CHECKPOINT_COLOR_INACTIVE);

    //this->query_vec = b2Vec2(0.f, -.5f);

    this->set_flag(ENTITY_MUST_BE_DYNAMIC, true);

    this->num_s_out = 2;
    this->num_s_in = 1;

    this->s_in[0].ctype = CABLE_RED;
    this->s_in[0].lpos = b2Vec2(-.25f, .0f);

    this->s_out[0].ctype = CABLE_RED;
    this->s_out[0].lpos = b2Vec2(.0f, .0f);

    this->s_out[1].ctype = CABLE_RED;
    this->s_out[1].lpos = b2Vec2(.25f, .0f);

    this->scaleselect = true;

    this->height = .15f;
    float qw = (.5f)/2.f+0.15f;
    float qh = (this->height/2.f)/2.f+.15f;
    this->query_sides[0].Set(0.f,  0.f); /* up */
    this->query_sides[1].Set(-qw, 0.f); /* left */
    this->query_sides[2].Set(0.f, -qh); /* down */
    this->query_sides[3].Set( qw, 0.f); /* right */
}

void
checkpoint::add_to_world()
{
    this->create_rect(this->get_dynamic_type(), .5f, .15f, this->material);

    if (!W->is_paused()) {
        b2PolygonShape sh;
        sh.SetAsBox(.5f, 1.f, b2Vec2(0.f, .25f+.5f), 0.f);

        b2FixtureDef fd;
        fd.isSensor = true;
        fd.density = 0.0001f;
        fd.shape = &sh;
        fd.filter = world::get_filter_for_layer(this->get_layer(), 15);

        this->body->CreateFixture(&fd)->SetUserData(this);
    }
}

void
checkpoint::on_touch(b2Fixture *my, b2Fixture *other)
{
    entity *e = static_cast<entity*>(other->GetUserData());

    if (!other->IsSensor() && e && e->is_creature()) {
        /* player robot touched the checkpoint */
        ((creature*)e)->set_checkpoint(this);
    }
}

edevice*
checkpoint::solve_electronics(void)
{
    if (!this->s_in[0].is_ready()) {
        return this->s_in[0].get_connected_edevice();
    }

    bool do_activate = (bool)roundf(this->s_in[0].get_value());

    if (do_activate) {
        /* register as the current checkpoint in game */
        if (adventure::player) {
            adventure::player->set_checkpoint(this);
        } else {
            adventure::checkpoint_activated(this);
        }
    }

    this->s_out[0].write((float)this->spawned);
    this->s_out[1].write((float)this->activated);
    this->spawned = false;
    this->activated = false;

    return 0;
}

