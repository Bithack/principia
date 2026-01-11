#include "sticky.hh"
#include "model.hh"
#include "material.hh"
#include "world.hh"
#include "ui.hh"

#include "font.hh"
#include "gui.hh"
#include <SDL.h>

#include <cstddef>
#include <cstdlib>
#include <cstring>

#define NUM_SLOTS 33

#define NOTE_FONT "data/fonts/easyspeech.ttf"
#define NUM_SIZES 4

#define PIXELSZ 1

#define TEX_WIDTH 2048
#define TEX_HEIGHT 2048

#define WIDTH 256
#define HEIGHT 256

#define FONT_SCALING_FACTOR 2.

//computed
#define UV_X ((double) WIDTH / (double) TEX_WIDTH)
#define UV_Y ((double) HEIGHT / (double) TEX_HEIGHT)
#define SLOTS_PER_TEX_LINE (TEX_WIDTH / WIDTH)
//

static bool slots[NUM_SLOTS];
static bool initialized = false;
static p_font *note_font[NUM_SIZES];
static SDL_Surface *surface;
static int spacing[NUM_SIZES];
tms_texture sticky::texture;

// Decode one UTF-8 codepoint; advance *s. Returns codepoint or -1 on end/error.
// TODO: Use SDL_StepUTF8() when we're on SDL3
static int utf8_next(const char **s) {
    const unsigned char *p = (const unsigned char*)*s;
    if (*p == 0) return -1;
    if (*p < 0x80) { int cp = *p; *s += 1; return cp; }
    if ((*p & 0xE0) == 0xC0) {
        unsigned char b0 = p[0], b1 = p[1];
        if ((b1 & 0xC0) != 0x80) { *s += 1; return '?'; }
        int cp = ((b0 & 0x1F) << 6) | (b1 & 0x3F); *s += 2; return cp;
    } else if ((*p & 0xF0) == 0xE0) {
        unsigned char b0 = p[0], b1 = p[1], b2 = p[2];
        if ((b1 & 0xC0) != 0x80 || (b2 & 0xC0) != 0x80) { *s += 1; return '?'; }
        int cp = ((b0 & 0x0F) << 12) | ((b1 & 0x3F) << 6) | (b2 & 0x3F); *s += 3; return cp;
    } else if ((*p & 0xF8) == 0xF0) {
        unsigned char b0 = p[0], b1 = p[1], b2 = p[2], b3 = p[3];
        if ((b1 & 0xC0) != 0x80 || (b2 & 0xC0) != 0x80 || (b3 & 0xC0) != 0x80) { *s += 1; return '?'; }
        int cp = ((b0 & 0x07) << 18) | ((b1 & 0x3F) << 12) | ((b2 & 0x3F) << 6) | (b3 & 0x3F); *s += 4; return cp;
    }
    *s += 1; return '?';
}

// Convert UTF-8 string to Latin-1 in dst (dst_sz bytes). Returns number of bytes written (excluding null).
static int utf8_to_latin1(const char *src, char *dst, int dst_sz) {
    const char *p = src;
    int out = 0;
    while (*p && out + 1 < dst_sz) {
        int cp = utf8_next(&p);
        if (cp < 0)
            break;
        if (cp == '\n') {
            dst[out++] = '\n';
            continue;
        }

        dst[out++] = ((cp >= 0 && cp <= 255) ? (unsigned char)cp : '?');
    }
    dst[out] = 0;
    return out;
}

// Measure ASCII/UTF8 (ASCII-focused) width/height using p_font advances.
static void note_font_measure(p_font *f, const char *text, int *out_w, int *out_h) {
    size_t len = strlen(text);
    char *buf = (char*)malloc(len + 1);
    if (!buf) { if (out_w) *out_w = 0; if (out_h) *out_h = f->height; return; }
    utf8_to_latin1(text, buf, (int)len + 1);

    int w = 0;
    for (const unsigned char *p = (const unsigned char*)buf; *p; ++p) {
        struct glyph *g = f->get_glyph(*p);
        if (!g)
            continue;

        if (g->newline)
            break;
        w += g->ax;
    }
    if (out_w) *out_w = w;
    if (out_h) *out_h = f->height;
    free(buf);
}

// Render text into an 8-bit SDL surface where each pixel is a glyph alpha value.
// Slightly modified from SDL_TTF
static SDL_Surface* note_render_shaded(p_font *f, const char *text) {
    int w, h;
    note_font_measure(f, text, &w, &h);
    if (w <= 0) w = 1;
    if (h <= 0) h = 1;

    SDL_Surface *srf = SDL_CreateRGBSurfaceWithFormat(0, w, h, 8, SDL_PIXELFORMAT_INDEX8);
    if (!srf) return NULL;

    // populate palette (white entries, alpha is simulated by byte value copied out later)
    SDL_Color pal[256];
    for (int i = 0; i < 256; ++i) {
        pal[i].r = 0xFF;
        pal[i].g = 0xFF;
        pal[i].b = 0xFF;
    }
    if (srf->format && srf->format->palette) SDL_SetPaletteColors(srf->format->palette, pal, 0, 256);

    // clear
    memset(srf->pixels, 0, srf->pitch * srf->h);

    // Convert UTF-8 to Latin1 first, then render by byte.
    size_t len = strlen(text);
    char *buf = (char*)malloc(len + 1);
    if (!buf) return NULL;
    utf8_to_latin1(text, buf, (int)len + 1);

    int x_cursor = 0;
    for (const unsigned char *p = (const unsigned char*)buf; *p; ++p) {
        struct glyph *g = f->get_glyph(*p);
        if (!g) continue;
        if (g->newline) break;

        if (g->m_sprite_buf && g->bw > 0 && g->bh > 0) {
            int dest_x = x_cursor + g->bl;
            int dest_y = f->ascent - g->bt;

            for (int gy = 0; gy < g->bh; ++gy) {
                for (int gx = 0; gx < g->bw; ++gx) {
                    int sx = dest_x + gx;
                    int sy = dest_y + gy;
                    if (sx < 0 || sx >= w || sy < 0 || sy >= h)
                        continue;
                    unsigned char val = g->m_sprite_buf[gy * g->bw + gx];

                    // Don't draw transparent pixels (may cut off previous glyphs)
                    if (val == 0)
                        continue;

                    ((unsigned char*)srf->pixels)[sy * srf->pitch + sx] = val;
                }
            }
        }

        x_cursor += g ? g->ax : 0;
    }

    return srf;
}

void sticky::_init(void) {
    int unused;

    for (int size_idx = 0; size_idx < NUM_SIZES; size_idx++) {
        int font_size = FONT_SCALING_FACTOR * (double)(16 + 6 * size_idx);
        note_font[size_idx] = new p_font(gui_spritesheet::atlas_text, NOTE_FONT, font_size, true);

        int w = 0;
        const char *sp = " ";
        for (const unsigned char *c = (const unsigned char*)sp; *c; ++c) {
            struct glyph *g = note_font[size_idx]->get_glyph(*c);
            if (g) w += g->ax;
        }

        spacing[size_idx] = w;
    }

    //texture = tms_texture_alloc();
    tms_texture_init(&sticky::texture);
    tms_texture_set_filtering(&sticky::texture, GL_LINEAR);
    tms_texture_alloc_buffer(&sticky::texture, TEX_WIDTH, TEX_HEIGHT, PIXELSZ);
    tms_texture_clear_buffer(&sticky::texture, 0);

    initialized = true;
}

void sticky::_deinit(void) {
    for (int x=0; x<NUM_SIZES; x++) {
        if (note_font[x]) {
            delete note_font[x];
            note_font[x] = nullptr;
        }
    }
}

sticky::sticky() {
    if (!initialized) {
        _init();
    }

    for (int x=0; x<STICKY_MAX_LINES; ++x) {
        memset(&this->lines[x], 0, STICKY_MAX_PER_LINE);
        this->linelen[x] = 0;
    }

    this->set_flag(ENTITY_ALLOW_CONNECTIONS,    false);
    this->set_flag(ENTITY_DISABLE_LAYERS,       true);
    this->set_flag(ENTITY_HAS_CONFIG,           true);

    this->dialog_id = DIALOG_STICKY;

    this->update_method = ENTITY_UPDATE_STATIC_CUSTOM;
    this->menu_scale = .75f;
    this->set_mesh(mesh_factory::get_mesh(MODEL_STICKY));
    this->set_material(&m_sticky);
    this->set_uniform("~color", sqrtf(233.f/255.f), sqrtf(191.f/255.f), sqrtf(.2f), 1.f);
    this->body = 0;
    this->currline = 0;

    this->_angle = -.1f + (rand()%100)/100.f * .2f;

    this->slot = -1;
    this->width = .76f*2.f;
    this->height = .76f*2.f;

    int maxslots = NUM_SLOTS;
    // Compatibility
    if (W->level.version < LEVEL_VERSION_2023_06_05) {
        maxslots = 8;
    }

    for (int x=0; x < maxslots; x++) {
        if (slots[x] == false) {
            this->slot = x;
            slots[x] = true;
            break;
        }
    }

    if (this->slot == -1) {
        /* XXX: This slot is incremented each load, and not reset. */
        ui::message("The maximum number of sticky notes has been reached.");
        return;
    }
    tms_infof("NOTE SLOT is %d", this->slot);

    //Set sprite coords
    int nx = this->slot % SLOTS_PER_TEX_LINE;
    int ny = this->slot / SLOTS_PER_TEX_LINE;
    this->set_uniform(
        "sprite_coords",
        UV_X * nx,
        UV_Y * ny,
        UV_X * (float)(nx + 1),
        UV_Y * (float)(ny + 1)
    );

    tmat4_load_identity(this->M);
    tmat3_load_identity(this->N);

    this->set_num_properties(4);
    this->properties[0].type = P_STR;  // text
    this->properties[1].type = P_INT8; // horizontal align
    this->properties[2].type = P_INT8; // vertical align
    this->properties[3].type = P_INT8; // font size

    this->set_property(1, (uint8_t) 1);
    this->set_property(2, (uint8_t) 1);
    this->set_property(3, (uint8_t) 3);

    this->set_text("Hello!");
}

sticky::~sticky() {
    if (this->slot > 0 && this->slot < NUM_SLOTS) {
        slots[this->slot] = false;
    }
}

void sticky::add_word(const char *word, int len) {
    int w,h;

    if (this->currline >= STICKY_MAX_LINES)
        return;

    int added_space = 0;

    /* XXX: This might be broken, but it works! */
    if (this->linelen[this->currline]) {
        int bytes_left = STICKY_MAX_LINES - this->linelen[this->currline];
        if (bytes_left > 0) {
            added_space = 1;
            int write_len = (std::min(bytes_left, this->linelen[this->currline]+len));
            memcpy(this->lines[this->currline]+this->linelen[this->currline]+1, word, write_len);
            this->lines[this->currline][write_len] = ' ';
            this->lines[this->currline][write_len+1] = 0;
        }
    } else {
        added_space = 0;
        int write_len = std::min(STICKY_MAX_PER_LINE-1, len);
        memcpy(this->lines[this->currline]+this->linelen[this->currline], word, write_len);
        this->lines[this->currline][write_len] = 0;
    }

    note_font_measure(note_font[this->properties[3].v.i8], this->lines[this->currline], &w, &h);

    if (w > WIDTH) {
        char p[STICKY_MAX_PER_LINE];
        strcpy(p, this->lines[this->currline]);
        p[STICKY_MAX_PER_LINE-1] = '\0';
        do {
            p[strlen(p)-1] = 0;
            note_font_measure(note_font[this->properties[3].v.i8], p, &w, NULL);
        } while (w > WIDTH);
        this->lines[this->currline][this->linelen[this->currline]] = 0;

        memcpy(this->lines[this->currline], p, strlen(p));
        this->lines[this->currline][strlen(p)] = '\0';
        this->linelen[this->currline]=strlen(p);
        if (this->linelen[this->currline] > STICKY_MAX_PER_LINE) {
            this->linelen[this->currline] = STICKY_MAX_PER_LINE;
        }
    } else {
        this->linelen[this->currline]+=len+added_space;
        if (this->linelen[this->currline] > STICKY_MAX_PER_LINE) {
            this->linelen[this->currline] = STICKY_MAX_PER_LINE;
        }

        this->lines[this->currline][this->linelen[this->currline]] = '\0';
    }
}

void sticky::on_load(bool created, bool has_state) {
    this->update_text();
}

void sticky::next_line() {
    this->currline++;
}

void sticky::draw_text(const char *txt) {
    const char *s = txt;
    const char *w = s;
    for (;;s++) {
        if (*s == '\n') {
            if (s-w > 0) this->add_word(w, (int)(s-w));
            this->next_line();
            w = s+1;
        } else if (*s == 0) {
            if (s-w > 0) this->add_word(w, (int)(s-w));
            break;
        }
    }

    this->currline++;
    if (this->currline > STICKY_MAX_LINES) this->currline = STICKY_MAX_LINES;

    unsigned char *buf = tms_texture_get_buffer(&sticky::texture);

    int line_skip = note_font[this->properties[3].v.i8]->lineskip;

    for (size_t text_line = 0; text_line < this->currline; text_line++) {
        /* Skip any lines that do not contain any content */
        if (this->linelen[text_line] == 0) continue;

        SDL_Surface *srf = note_render_shaded(
            note_font[this->properties[3].v.i8],
            this->lines[text_line]
        );

        if (srf == NULL) {
            tms_errorf("Error creating SDL Surface:%s", SDL_GetError());
            continue;
        }

        //Alignment/centering
        int align_y = this->properties[2].v.i8 ? ((HEIGHT + this->currline * line_skip) / 2.) : HEIGHT-1;
        int align_x = this->properties[1].v.i8 ? ((WIDTH - srf->w) / 2.) : 0;

        for (int y = 0; y < srf->h; y++) {
            for (int x = 0; x < srf->pitch; x++) {
                for (int z = 0; z < PIXELSZ; z++) {
                    int dest_y = ((align_y - line_skip * text_line) - y);
                    int dest_x = align_x + x;

                    //Prevent leaking to other notes
                    if ((dest_x < 0) || (dest_x >= WIDTH)) continue;
                    if ((dest_y < 0) || (dest_y >= HEIGHT)) continue;

                    //Destination
                    size_t offset = (
                        (((this->slot / SLOTS_PER_TEX_LINE) * HEIGHT + (size_t) dest_y) * TEX_WIDTH) +
                        ((this->slot % SLOTS_PER_TEX_LINE) * WIDTH) + (size_t) dest_x
                    ) * PIXELSZ + z;

                    //Prevent out-of-bounds access
                    if (offset >= TEX_HEIGHT * TEX_WIDTH) continue;

                    //Source
                    int data_offset = (y * srf->pitch) + x;

                    //Source -> Destination
                    unsigned char data = ((unsigned char*) srf->pixels)[data_offset];
                    buf[offset] = data;

                    // tms_debugf(
                    //     "slot/offset/data_offset/data %d/%d/%d/%d/%x - '%s'",
                    //     this->slot, offset, data_offset, data, this->lines[text_line]
                    // );
                }
            }
        }
        SDL_FreeSurface(srf);
    }
}

void sticky::set_text(const char *txt) {
    this->set_property(0, txt);
    this->update_text();
}

void sticky::update_text() {
    for (int x = 0; x < STICKY_MAX_LINES; x++) {
        this->linelen[x] = 0;
    }
    this->currline = 0;

    // clear the texture
    unsigned char *buf = tms_texture_get_buffer(&sticky::texture);
    size_t ty = (this->slot / SLOTS_PER_TEX_LINE) * HEIGHT;
    size_t tx = (this->slot % SLOTS_PER_TEX_LINE) * WIDTH;
    for (size_t y = ty; y < (ty + HEIGHT); y++) {
        for (size_t x = tx; x < (tx + WIDTH); x++) {
            buf[x + y * TEX_WIDTH] = 0;
        }
    }

    // inital render
    this->draw_text(this->properties[0].v.s.buf);
    tms_texture_upload(&sticky::texture);
}

void sticky::add_to_world() {
    b2Fixture *f;
    this->create_rect(b2_staticBody, .76f, .76f, this->get_material(), &f);
    this->body->GetFixtureList()[0].SetSensor(true);
}

void sticky::update(void) {
    if (this->body) {
        b2Transform t;
        t = this->body->GetTransform();

        this->M[0] = t.q.c;
        this->M[1] = t.q.s;
        this->M[4] = -t.q.s;
        this->M[5] = t.q.c;
        this->M[12] = t.p.x;
        this->M[13] = t.p.y;
        this->M[14] = LAYER_DEPTH / -2.1f;
        //this->M[14] = this->get_layer()*LAYER_DEPTH - LAYER_DEPTH/2.1f;

        tmat3_copy_mat4_sub3x3(this->N, this->M);
    } else {
        tmat4_load_identity(this->M);
        tmat4_translate(this->M, this->_pos.x, this->_pos.y, -.0f);
        tmat4_rotate(this->M, this->_angle * (180.f/M_PI), 0, 0, -1);
        tmat3_copy_mat4_sub3x3(this->N, this->M);
    }
}
