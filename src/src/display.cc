#include "display.hh"
#include "model.hh"
#include "material.hh"
#include "ui.hh"

#define RSIZE .08f
#define SIZE (RSIZE+.02f)

static tms::gbuffer *verts;
static tms::gbuffer *indices;
static tms::varray *_va;
static tms::mesh *_mesh;
static tms::entity *_e;

static int n = 0;

static tvec4 base[4];

void display::reset()
{
    n = 0;
}

tms::entity *
display::get_full_entity()
{
    if (_e) return _e;

    _e = new tms::entity();
    _mesh = new tms::mesh(_va, indices);

    _e->prio = 0;
    _e->set_mesh(_mesh);
    _e->set_material(&m_digbuf);

    _mesh->i_start = 0;
    _mesh->i_count = n*7*3;

    return _e;
}

void
display::_init()
{
    verts = new tms::gbuffer(4*DISPLAY_MAX_TOTAL_SQUARES*sizeof(tvec4));
    verts->usage = TMS_GBUFFER_STREAM_DRAW;

    indices = new tms::gbuffer(6*DISPLAY_MAX_TOTAL_SQUARES*sizeof(uint16_t));
    indices->usage = TMS_GBUFFER_STATIC_DRAW;

    _va = new tms::varray(1);
    _va->map_attribute("position", 4, GL_FLOAT, verts);

    base[0] = (tvec4){1.f*RSIZE/2.f, 1.f*RSIZE/2.f, 0.f, 0.f};
    base[1] = (tvec4){-1.f*RSIZE/2.f, 1.f*RSIZE/2.f, 0.f, 0.f};
    base[2] = (tvec4){-1.f*RSIZE/2.f, -1.f*RSIZE/2.f, 0.f, 0.f};
    base[3] = (tvec4){1.f*RSIZE/2.f, -1.f*RSIZE/2.f, 0.f, 0.f};

    uint16_t *i = (uint16_t*)indices->get_buffer();
    for (int x=0; x<DISPLAY_MAX_TOTAL_SQUARES; x++) {
        int o = x*6;
        int vo = x*4;

        i[o+0] = vo+0;
        i[o+1] = vo+1;
        i[o+2] = vo+2;
        i[o+3] = vo+0;
        i[o+4] = vo+2;
        i[o+5] = vo+3;
    }

    indices->upload();

    reset();
}

void
display::add(float x, float y, float z, float sn, float cs, float col)
{
    if (n < DISPLAY_MAX_TOTAL_SQUARES-1) {
        tvec4 *b = (tvec4*)verts->get_buffer();

        int num = __sync_fetch_and_add(&n, 1);

        for (int ix=0; ix<4; ix++) {
            b[num*4+ix].x = base[ix].x * cs - base[ix].y * sn;
            b[num*4+ix].y = base[ix].x * sn + base[ix].y * cs;

            b[num*4+ix].x += x;
            b[num*4+ix].y += y;
            b[num*4+ix].z = z;
            b[num*4+ix].w = col;
        }
    }
}

void
display::add_custom(float x, float y, float z, float width, float height, float sn, float cs, float col)
{
    if (n < DISPLAY_MAX_TOTAL_SQUARES-1) {
        tvec4 *b = (tvec4*)verts->get_buffer();

        int num = __sync_fetch_and_add(&n, 1);

        for (int ix=0; ix<4; ix++) {

            tvec4 _b = base[ix];
            _b.x *= width;
            _b.y *= height;

            b[num*4+ix].x = _b.x * cs - _b.y * sn;
            b[num*4+ix].y = _b.x * sn + _b.y * cs;

            b[num*4+ix].x += x;
            b[num*4+ix].y += y;
            b[num*4+ix].z = z;
            b[num*4+ix].w = col;
        }
    }
}

void display::upload()
{
    if (_mesh) {
        _mesh->i_start = 0;
        _mesh->i_count = n*6;
    }

    if (n)
        verts->upload_partial(n*4*sizeof(tvec4));
}

static char preset[] =
    "01110\n"
    "10001\n"
    "10011\n"
    "10101\n"
    "11001\n"
    "10001\n"
    "01110\n\n"

    "00010\n"
    "01110\n"
    "00010\n"
    "00010\n"
    "00010\n"
    "00010\n"
    "00010\n\n"

    "01110\n"
    "10001\n"
    "00001\n"
    "00010\n"
    "00100\n"
    "01000\n"
    "11111\n\n"

    "01110\n"
    "10001\n"
    "00001\n"
    "01110\n"
    "00001\n"
    "10001\n"
    "01110\n\n"

    "00010\n"
    "00110\n"
    "01010\n"
    "10010\n"
    "11111\n"
    "00010\n"
    "00010\n\n"

    "11111\n"
    "10000\n"
    "10000\n"
    "11110\n"
    "00001\n"
    "00001\n"
    "11110\n\n"

    "01110\n"
    "10001\n"
    "10000\n"
    "11110\n"
    "10001\n"
    "10001\n"
    "01110\n\n"

    "11111\n"
    "00001\n"
    "00001\n"
    "00010\n"
    "00100\n"
    "00100\n"
    "00100\n\n"

    "01110\n"
    "10001\n"
    "10001\n"
    "01110\n"
    "10001\n"
    "10001\n"
    "01110\n\n"

    "01110\n"
    "10001\n"
    "10001\n"
    "01111\n"
    "00001\n"
    "10001\n"
    "01110\n\n"

    "01110\n"
    "10001\n"
    "10001\n"
    "11111\n"
    "10001\n"
    "10001\n"
    "10001\n\n"

    "11110\n"
    "10001\n"
    "10001\n"
    "11111\n"
    "10001\n"
    "10001\n"
    "11110\n\n"

    "01110\n"
    "10001\n"
    "10000\n"
    "10000\n"
    "10000\n"
    "10001\n"
    "01110\n\n"

    "11110\n"
    "10001\n"
    "10001\n"
    "10001\n"
    "10001\n"
    "10001\n"
    "11110\n\n"

    "11111\n"
    "10000\n"
    "10000\n"
    "11111\n"
    "10000\n"
    "10000\n"
    "11111\n\n"

    "11111\n"
    "10000\n"
    "10000\n"
    "11111\n"
    "10000\n"
    "10000\n"
    "10000\n\n"

    "01111\n"
    "10001\n"
    "10000\n"
    "10111\n"
    "10001\n"
    "10001\n"
    "01110\n\n"

    "10001\n"
    "10001\n"
    "10001\n"
    "11111\n"
    "10001\n"
    "10001\n"
    "10001\n\n"

    "00100\n"
    "00100\n"
    "00100\n"
    "00100\n"
    "00100\n"
    "00100\n"
    "00100\n\n"

    "00010\n"
    "00010\n"
    "00010\n"
    "00010\n"
    "01010\n"
    "01010\n"
    "00100\n\n"

    "10001\n"
    "10010\n"
    "10100\n"
    "11000\n"
    "10100\n"
    "10010\n"
    "10001\n\n"

    "10000\n"
    "10000\n"
    "10000\n"
    "10000\n"
    "10000\n"
    "10000\n"
    "11111\n\n"

    "10001\n"
    "11011\n"
    "10101\n"
    "10101\n"
    "10001\n"
    "10001\n"
    "10001\n\n"

    "10001\n"
    "10001\n"
    "11001\n"
    "10101\n"
    "10011\n"
    "10001\n"
    "10001\n\n"

    "01110\n"
    "10001\n"
    "10001\n"
    "10001\n"
    "10001\n"
    "10001\n"
    "01110\n\n"

    "11110\n"
    "10001\n"
    "10001\n"
    "11110\n"
    "10000\n"
    "10000\n"
    "10000\n\n"

    "01110\n"
    "10001\n"
    "10001\n"
    "10001\n"
    "10101\n"
    "10011\n"
    "01111\n\n"

    "11110\n"
    "10001\n"
    "10001\n"
    "11110\n"
    "10100\n"
    "10010\n"
    "10001\n\n"

    "01110\n"
    "10001\n"
    "10000\n"
    "01110\n"
    "00001\n"
    "10001\n"
    "01110\n\n"

    "11111\n"
    "00100\n"
    "00100\n"
    "00100\n"
    "00100\n"
    "00100\n"
    "00100\n\n"

    "10001\n"
    "10001\n"
    "10001\n"
    "10001\n"
    "10001\n"
    "10001\n"
    "01110\n\n"

    "10001\n"
    "10001\n"
    "10001\n"
    "10001\n"
    "10001\n"
    "01010\n"
    "00100\n\n"

    "10001\n"
    "10001\n"
    "10001\n"
    "10001\n"
    "10001\n"
    "10101\n"
    "01010\n\n"

    "10001\n"
    "10001\n"
    "01010\n"
    "00100\n"
    "01010\n"
    "10001\n"
    "10001\n\n"

    "10001\n"
    "10001\n"
    "10001\n"
    "01010\n"
    "00100\n"
    "00100\n"
    "00100\n\n"

    "11111\n"
    "00001\n"
    "00010\n"
    "00100\n"
    "01000\n"
    "10000\n"
    "11111\n\n"

    "\0"
;

display::display()
{
    this->set_flag(ENTITY_HAS_CONFIG, true);

    this->dialog_id = DIALOG_DIGITALDISPLAY;

    this->set_material(&m_colored);
    this->set_uniform("~color", .15f, .15f, .15f, 1.f);

    this->active_symbol = 0;

    this->set_flag(ENTITY_DO_UPDATE_EFFECTS, true);

    this->set_as_rect(.3f, .5f);

    this->active = true;

    this->set_num_properties(3);
    this->properties[0].type = P_INT8;
    this->properties[0].v.i8 = 0; /* for passive: wrap enable/disable. for active: unused */

    this->properties[1].type = P_INT8;
    this->properties[1].v.i8 = 0; /* initial position */

    this->properties[2].type = P_STR;
    this->properties[2].v.s.buf = 0; /* symbols */
    this->properties[2].v.s.len = 0;
}

void
display::construct()
{
    this->on_load(false, false);
}

void
display::on_load(bool created, bool has_state)
{
    this->load_symbols();
}

void
display::setup()
{
    this->active_symbol = this->properties[1].v.i8;

    if (this->active_symbol >= this->num_symbols)
        this->active_symbol = this->num_symbols-1;
}

void
display::on_pause()
{
    this->active_symbol = this->properties[1].v.i8;
    if (this->active_symbol >= this->num_symbols)
        this->active_symbol = this->num_symbols-1;

    this->active = true;
}

void
display::update_effects()
{
    float sn = sinf(-this->get_angle());
    float cs = cosf(-this->get_angle());

    for (int y=0; y<35; y++) {
        float sx = -2.f * SIZE + (y % 5) * SIZE;
        float sy = 8.0f/2.f * SIZE - (y/5) * SIZE;

        b2Vec2 p = this->local_to_world(b2Vec2(sx, sy), 0);

        bool active = (this->symbols[this->active_symbol] & (1ull << y));

#if 0
        if (active) {
            this->add(p.x, p.y, this->get_layer()*LAYER_DEPTH + .251f, cs, sn, 1.f);
        }
#else
        this->add(p.x, p.y, this->get_layer()*LAYER_DEPTH + .251f, cs, sn, active && this->active?1.f:0.f);
#endif
    }
}

void
display::load_symbols()
{
    int sn = 0;
    char *s = this->properties[2].v.s.buf;
    this->num_symbols = 0;

    if (!s) {
        this->set_property(2, preset);
        s = this->properties[2].v.s.buf;
    }

    while (*s && sn < DISPLAY_MAX_SYMBOLS) {
        symbols[sn] = 0;
        int x;
        for (x=0; x<35 && *s; ) {
            if (*s == '1') {
                symbols[sn] |= (1ull << x);
                x++;
            } else if (*s =='0')
                x++;

            s ++;
        }

        if (x > 0) sn ++;
    }

    this->num_symbols = sn;
    this->active_symbol = this->properties[1].v.i8;
    if (this->active_symbol >= this->num_symbols)
        this->active_symbol = this->num_symbols-1;
}

passive_display::passive_display()
{
    this->set_mesh(mesh_factory::get_mesh(MODEL_DISPLAY));
    this->num_s_out = 0;
    this->num_s_in = 3;

    this->s_in[0].ctype = CABLE_RED;
    this->s_in[0].lpos = b2Vec2(-.2f, -.4f); /* activate */
    this->s_in[0].tag = SOCK_TAG_ONOFF;

    this->s_in[1].ctype = CABLE_RED;
    this->s_in[1].lpos = b2Vec2(  .0f, -.4f); /* increment */
    this->s_in[1].tag = SOCK_TAG_INCREASE;

    this->s_in[2].ctype = CABLE_RED;
    this->s_in[2].lpos = b2Vec2( .2f, -.4f); /* decrement */
    this->s_in[2].tag = SOCK_TAG_DECREASE;

}

edevice*
passive_display::solve_electronics()
{
    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();
    if (!this->s_in[1].is_ready())
        return this->s_in[1].get_connected_edevice();
    if (!this->s_in[2].is_ready())
        return this->s_in[2].get_connected_edevice();

    this->active = (this->s_in[0].p == 0 || (bool)(int)roundf(this->s_in[0].get_value()));

    if ((bool)roundf(this->s_in[1].get_value()))
        this->active_symbol ++;
    if ((bool)roundf(this->s_in[2].get_value()))
        this->active_symbol --;

    if (this->active_symbol < 0){
        if (this->properties[0].v.i8)
            this->active_symbol = this->num_symbols-1;
        else
            this->active_symbol = 0;
    } else if (this->active_symbol >= this->num_symbols) {
        if (this->properties[0].v.i8)
            this->active_symbol = 0;
        else
            this->active_symbol = this->num_symbols-1;
    }

    return 0;
}

active_display::active_display()
{
    this->set_mesh(mesh_factory::get_mesh(MODEL_DISPLAY_ACTIVE));
    this->num_s_out = 1;
    this->num_s_in = 2;

    this->s_in[0].ctype = CABLE_RED;
    this->s_in[0].lpos = b2Vec2(-.2f, -.4f); /* active */
    this->s_in[0].tag = SOCK_TAG_ONOFF;

    this->s_in[1].ctype = CABLE_RED;
    this->s_in[1].lpos = b2Vec2(  .0f, -.4f); /* current symbol fraction */
    this->s_in[1].tag = SOCK_TAG_FRACTION;

    this->s_out[0].ctype = CABLE_RED;
    this->s_out[0].lpos = b2Vec2( .2f, -.4f); /* current symbol fraction out */
}

edevice*
active_display::solve_electronics()
{
    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();
    if (!this->s_in[1].is_ready())
        return this->s_in[1].get_connected_edevice();

    this->active = (this->s_in[0].p == 0 || (bool)(int)roundf(this->s_in[0].get_value()));

    float v = tclampf(this->s_in[1].get_value(), 0.f, 1.f);

    if (this->s_in[1].p) {
        // plug is connected
        this->active_symbol = roundf((this->num_symbols-1) * v);
    } else {
        this->active_symbol = this->properties[1].v.i;
    }

    this->s_out[0].write(((float)this->active_symbol) / (float)(this->num_symbols-1));

    return 0;
}
