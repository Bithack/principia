#include "sticky.hh"
#include "model.hh"
#include "material.hh"
#include "world.hh"
#include "ui.hh"

#include "SDL_ttf.h"

#define NUM_SIZES 4

static bool initialized = false;
static TTF_Font *ttf_font[NUM_SIZES];
static SDL_Surface *surface;
static int spacing[NUM_SIZES];

tms_texture sticky::texture;

#define WIDTH 128
#define HEIGHT 128
#define PIXELSZ 1

#define UV_RATIO ((1.0f * WIDTH)/(1.0f * (HEIGHT * NUM_SLOTS)))

#define NUM_SLOTS 32

static bool slots[NUM_SLOTS];

void
sticky::_init(void)
{
    TTF_Init();

    int unused;

    for (int x=0; x<NUM_SIZES; x++) {
        ttf_font[x] = TTF_OpenFont("data-shared/fonts/easyspeech.ttf", 16+6*x);
        //ttf_font[x] = TTF_OpenFont("data/fonts/quivira.ttf", 16+6*x);
        //ttf_font[x] = TTF_OpenFont("data/fonts/gulim.ttf", 16+6*x);
        //ttf_font[x] = TTF_OpenFont("data/fonts/DejaVuSans.ttf", 16+6*x);
        //ttf_font[x] = TTF_OpenFont("data/fonts/Ubuntu-R.ttf", 16+6*x);
        TTF_SizeUTF8(ttf_font[x], " ", &spacing[x], &unused);
    }

    //texture = tms_texture_alloc();
    tms_texture_init(&sticky::texture);
    tms_texture_set_filtering(&sticky::texture, GL_LINEAR);
    tms_texture_alloc_buffer(&sticky::texture, WIDTH, HEIGHT * NUM_SLOTS, PIXELSZ);
    tms_texture_clear_buffer(&sticky::texture, 0);

    initialized = true;
}

void
sticky::_deinit(void)
{
    for (int x=0; x<NUM_SIZES; x++) {
        TTF_CloseFont(ttf_font[x]);
    }

    TTF_Quit();
}

sticky::sticky()
{
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
    //this->set_uniform("~color", 233.f/255.f, 191.f/255.f, .2f, 1.f);
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

    this->set_uniform("sprite_coords", .0f, (UV_RATIO)*(this->slot), 1.0f, (UV_RATIO)*(this->slot+1));

    tmat4_load_identity(this->M);
    tmat3_load_identity(this->N);

    this->set_num_properties(4);
    this->properties[0].type = P_STR;
    this->properties[1].type = P_INT8; /* horizontal */
    this->properties[2].type = P_INT8; /* vertical */
    this->properties[3].type = P_INT8; /* size */

    this->set_property(1, (uint8_t)1);
    this->set_property(2, (uint8_t)1);
    this->set_property(3, (uint8_t)3);

    this->set_text("Hello!");
}

sticky::~sticky()
{
    if (this->slot > 0 && this->slot < NUM_SLOTS) {
        slots[this->slot] = false;
    }
}

void
sticky::add_word(const char *word, int len)
{
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

    TTF_SizeUTF8(ttf_font[this->properties[3].v.i8], this->lines[this->currline], &w, &h);

    if (w > WIDTH) {
        char p[STICKY_MAX_PER_LINE];
        strcpy(p, this->lines[this->currline]);
        p[STICKY_MAX_PER_LINE-1] = '\0';
        do {
            p[strlen(p)-1] = 0;
            TTF_SizeUTF8(ttf_font[this->properties[3].v.i8], p, &w, NULL);
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

void
sticky::on_load(bool created, bool has_state)
{
    this->update_text();
}

void sticky::next_line()
{
    this->currline ++;
}

void sticky::draw_text(const char *txt)
{
    const char *s = txt;
    const char *w = s;

    for (;;s++) {
        if (*s == '\n') {
            if (s-w > 0)
                this->add_word(w, (int)(s-w));
            this->next_line();
            w = s+1;
            /* it's not necessary for us to handle separately since we
             * are not performing any wrapping */
        /* } else if (*s == ' ') {
            if (s-w > 0)
                this->add_word(w, (int)(s-w));
            w = s+1; */
        } else if (*s == 0) {
            if (s-w > 0)
                this->add_word(w, (int)(s-w));
            break;
        }
    }

    this->currline ++;
    if (this->currline > STICKY_MAX_LINES) this->currline = STICKY_MAX_LINES;

    /*
    tms_infof("drawed all lines:");
    for (int x=0; x<currline; x++) {
        tms_infof("line %d: '%s'", x, this->lines[x]);
    }
    */

    unsigned char *buf = tms_texture_get_buffer(&sticky::texture);
    buf += this->slot * WIDTH *PIXELSZ * HEIGHT;
    int buf_offset = this->slot * WIDTH *PIXELSZ * HEIGHT;
    //int by = this->slot * WIDTH * HEIGHT;

    int lsz = TTF_FontLineSkip(ttf_font[this->properties[3].v.i8]);

    for (int lx=0; lx<this->currline; lx++) {
        /* Skip any lines that do not contain any content */
        if (this->linelen[lx] == 0) continue;

        SDL_Surface *srf = TTF_RenderUTF8_Shaded(ttf_font[this->properties[3].v.i8],
                this->lines[lx],
                (SDL_Color){255, 255, 255, 255},
                (SDL_Color){0,0,0,0}
            );

        if (srf == NULL) {
            tms_errorf("Error creating SDL Surface:%s", TTF_GetError());
            continue;
        }

        int sy = this->properties[2].v.i8 ? HEIGHT/2 + this->currline*lsz/2. : HEIGHT-1;
        int sx = this->properties[1].v.i8 ? (WIDTH/2 - srf->w/2) * PIXELSZ : 0;

        for (int y=0; y<srf->h; y++) {
            for (int x=0; x<srf->pitch; x++) {
                for (int z=0; z<PIXELSZ; z++) {
                    int offset = ((sy-lsz*lx)-y)*WIDTH*PIXELSZ + 0+sx+x*PIXELSZ+z;
                    int data_offset = ((y)*srf->pitch)+x;

                    unsigned char data = ((unsigned char*)srf->pixels)[data_offset];

                    /*
                    tms_debugf("buf_offset/slot/offset/data_offset/data %d/%d/%d/%d/%x - '%s'",
                            buf_offset, this->slot, offset, data_offset, data, this->lines[lx]);
                            */

                    buf[offset] = data;
                }
            }
        }
        SDL_FreeSurface(srf);
    }
}

void sticky::set_text(const char *txt)
{
    this->set_property(0, txt);
    this->update_text();
}

void sticky::update_text()
{
    for (int x=0; x<STICKY_MAX_LINES; x++)
        this->linelen[x] = 0;
    this->currline = 0;

    /* clear the texture */
    int sy = HEIGHT-1;
    int sx = 0;
    unsigned char *buf = tms_texture_get_buffer(&sticky::texture);
    buf += this->slot * WIDTH * HEIGHT;

    for (int y=0; y<HEIGHT; y++) {
        for (int x=0; x<WIDTH; x++) {
            buf[((sy)-y)*WIDTH + 0+sx+x] = 0;
        }
    }

    this->draw_text(this->properties[0].v.s.buf);
    tms_texture_upload(&sticky::texture);
}

void
sticky::add_to_world()
{
    b2Fixture *f;
    this->create_rect(b2_staticBody, .76f, .76f, this->get_material(), &f);
    this->body->GetFixtureList()[0].SetSensor(true);
}

void
sticky::update(void)
{
    if (this->body) {
        b2Transform t;
        t = this->body->GetTransform();

        //tmat4_load_identity(this->M);
        this->M[0] = t.q.c;
        this->M[1] = t.q.s;
        this->M[4] = -t.q.s;
        this->M[5] = t.q.c;
        this->M[12] = t.p.x;
        this->M[13] = t.p.y;
        this->M[14] = this->get_layer()*LAYER_DEPTH - LAYER_DEPTH/2.1f;

        tmat3_copy_mat4_sub3x3(this->N, this->M);
    } else {
        tmat4_load_identity(this->M);
        tmat4_translate(this->M, this->_pos.x, this->_pos.y, -.0f);
        tmat4_rotate(this->M, this->_angle * (180.f/M_PI), 0, 0, -1);
        tmat3_copy_mat4_sub3x3(this->N, this->M);
    }
}
