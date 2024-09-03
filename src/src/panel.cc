#include "panel.hh"
#include "font.hh"
#include "game.hh"
#include "gui.hh"
#include "material.hh"
#include "model.hh"

static void
panel_post_render(struct tms_wdg *w, struct tms_surface *s)
{
    panel::widget *wdg = static_cast<panel::widget*>(w->data3);

    if (wdg->glyph) {
        /* push glyph for rendering */
        G->add_glyph(wdg->glyph, w->pos.x, w->pos.y/*+(w->size.y/3.f)*/);
    }
}

/**
 * pack/unpack-functions based off the matlab implementation
 * of half-precision floating points
 **/

static void
halfp2singles(void *dest, void *source)
{
    uint16_t *hp = (uint16_t *) source; // Type pun input as an unsigned 16-bit int
    uint32_t *xp = (uint32_t *) dest; // Type pun output as an unsigned 32-bit int
    uint16_t h, hs, he, hm;
    uint32_t xs, xe, xm;
    int32_t xes;
    int e;

    if (!source || !dest) return;

    h = *hp++;
    if ((h & 0x7FFFu) == 0) {  // Signed zero
        *xp++ = ((uint32_t)h) << 16;  // Return the signed zero
    } else { // Not zero
        hs = h & 0x8000u;  // Pick off sign bit
        he = h & 0x7C00u;  // Pick off exponent bits
        hm = h & 0x03FFu;  // Pick off mantissa bits
        if (he == 0) {  // Denormal will convert to normalized
            e = -1; // The following loop figures out how much extra to adjust the exponent
            do {
                e++;
                hm <<= 1;
            } while ((hm & 0x0400u) == 0); // Shift until leading bit overflows into exponent bit
            xs = ((uint32_t)hs) << 16; // Sign bit
            xes = ((int32_t)(he >> 10)) - 15 + 127 - e; // Exponent unbias the halfp, then bias the single
            xe = (uint32_t)(xes << 23); // Exponent
            xm = ((uint32_t)(hm & 0x03FFu)) << 13; // Mantissa
            *xp++ = (xs | xe | xm); // Combine sign bit, exponent bits, and mantissa bits
        } else if (he == 0x7C00u) {  // Inf or NaN (all the exponent bits are set)
            if (hm == 0) { // If mantissa is zero ...
                *xp++ = (((uint32_t)hs) << 16) | ((uint32_t)0x7F800000u); // Signed Inf
            } else {
                *xp++ = (uint32_t)0xFFC00000u; // NaN, only 1st mantissa bit set
            }
        } else { // Normalized number
            xs = ((uint32_t)hs) << 16; // Sign bit
            xes = ((int32_t)(he >> 10)) - 15 + 127; // Exponent unbias the halfp, then bias the single
            xe = (uint32_t)(xes << 23); // Exponent
            xm = ((uint32_t)hm) << 13; // Mantissa
            *xp++ = (xs | xe | xm); // Combine sign bit, exponent bits, and mantissa bits
        }
    }
}

static void
singles2halfp(void *dest, void *source)
{
    uint16_t *hp = (uint16_t *) dest; // Type pun output as an unsigned 16-bit int
    uint32_t *xp = (uint32_t *) source; // Type pun input as an unsigned 32-bit int
    uint16_t    hs, he, hm;
    uint32_t x, xs, xe, xm;
    int hes;

    if (!source || !dest) return;

    x = *xp++;
    if ((x & 0x7FFFFFFFu) == 0) {  // Signed zero
        *hp++ = (uint16_t)(x >> 16);  // Return the signed zero
    } else { // Not zero
        xs = x & 0x80000000u;  // Pick off sign bit
        xe = x & 0x7F800000u;  // Pick off exponent bits
        xm = x & 0x007FFFFFu;  // Pick off mantissa bits
        if (xe == 0) {  // Denormal will underflow, return a signed zero
            *hp++ = (uint16_t)(xs >> 16);
        } else if (xe == 0x7F800000u) {  // Inf or NaN (all the exponent bits are set)
            if (xm == 0) { // If mantissa is zero ...
                *hp++ = (uint16_t)((xs >> 16) | 0x7C00u); // Signed Inf
            } else {
                *hp++ = (uint16_t)0xFE00u; // NaN, only 1st mantissa bit set
            }
        } else { // Normalized number
            hs = (uint16_t)(xs >> 16); // Sign bit
            hes = ((int)(xe >> 23)) - 127 + 15; // Exponent unbias the single, then bias the halfp
            if (hes >= 0x1F) {  // Overflow
                *hp++ = (uint16_t)((xs >> 16) | 0x7C00u); // Signed Inf
            } else if (hes <= 0) {  // Underflow
                if ((14 - hes) > 24) {  // Mantissa shifted all the way off & no rounding possibility
                    hm = (uint16_t)0u;  // Set mantissa to zero
                } else {
                    xm |= 0x00800000u;  // Add the hidden leading bit
                    hm = (uint16_t)(xm >> (14 - hes)); // Mantissa
                    if ((xm >> (13 - hes)) & 0x00000001u) // Check for rounding
                        hm += (uint16_t)1u; // Round, might overflow into exp bit, but this is OK
                }
                *hp++ = (hs | hm); // Combine sign bit and mantissa bits, biased exponent is zero
            } else {
                he = (uint16_t)(hes << 10); // Exponent
                hm = (uint16_t)(xm >> 13); // Mantissa
                if (xm & 0x00001000u) // Check for rounding
                    *hp++ = (hs | he | hm) + (uint16_t)1u; // Round, might overflow to inf, this is OK
                else
                    *hp++ = (hs | he | hm);  // No rounding
            }
        }
    }
}

static void
packfloats(void *dest, void *source1, void *source2)
{
    uint16_t val1, val2;

    // convert source1 and source2 single-precision floating points into half-precision floating points
    singles2halfp(&val1, source1);
    singles2halfp(&val2, source2);

    // combine the two half-precision floating points into a single-precision floating point
    uint32_t cool = val2 + (val1 << 16);
    memcpy(dest, &cool, 4);
}

static void
unpackfloats(void *source, void *dest1, void *dest2)
{
    uint32_t v = *((uint32_t*)source);
    uint16_t val1 = v >> 16;
    uint16_t val2 = v;

    halfp2singles(dest1, &val1);
    halfp2singles(dest2, &val2);
}

struct widget_info widget_data[NUM_PANEL_WIDGET_TYPES] = {
    { // PANEL_SLIDER
        TMS_WDG_SLIDER,
        (gui_spritesheet::get_rsprite(S_SLIDER_2)), (gui_spritesheet::get_rsprite(S_SLIDER_HANDLE)),
        2, 1,
        1,
        { 0.f, 0.f },
        LEVEL_VERSION_ANY,
        true,
    },
    { // PANEL_LEFT
        TMS_WDG_BUTTON,
        gui_spritesheet::get_rsprite(S_LEFT), 0,
        1, 1,
        1,
        { 0.f, 0.f },
        LEVEL_VERSION_ANY,
        false
    },
    { // PANEL_RIGHT
        TMS_WDG_BUTTON,
        gui_spritesheet::get_rsprite(S_RIGHT), 0,
        1, 1,
        1,
        { 0.f, 0.f },
        LEVEL_VERSION_ANY,
        false
    },
    { // PANEL_UP
        TMS_WDG_BUTTON,
        gui_spritesheet::get_rsprite(S_UP), 0,
        1, 1,
        1,
        { 0.f, 0.f },
        LEVEL_VERSION_ANY,
        false
    },
    { // PANEL_DOWN
        TMS_WDG_BUTTON,
        gui_spritesheet::get_rsprite(S_DOWN), 0,
        1, 1,
        1,
        { 0.f, 0.f },
        LEVEL_VERSION_ANY,
        false
    },
    { // PANEL_RADIAL
        TMS_WDG_RADIAL,
        gui_spritesheet::get_rsprite(S_RADIAL_2), gui_spritesheet::get_rsprite(S_RADIAL_KNOB),
        2, 2,
        1,
        { 0.f, 0.f },
        LEVEL_VERSION_ANY,
        true
    },
    { // PANEL_BTN
        TMS_WDG_BUTTON,
        gui_spritesheet::get_rsprite(S_EMPTY), 0,
        1, 1,
        1,
        { 0.f, 0.f },
        LEVEL_VERSION_ANY,
        false
    },
    { // PANEL_BIGSLIDER
        TMS_WDG_SLIDER,
        gui_spritesheet::get_rsprite(S_SLIDER_3), gui_spritesheet::get_rsprite(S_SLIDER_HANDLE),
        3, 1,
        1,
        { 0.f, 0.f },
        LEVEL_VERSION_ANY,
        true
    },
    { // PANEL_VSLIDER
        TMS_WDG_VSLIDER,
        gui_spritesheet::get_rsprite(S_SLIDER_2), gui_spritesheet::get_rsprite(S_SLIDER_HANDLE),
        1, 2,
        1,
        { 0.f, 0.f },
        LEVEL_VERSION_ANY,
        true
    },
    { // PANEL_VBIGSLIDER
        TMS_WDG_VSLIDER,
        gui_spritesheet::get_rsprite(S_SLIDER_3), gui_spritesheet::get_rsprite(S_SLIDER_HANDLE),
        1, 3,
        1,
        { 0.f, 0.f },
        LEVEL_VERSION_ANY,
        true
    },
    { // PANEL_BIGRADIAL
        TMS_WDG_RADIAL,
        gui_spritesheet::get_rsprite(S_RADIAL_3), gui_spritesheet::get_rsprite(S_RADIAL_KNOB),
        3, 3,
        1,
        { 0.f, 0.f },
        LEVEL_VERSION_ANY,
        true
    },
    { // PANEL_FIELD
        TMS_WDG_FIELD,
        gui_spritesheet::get_rsprite(S_FIELD_2), gui_spritesheet::get_rsprite(S_FIELD_KNOB),
        2, 2,
        2,
        { 0.f, 0.f },
        LEVEL_VERSION_1_5,
        true
    },
    { // PANEL_BIGFIELD
        TMS_WDG_FIELD,
        gui_spritesheet::get_rsprite(S_FIELD_3), gui_spritesheet::get_rsprite(S_FIELD_KNOB),
        3, 3,
        2,
        { 0.f, 0.f },
        LEVEL_VERSION_1_5,
        true
    },
    { // PANEL_BTN_RADIAL
        TMS_WDG_BTN_RADIAL,
        gui_spritesheet::get_rsprite(S_RADIAL_W_KNOB), gui_spritesheet::get_rsprite(S_RADIAL_2),
        2, 2,
        2,
        { 0.f, 0.f },
        LEVEL_VERSION_1_5,
        true
    },
};

panel::panel(int type)
    : activator(ATTACHMENT_JOINT)
{
    this->set_flag(ENTITY_IS_CONTROL_PANEL,     true);
    this->set_flag(ENTITY_HAS_CONFIG,           true);
    this->set_flag(ENTITY_HAS_INGAME_CONFIG,    true);
    this->set_flag(ENTITY_HAS_ACTIVATOR,        true);

    this->widgets_in_use = 0;
    this->ptype = type;
    this->scaleselect = true;

    switch (this->ptype) {
        case PANEL_BIG: this->init_bigpanel();break;
        case PANEL_SMALL: this->init_smallpanel();break;
        case PANEL_MEDIUM: this->init_mpanel();break;
        case PANEL_XSMALL: this->init_xsmallpanel();break;
    }

    this->set_num_properties(4*this->num_widgets);

    for (int x=0; x<this->num_widgets; x++) {
        this->widgets[x].p = this;
        this->widgets[x].used = false;
        this->widgets[x].owned = false;
        this->widgets[x].first_click = true;
        this->widgets[x].num_socks = 0;
        this->widgets[x].wtype = 0;
        this->widgets[x].outputs = 0;
        this->widgets[x].focused = 0;
        this->widgets[x].num_outputs = 1;
        this->widgets[x].default_value[0] = 0.f;
        this->widgets[x].default_value[1] = 0.f;
        this->widgets[x].glyph = 0;

        this->properties[x*4+0].type = P_INT;
        this->properties[x*4+0].v.i = 0;
        this->properties[x*4+1].type = P_INT;
        this->properties[x*4+1].v.i = 0;
        this->properties[x*4+2].type = P_INT;
        this->properties[x*4+2].v.i = 0;
        this->properties[x*4+3].type = P_FLT;
        this->properties[x*4+3].v.f = 0;
    }

    float ww, h;
    switch (this->ptype) {
        case PANEL_SMALL:
            ww = 1.5f / 2.f;
            h  = 0.5f / 2.f;
            break;

        case PANEL_MEDIUM:
            ww = 1.5f / 2.f;
            h  = 1.0f / 2.f;
            break;

        case PANEL_BIG:
            ww = 2.5f / 2.f;
            h  = 1.7f / 2.f;
            break;

        default:
            tms_errorf("Unknown panel type: %d", this->ptype);
        case PANEL_XSMALL:
            ww = .15f;
            h = .2f;
            break;
    }

    this->set_as_rect(ww, h);
}

void
panel::init_mpanel()
{
    this->num_widgets = 3;
    this->set_mesh(mesh_factory::get_mesh(MODEL_PANEL_MEDIUM));
    this->set_material(&m_mpanel);
    this->menu_scale = 1.f/1.5f;

    this->num_s_in = 9;
    this->num_s_out = 6;

    this->num_feed = 3;
    this->num_set = 3;
    this->has_focus = 1;

    delete [] s_in;
    this->s_in = new socket_in[9];

    float l = 1.5f * .3f - .15f;
    float s = .3f;

    for (int x=0; x<3; x++) {
        this->s_out[x].lpos = b2Vec2(-.6f, l - x*s);
        this->s_out[3+x].lpos = b2Vec2(-.3f, l - x*s);

        this->s_in[x].lpos = b2Vec2(.6f, l - x*s);
        this->s_in[3+x].lpos = b2Vec2(.3f, l - x*s);
        this->s_in[6+x].lpos = b2Vec2(.0f, l - x*s);

        this->s_out[x].ctype = CABLE_RED;
        this->s_out[x].tag = SOCK_TAG_VALUE;
        this->s_out[3+x].ctype = CABLE_RED;
        this->s_out[3+x].tag = SOCK_TAG_FOCUS;

        this->s_in[x].ctype = CABLE_RED;
        this->s_in[x].tag = SOCK_TAG_FEEDBACK;
        this->s_in[3+x].ctype = CABLE_RED;
        this->s_in[3+x].tag = SOCK_TAG_SET_VALUE;
        this->s_in[6+x].ctype = CABLE_RED;
        this->s_in[6+x].tag = SOCK_TAG_SET_ENABLE;
    }
}

void
panel::init_xsmallpanel()
{
    this->num_widgets = 1;
    this->set_mesh(mesh_factory::get_mesh(MODEL_I0O1));
    this->set_material(&m_iomisc);
    this->menu_scale = 1.f;

    this->num_feed = 0;
    this->num_set = 0;
    this->has_focus = 0;

    this->num_s_in = 0;
    this->num_s_out = 1;

    this->s_out[0].lpos = b2Vec2(0.f, 0.f);
    this->s_out[0].ctype = CABLE_RED;
    this->s_out[0].tag = SOCK_TAG_VALUE;
}

void
panel::init_smallpanel()
{
    this->num_widgets = 4;
    this->set_mesh(mesh_factory::get_mesh(MODEL_PANEL_SMALL));
    this->set_material(&m_smallpanel);
    this->menu_scale = 1.f/1.5f;

    this->num_feed = 0;
    this->num_set = 0;
    this->has_focus = 0;

    this->num_s_in = 0;
    this->num_s_out = 4;

    for (int x=0; x<4; x++) {
        this->s_out[x].lpos = b2Vec2(-.45f + .3f * x, -.05f);
        this->s_out[x].ctype = CABLE_RED;
        this->s_out[x].tag = SOCK_TAG_VALUE;
    }
}

void
panel::init_bigpanel()
{
    this->num_widgets = 8;
    this->set_mesh(mesh_factory::get_mesh(MODEL_PANEL_BIG));
    this->set_material(&m_bigpanel);
    this->menu_scale = 1.f/2.f;

    this->num_feed = 8;
    this->num_set = 8;
    this->has_focus = 1;

    delete [] s_in;
    delete [] s_out;
    this->s_in = new socket_in[24];
    this->s_out = new socket_out[16];

    this->num_s_in = 24;
    this->num_s_out = 16;

    float l = -4.f * .3f + .15f;
    float s = .3f;

    for (int x=0; x<8; x++) {
        this->s_out[x].lpos = b2Vec2(l + x*s, -.7f);
        this->s_out[8+x].lpos = b2Vec2(l + x*s, -.4f);

        this->s_in[x].lpos = b2Vec2(l + x*s, .6f);
        this->s_in[8+x].lpos = b2Vec2(l + x*s, .3f);
        this->s_in[16+x].lpos = b2Vec2(l + x*s, .0f);

        this->s_out[x].ctype = CABLE_RED;
        this->s_out[x].tag = SOCK_TAG_VALUE;
        this->s_out[8+x].ctype = CABLE_RED;
        this->s_out[8+x].tag = SOCK_TAG_FOCUS;

        this->s_in[x].ctype = CABLE_RED;
        this->s_in[x].tag = SOCK_TAG_FEEDBACK;
        this->s_in[8+x].ctype = CABLE_RED;
        this->s_in[8+x].tag = SOCK_TAG_SET_VALUE;
        this->s_in[16+x].ctype = CABLE_RED;
        this->s_in[16+x].tag = SOCK_TAG_SET_ENABLE;
    }
}

void
panel::on_load(bool created, bool has_state)
{
    tms_debugf("panel on load");
    if (W->level.version >= LEVEL_VERSION_1_5) {
        for (int x=0; x<this->num_widgets; x++) {
            this->widgets[x].sock[0] = (uint8_t)this->properties[x*4+0].v.i;
            this->widgets[x].wtype = (uint8_t)this->properties[x*4+1].v.i;
            this->widgets[x].used = this->properties[x*4+2].v.i & PANEL_WIDGET_USED;
            this->widgets[x].owned = this->properties[x*4+2].v.i & PANEL_WIDGET_OWNED;
            this->widgets[x].outputs = this->properties[x*4+2].v.i;
            this->widgets[x].first_click = true;

            if (this->widgets[x].used) {
                this->widgets_in_use ++;
                this->init_widget(&this->widgets[x]);

                if (widget_data[this->widgets[x].wtype].can_save_default_value) {
                    if (widget_data[this->widgets[x].wtype].num_outputs > 1) {
                        unpackfloats(&this->properties[x*4+3].v.f, &this->widgets[x].default_value[0], &this->widgets[x].default_value[1]);
                    } else {
                        this->widgets[x].default_value[0] = this->properties[x*4+3].v.f;
                        this->widgets[x].default_value[1] = this->properties[x*4+3].v.f;
                    }
                } else {
                    this->widgets[x].default_value[0] = 0.f;
                    this->widgets[x].default_value[1] = 0.f;
                }
            }
        }
    } else {
        for (int x=0; x<this->num_widgets; x++) {
            this->widgets[x].sock[0] = (uint8_t)this->properties[x*4+0].v.i;
            this->widgets[x].wtype = (uint8_t)this->properties[x*4+1].v.i;
            this->widgets[x].used = (bool)this->properties[x*4+2].v.i;
            this->widgets[x].first_click = true;

            if (this->widgets[x].used) {
                this->widgets_in_use ++;
                this->init_widget(&this->widgets[x]);

                if (this->widgets[x].wtype != PANEL_RADIAL && this->widgets[x].wtype != PANEL_BIGRADIAL &&
                        this->widgets[x].wtype != PANEL_SLIDER && this->widgets[x].wtype != PANEL_BIGSLIDER &&
                        this->widgets[x].wtype != PANEL_VSLIDER && this->widgets[x].wtype != PANEL_VBIGSLIDER) {
                    this->widgets[x].default_value[0] = 0.f;
                    this->widgets[x].default_value[1] = 0.f;
                } else {
                    this->widgets[x].default_value[0] = this->properties[x*4+3].v.f;
                    this->widgets[x].default_value[1] = this->properties[x*4+3].v.f;
                }
            }
        }
    }

    this->update_panel_key_labels();

    entity::on_load(created, has_state);
}

void
panel::pre_write(void)
{
    if (W->level.version >= LEVEL_VERSION_1_5) {
        for (int x=0; x<this->num_widgets; x++) {
            this->properties[x*4+0].v.i = (int)this->widgets[x].sock[0];
            this->properties[x*4+1].v.i = (int)this->widgets[x].wtype;
            uint32_t o = 0;
            if (this->widgets[x].used) {
                o |= PANEL_WIDGET_USED;
            }
            if (this->widgets[x].owned) {
                o |= PANEL_WIDGET_OWNED;
            }

            if (widget_data[this->widgets[x].wtype].num_outputs > 1) {
                for (int n=0; n<this->num_widgets; ++n) {
                    if (this->widgets[x].outputs & (1ULL << (n+1))) {
                        o |= (1ULL << (n+1));
                    }
                }
            } else {
                o |= (1ULL << (x+1));
                // for normal widgets, the sock will be used
            }

            this->properties[x*4+2].v.i = o;
            if (widget_data[this->widgets[x].wtype].num_outputs > 1) {
                packfloats(&this->properties[x*4+3].v.f, &this->widgets[x].default_value[0], &this->widgets[x].default_value[1]);
            } else {
                // only store the first default value
                this->properties[x*4+3].v.f = this->widgets[x].default_value[0];
            }
        }
    } else {
        for (int x=0; x<this->num_widgets; x++) {
            this->properties[x*4+0].v.i = (int)this->widgets[x].sock[0];
            this->properties[x*4+1].v.i = (int)this->widgets[x].wtype;
            this->properties[x*4+2].v.i = (int)this->widgets[x].used;
            this->properties[x*4+3].v.f = this->widgets[x].default_value[0];
        }
    }

    entity::pre_write();
}

void
panel::remove_widget(int index)
{
    this->widgets[index].used = false;

    for (int n=0; n<this->num_widgets; ++n) {
        if (this->widgets[index].outputs & (1ULL << (n+1))) {
            this->widgets[n].owned = false;
        }
    }

    this->update_panel_key_labels();
    this->widgets_in_use --;
}

int
panel::add_widget(struct widget_decl decl, int x, int y, int z)
{
    tms_debugf("Adding widget with type %d", decl.type);
    widget *w = 0;
    widget *w2 = 0;

    int sockets_required = widget_data[decl.type].num_outputs;

    int n = 0;
    while (n < this->num_widgets) {
        if (!this->widgets[n].used && !this->widgets[n].owned) {
            w = &this->widgets[n];
            break;
        } else {
            ++ n;
        }
    }

    tms_debugf("Widgets currently in use: %d", this->widgets_in_use);

    if (!w) return PANEL_NO_ROOM;

    if (sockets_required > 1) {
        // first output is the slot we were given, n
        int nn=0;
        for (; nn<this->num_widgets; ++nn) {
            if (nn == n) continue;
            if (!this->widgets[nn].used && !this->widgets[nn].owned) {
                w2 = &this->widgets[nn];
                break;
            }
        }

        if (!w2) {
            tms_debugf("No room for this widget that requires 2 outputs");
            return PANEL_NO_ROOM;
        }

        w2->owned = true;
        w->outputs = 0;
        w->outputs |= (1ULL << (n+1));
        w->outputs |= (1ULL << (nn+1));
    }

    w->index = n;
    w->sock[0] = z*9+y*3+x;
    w->wtype = decl.type;

    this->widgets_in_use ++;
    this->init_widget(w);

    this->update_panel_key_labels();

    return PANEL_OK;
}

void
panel::init_widget(panel::widget *w)
{
    int sx = 2;
    int sy = 2;

    struct tms_sprite *s1 = 0;
    struct tms_sprite *s2 = 0;

    if (widget_data[w->wtype].s1) s1 = *widget_data[w->wtype].s1;
    if (widget_data[w->wtype].s2) s2 = *widget_data[w->wtype].s2;

    tms_wdg_init(w, widget_data[w->wtype].wdg_type, s1, s2);
    sx = widget_data[w->wtype].sx;
    sy = widget_data[w->wtype].sy;
    w->default_value[0] = widget_data[w->wtype].default_value[0];
    w->default_value[1] = widget_data[w->wtype].default_value[1];
    w->num_outputs = widget_data[w->wtype].num_outputs;
    w->post_render = panel_post_render;
    w->data3 = w;

    w->on_change = 0;
    w->used = true;
    w->size.x = sx*b_w;
    w->size.y = sy*b_h;

    int base_sock = w->sock[0];
    w->num_socks = 0;

    for (int y=0; y<sy; y++) {
        for (int x=0; x<sx; x++) {
            w->sock[w->num_socks++] = base_sock + y*3 + x;
        }
    }

    int x = base_sock%3;
    int y = (base_sock%9)/3;
    float px = (base_sock >= 9) * (_tms.window_width - 3*(PANEL_WDG_OUTER_X));
    w->pos = (tvec2){
        px+(PANEL_WDG_OUTER_X)/2.f + x*PANEL_WDG_OUTER_X + ((sx-1) * .5f)*PANEL_WDG_OUTER_X,
        (PANEL_WDG_OUTER_Y)/2.f + y*PANEL_WDG_OUTER_Y + ((sy-1) * .5f)*PANEL_WDG_OUTER_Y
    };
}

#define MAX_BTN 5
#define MAX_DIR_BTN 1
#define MAX_SLIDERS 4

static const char empty_btn_chars[MAX_BTN] = {
    'F', 'G', 'H', 'J', 'K'
};

static const char slider_chars[MAX_SLIDERS] = {
    'Z', 'X', 'C', 'V'
};

void
panel::update_panel_key_labels()
{
    uint8_t wdg_up_i = 0;
    uint8_t wdg_down_i = 0;
    uint8_t wdg_left_i = 0;
    uint8_t wdg_right_i = 0;
    uint8_t wdg_btn_i = 0;
    uint8_t wdg_slider_i = 0;

    static p_font *font = font::medium;

    for (int x=0; x<this->num_widgets; ++x) {
        if (this->widgets[x].used) {
            switch (this->widgets[x].wtype) {
                case PANEL_LEFT:  if (wdg_left_i < MAX_DIR_BTN) {
                                      this->widgets[x].glyph = font->get_glyph('A');
                                      this->widgets[x].s[0] = gui_spritesheet::get_sprite(S_EMPTY);
                                      ++ wdg_left_i;
                                  } else {
                                      this->widgets[x].glyph = 0;
                                      this->widgets[x].s[0] = *widget_data[this->widgets[x].wtype].s1;
                                  }
                                  break;
                case PANEL_RIGHT: if (wdg_right_i < MAX_DIR_BTN) {
                                      this->widgets[x].glyph = font->get_glyph('D');
                                      this->widgets[x].s[0] = gui_spritesheet::get_sprite(S_EMPTY);
                                      ++ wdg_right_i;
                                  } else {
                                      this->widgets[x].glyph = 0;
                                      this->widgets[x].s[0] = *widget_data[this->widgets[x].wtype].s1;
                                  }
                                  break;
                case PANEL_UP:    if (wdg_up_i < MAX_DIR_BTN) {
                                      this->widgets[x].glyph = font->get_glyph('W');
                                      this->widgets[x].s[0] = gui_spritesheet::get_sprite(S_EMPTY);
                                      ++ wdg_up_i;
                                  } else {
                                      this->widgets[x].glyph = 0;
                                      this->widgets[x].s[0] = *widget_data[this->widgets[x].wtype].s1;
                                  }
                                  break;
                case PANEL_DOWN:  if (wdg_down_i < MAX_DIR_BTN) {
                                      this->widgets[x].glyph = font->get_glyph('S');
                                      this->widgets[x].s[0] = gui_spritesheet::get_sprite(S_EMPTY);
                                      ++ wdg_down_i;
                                  } else {
                                      this->widgets[x].glyph = 0;
                                      this->widgets[x].s[0] = *widget_data[this->widgets[x].wtype].s1;
                                  }
                                  break;
                case PANEL_BTN:   if (wdg_btn_i < MAX_BTN) {
                                      this->widgets[x].glyph = font->get_glyph(empty_btn_chars[wdg_btn_i]);
                                      ++ wdg_btn_i;
                                  } else {
                                      this->widgets[x].glyph = 0;
                                  }
                                  break;
                case PANEL_SLIDER:
                case PANEL_BIGSLIDER:
                case PANEL_VSLIDER:
                case PANEL_VBIGSLIDER:
                case PANEL_RADIAL:
                case PANEL_FIELD:
                case PANEL_BIGFIELD:
                case PANEL_BIGRADIAL:
                                  if (wdg_slider_i < MAX_SLIDERS) {
                                      this->widgets[x].glyph = font->get_glyph(slider_chars[wdg_slider_i]);
                                      ++ wdg_slider_i;
                                  } else {
                                      this->widgets[x].glyph = 0;
                                  }
                                  break;
            }
        }
    }
}

bool
panel::slot_owned_by_radial(int x, int y, int z)
{
    int v = z*9+y*3+x;

    for (int i=0; i<this->num_widgets; i++) {
        if (this->widgets[i].used) {
            for (int j=0; j<this->widgets[i].num_socks;j ++) {
                if (this->widgets[i].sock[j] == v) {
                    if (this->widgets[i].wtype == PANEL_RADIAL)
                        return true;
                    else
                        return false;
                }
            }
        }
    }

    return false;
}

bool
panel::slot_used(int x, int y, int z)
{
    int v = z*9+y*3+x;

    for (int i=0; i<this->num_widgets; i++) {
        if (this->widgets[i].used) {
            for (int j=0; j<this->widgets[i].num_socks;j ++) {
                if (this->widgets[i].sock[j] == v)
                    return true;
            }
        }
    }

    return false;
}

edevice*
panel::solve_electronics()
{
    for (int x=0; x<this->num_widgets; x++) {
        panel::widget *w = &this->widgets[x];

        /* We will assume "owned-widgets" are written from elsewhere */
        if (this->s_out[x].written() || w->owned)
            continue;

        for (int n=0; n<w->num_outputs; ++n) {
            int index = 0;
            if (n == 0) {
                index = x;
            } else {
                for (int i=0; i<this->num_widgets; ++i) {
                    if (i == x) continue;
                    if (w->outputs & (1ULL << (i+1))) {
                        index = i;
                        break;
                    }
                }
            }

            float value = (w->used ? w->value[n] : 0.f);

            if (w->wtype == PANEL_LEFT ||
                    w->wtype == PANEL_RIGHT ||
                    w->wtype == PANEL_UP ||
                    w->wtype == PANEL_DOWN ||
                    w->wtype == PANEL_BTN) {
                if (w->value[n] > .5f) {
                    if (w->first_click) {
                        w->first_click = false;
                        G->play_sound(SND_CLICK, 0.f, 0.f, rand(), 0.5f, false, 0, true);
                    }
                } else {
                    w->first_click = true;
                }
            }

            this->s_out[index].write(value);

            if (this->has_focus) {
                this->s_out[index+this->num_widgets].write(w->focused ? 1.f : 0.f);
            }
        }
    }

    if (!this->widgets_in_use) {
        G->add_error(this, ERROR_RC_NO_WIDGETS);
    }

    for (int x=0; x<num_feed+num_set*2; x++) {
        if (!this->s_in[x].is_ready())
            this->s_in[x].get_connected_edevice();
    }

    for (int x=0; x<this->num_widgets; x++) {
        panel::widget *w = &this->widgets[x];

        // we will skip owned widgets, as the values will be read from elsewhere
        if (w->owned) continue;

        int i = 0;
        if (widget_data[w->wtype].num_outputs == 1) {
            if (x < this->num_feed && this->s_in[x].p) {
                w->ghost[i] = this->s_in[x].get_value();
                w->enable_ghost = 1;
            } else
                w->enable_ghost = 0;

            if (x < this->num_set) {
                if ((bool)roundf(this->s_in[this->num_feed+this->num_set+x].get_value())) {
                    w->value[i] = this->s_in[this->num_feed+x].get_value();
                }
            }
        } else {
            int i = 0;
            for (int n=0; n<this->num_widgets; ++n) {
                if (w->outputs & (1ULL << (n+1))) {
                    /* XXX: Ghost doesn't work quite as it should with multiple outputs,
                       but field (which for now is the only widget with multiple outputs)
                       doesn't have ghost support implemented either, so it doesn't matter */
                    if (n < this->num_feed && this->s_in[n].p) {
                        w->ghost[i] = this->s_in[n].get_value();
                        w->enable_ghost = 1;
                    } else {
                        w->enable_ghost = 0;
                    }

                    if (n < this->num_set) {
                        if ((bool)roundf(this->s_in[this->num_feed+this->num_set+n].get_value())) {
                            w->value[i] = this->s_in[this->num_feed+n].get_value();
                        }
                    }

                    ++ i;
                }
            }
        }
    }

    return 0;
}

void
panel::setup()
{
    for (int x=0; x<this->num_widgets; x++) {
        panel::widget *w = &this->widgets[x];
        if (w->used) {
            w->value[0] = w->default_value[0];
            w->value[1] = w->default_value[1];
        }
    }
}

void
panel::panel_disconnected()
{
    tms_debugf("Disconnected from panel %p, clearing widgets", this);

    for (int x=0; x<this->num_widgets; x++) {
        if (this->widgets[x].used) {
            if (this->widgets[x].type == TMS_WDG_BUTTON && (this->ptype == PANEL_SMALL || this->ptype == PANEL_XSMALL)) {
                // We can only reset widget value when we can be 100% sure the value cannot be overriden
                this->widgets[x].value[0] = this->widgets[x].default_value[0];
                this->widgets[x].value[1] = this->widgets[x].default_value[1];
            }
            G->get_surface()->remove_widget(&this->widgets[x]);
        }
    }
}

void
panel::activate(creature *by)
{
    tms_infof("the panel has been activated!");
}
