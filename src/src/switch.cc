#include "switch.hh"
#include "material.hh"
#include "model.hh"
#include "ledbuffer.hh"

switcher::switcher()
    : n_out(0)
{
    this->set_flag(ENTITY_DO_UPDATE_EFFECTS, true);

    this->scaleselect = true;

    this->num_s_out = 5;
    this->num_s_in = 3;

    for (int x=0; x<5; x++) {
        this->s_out[x].lpos = b2Vec2(-.2f, .6f-.3*(4-x));
        this->s_out[x].ctype = CABLE_RED;
        this->s_out[x].angle = M_PI;
    }

    this->s_out[1].abias = -.185f;
    this->s_out[3].abias = .185f;

    this->s_in[0].lpos = b2Vec2(.2f, -.6f);
    this->s_in[0].ctype = CABLE_RED;
    this->s_in[0].angle = 0;

    this->s_in[1].lpos = b2Vec2(.2f, .150f);
    this->s_in[1].ctype = CABLE_RED;
    this->s_in[1].angle = 0;
    this->s_in[1].tag = SOCK_TAG_RIGHT;

    this->s_in[2].lpos = b2Vec2(.2f, -.150f);
    this->s_in[2].ctype = CABLE_RED;
    this->s_in[2].angle = 0;
    this->s_in[2].tag = SOCK_TAG_LEFT;

    this->set_material(&m_edev);
    this->set_mesh(mesh_factory::get_mesh(MODEL_SWITCHER));

    this->set_as_rect(.375f, 0.75f);
}

void
switcher::setup()
{
    this->n_out = 0;
}

void
switcher::update_effects()
{
    float z = this->get_layer() * LAYER_DEPTH + LED_Z_OFFSET;

    for (int x=0; x<5; x++) {
        b2Vec2 p = this->local_to_world(b2Vec2(-.0f, .6-.3*(4-x)), 0);
        ledbuffer::add(p.x, p.y, z, x==this->n_out ? 1.f : 0.0f);
    }
}

edevice*
switcher::solve_electronics(void)
{
    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();

    /* ok, s_in[0] is ready, we have a value to output */
    float input = this->s_in[0].get_value();

    for (int x=0; x<5; x++) {
        if (!this->s_out[x].written()) {/* we haven't written the value yet, do that */
            if (x == this->n_out) {
                this->s_out[x].write(input);
            } else {
                this->s_out[x].write(0.f);
            }
        }
    }

    /* the value is written, each iteration will now reach this point
     * and we wait for the two inputs to get ready */
    if (!this->s_in[1].is_ready())
        return this->s_in[1].get_connected_edevice();
    if (!this->s_in[2].is_ready())
        return this->s_in[2].get_connected_edevice();

    int diff = (int)roundf(this->s_in[1].get_value());
    diff -= (int)roundf(this->s_in[2].get_value());

    this->n_out += diff;

    if (this->n_out < 0) this->n_out = 0;
    else if (this->n_out > 4) this->n_out = 4;

    return 0;
}

