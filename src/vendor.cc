#include "vendor.hh"
#include "game.hh"
#include "model.hh"
#include "object_factory.hh"
#include "resource.hh"
#include "item.hh"
#include "ui.hh"

/**
 * Sockets:
 * IN0 = Active
 * IN1 = Reset
 *
 * OUT0 = Bought one item
 * OUT1 = Fraction of items remaining
 **/
vendor::vendor()
{
    this->set_mesh(mesh_factory::get_mesh(MODEL_VENDOR));
    this->set_material(&m_iomisc);

    this->num_s_in  = 2;
    this->num_s_out = 2;

    this->s_in[0].ctype = CABLE_RED;
    this->s_in[0].lpos = b2Vec2(-.15f, -.12f);
    this->s_in[1].ctype = CABLE_RED;
    this->s_in[1].lpos = b2Vec2( .15f, -.12f);

    this->s_out[0].ctype = CABLE_RED;
    this->s_out[0].lpos = b2Vec2(-.15f, .12f);
    this->s_out[1].ctype = CABLE_RED;
    this->s_out[1].lpos = b2Vec2( .15f, .12f);

    this->set_as_rect(.375f, .25f);

    this->set_flag(ENTITY_HAS_TRACKER,  true);
    this->set_flag(ENTITY_HAS_CONFIG,   true);
    this->set_flag(ENTITY_DO_STEP,      true);

    this->dialog_id = DIALOG_VENDOR;

    this->set_num_properties(3);
    this->properties[0].type = P_INT; // currency g_id (i.e. O_RESOURCE)
    this->properties[1].type = P_INT; // currency type (i.e. RESOURCE_RUBY)
    this->properties[2].type = P_INT; // num items required
    this->properties[0].v.i = 0;
    this->properties[1].v.i = 0;
    this->properties[2].v.i = 0;

    this->num_sliders = 1;
}

void
vendor::init()
{
    if (this->get_body(0)) {
        b2Body *b = this->get_body(0);

        b2PolygonShape sh;
        sh.SetAsBox(this->width * .8f, .05f, b2Vec2(0, this->width + .05f), 0);

        b2FixtureDef fd;
        fd.isSensor = true;
        fd.shape = &sh;
        fd.density = 0.00000001f;
        fd.friction = FLT_EPSILON;
        fd.restitution = 0.f;
        fd.filter = world::get_filter_for_layer(this->get_layer(), 15);

        (this->absorb_sensor = b->CreateFixture(&fd))->SetUserData(this);
    }
}

void
vendor::setup()
{
    this->deposited = 0;
    this->num_bought = 0;
    this->active = false;
    this->do_reset = false;
    this->last_bought = false;
}

void
vendor::on_touch(b2Fixture *my, b2Fixture *other)
{
    // make sure we accept currencies in our current state
    if (!this->active) return;
    if (this->properties[0].v.i == 0) {
        // no currency selected
        return;
    }

    if (my == this->absorb_sensor) {
        entity *e = static_cast<entity*>(other->GetUserData());

        if (e) {
            if (e->g_id == this->properties[0].v.i) {
                uint32_t type = 0;
                if (e->g_id == O_ITEM) {
                    type = ((item*)e)->get_item_type();
                } else if (e->g_id == O_RESOURCE) {
                    type = ((resource*)e)->get_resource_type();
                }

                if (type == this->properties[1].v.i) {
                    // XXX: Should interaction be required?
                    if (G->interacting_with(e) && G->absorb(e)) {
                        int num = 1;
                        if (e->g_id == O_RESOURCE) {
                            num = ((resource*)e)->get_amount();
                        }

                        if (e->get_body(0)) e->get_body(0)->SetLinearVelocity(b2Vec2(0,0));

                        this->deposited += num;

                        G->play_sound(SND_CASH_REGISTER, this->get_position().x, this->get_position().y, 0, 1.f);
                    }
                }
            }
        }
    }
}

void
vendor::on_slider_change(int s, float value)
{
    this->properties[2].v.i = (1+(uint32_t)roundf(value * 14.f));
    G->show_numfeed(this->properties[2].v.i);
}

void
vendor::step()
{
    if (this->do_reset) {
        // reset stuff
        this->deposited = 0;
        this->do_reset = false;
    }

    if (this->num_bought == 0 && this->deposited >= this->properties[2].v.i) {
        this->deposited -= this->properties[2].v.i;

        ++ this->num_bought;
    }
}

edevice*
vendor::solve_electronics()
{
    if (!this->s_out[0].written()) {
        if (this->num_bought && !this->last_bought) {
            this->last_bought = true;
            this->s_out[0].write(1.f);
            -- this->num_bought;
        } else {
            this->s_out[0].write(0.f);
            this->last_bought = false;
        }
    }

    if (!this->s_out[1].written()) {
        float frac = tclampf((float)this->deposited / this->properties[2].v.i, 0.f, 1.f);
        this->s_out[1].write(frac);
    }

    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();

    if (!this->s_in[1].is_ready())
        return this->s_in[1].get_connected_edevice();

    if (this->s_in[0].p == 0) {
        this->active = true;
    } else {
        this->active = (bool)((int)roundf(this->s_in[0].get_value()));
    }

    this->do_reset = (bool)((int)roundf(this->s_in[1].get_value()));

    return 0;
}
