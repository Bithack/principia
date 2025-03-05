#include "egraph.hh"
#include "display.hh"
#include "model.hh"
#include "material.hh"
#include "game.hh"
#include "textbuffer.hh"
#include "world.hh"
#include "settings.hh"
#include "font.hh"

static const float DW = (1.5f*.75f);
static const float DH = (1.f*.75f);
static const float LEFT = -DW/2.f;
static const float STEP = DW/GRAPH_BUFSZ;

egraph::egraph()
{
    memset(this->buffer, 0, sizeof(this->buffer));
    this->set_mesh(mesh_factory::get_mesh(MODEL_GRAPHER));
    this->set_material(&m_edev_dark);

    this->num_s_in = 1;
    this->num_s_out = 1;

    this->s_in[0].lpos = b2Vec2(-.125f, -.45f);
    this->s_out[0].lpos = b2Vec2(.125f, -.45f);

    this->set_flag(ENTITY_DO_UPDATE_EFFECTS, true);

    this->set_as_rect(DW/2.f+.125f, DH/2.f+.2f);
}

void
egraph::on_pause()
{
    memset(this->buffer, 0, sizeof(this->buffer));
    buf_p = 0;
}

void
egraph::setup()
{
    memset(this->buffer, 0, sizeof(this->buffer));
    buf_p = 0;
}

#define SCALE 0.005f

void
egraph::update_effects(void)
{
    float z = this->get_layer()*LAYER_DEPTH + .2f;

    b2Vec2 p = this->get_position();

    float sn,cs;
    tmath_sincos(this->get_angle(), &sn, &cs);

    b2Vec2 left = b2Vec2(LEFT * cs, LEFT * sn);
    b2Vec2 step = b2Vec2(STEP * cs, STEP * sn);
    b2Vec2 down = b2Vec2(-(-1.f)*sn, -1.f*cs);

    display::add_custom(
            p.x,
            p.y,
            z,
            1.f/.08f * 1.25f,
            1.f/.08f * 1.1f,
            sn,cs,
            0.f
            );

    display::add_custom(
            p.x + down.x * -.125f,
            p.y + down.y * -.125f,
            z+.01f,
            1.f/.08f * 1.1f,
            .15f,
            sn,cs,
            0.2f
            );


    if (settings["display_grapher_value"]->v.b && W->is_playing() && (G->state.sandbox || G->state.test_playing)) {
        p_font *f = font::large;
        float val = this->buffer[(this->buf_p-1)%GRAPH_BUFSZ];
        char val_str[32];
        sprintf(val_str, "%.*f", 2, val);
        float width = 0.f;
        int slen = strlen(val_str);
        struct glyph *g;

        for (int i=0; i<slen; ++i) {
            if ((g = f->get_glyph(val_str[i]))) {
                width += g->ax;
            }
        }

        float x = p.x - 0.60f;
        float y = p.y + 0.4f;

        for (int i=0; i<slen; ++i) {
            if ((g = f->get_glyph(val_str[i]))) {
                float x2 = x + (g->bl + g->bw/2.f) * SCALE;
                float y2 = y + (g->bt - g->bh/2.f) * SCALE;

                x += g->ax * SCALE;
                y += g->ay * SCALE;
                textbuffer::add_char(g,
                        x2,
                        y2,
                        z + 0.01f,
                        .3f, .8f, .8f, 1.0f,
                        g->bw*SCALE,
                        g->bh*SCALE
                        );
            }
        }
    }

    for (int x=0; x<GRAPH_BUFSZ; x++) {
        float v = buffer[(GRAPH_BUFSZ-1-x+buf_p)%GRAPH_BUFSZ];

        v *= DH;

        //float vv = (1.f-v)*(1.f / .08f);
        float vv = 1.f-v - .5f;
        v = v*(1.f/0.08f);

        display::add_custom(
                p.x + left.x + step.x * x + down.x*vv/2.f,
                p.y + left.y + step.y * x + down.y*vv/2.f,
                z+.01f,
                .15f,
                v,
                sn,cs,
                1.f
                );
    }
}

edevice*
egraph::solve_electronics(void)
{
    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();

    float v = this->s_in[0].get_value();
    this->buffer[this->buf_p] = v;
    this->buf_p ++;

    if (this->buf_p >= GRAPH_BUFSZ)
        this->buf_p = 0;

    this->s_out[0].write(v);

    return 0;
}

