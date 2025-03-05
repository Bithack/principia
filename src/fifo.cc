#include "fifo.hh"
#include "ledbuffer.hh"


fifo::fifo()
    : ptr(0)
{
    for (int x=0; x<8; x++) {
        this->queue[x] = 0.f;
    }

    this->menu_scale = 1.0f;
    this->set_flag(ENTITY_DO_UPDATE_EFFECTS, true);

    this->ptr = 0;
}

void
fifo::write_state(lvlinfo *lvl, lvlbuf *lb)
{
    entity::write_state(lvl,lb);
    lb->ensure(8*sizeof(float) + sizeof(uint8_t));
    for (int x=0; x<8; x++) {
        lb->w_float(this->queue[x]);
    }
    lb->w_uint8(this->ptr);
}

void
fifo::read_state(lvlinfo *lvl, lvlbuf *lb)
{
    entity::read_state(lvl,lb);
    for (int x=0; x<8; x++) {
        this->queue[x] = lb->r_float();
    }
    this->ptr = lb->r_uint8();
}

void
fifo::setup()
{
    for (int x=0; x<8; x++)
        this->queue[x] = 0.f;
    this->ptr = 0;
}

edevice*
fifo::solve_electronics()
{
    if (!this->s_out[0].written()) {
        float out = queue[this->ptr];
        this->s_out[0].write(out);
    }

    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();

    float v = this->s_in[0].get_value();
    queue[this->ptr] = v;

    this->ptr ++;
    this->ptr = this->ptr & 7;

    return 0;
}

void
fifo::update_effects(void)
{
    float z = this->get_layer() * LAYER_DEPTH + LED_Z_OFFSET;
    for (int x=0; x<8; x++) {
        int y = (this->ptr+x) & 7;

        b2Vec2 p = this->local_to_world(b2Vec2(0, .25f-x*(.5f/8.f)), 0);
        ledbuffer::add(p.x, p.y, z, queue[y]);
    }
}
