#include "gui.hh"
#include "edevice.hh"
#include "font.hh"
#include "settings.hh"
#include "misc.hh"
#include <tms/bindings/cpp/cpp.hh>

#define FONT_CACHE_VERSION 34

static const char *FONT_PATH = "data/fonts/Roboto-Bold.ttf";
//static const char *FONT_PATH = "data/fonts/DejaVuSans.ttf";
//static const char *FONT_PATH = "data/fonts/DejaVuSans-Bold.ttf";

static bool initialized = false;
static bool font_init = false;

SDL_Color C_WHITE = {MENU_WHITE, 0xFF};
SDL_Color C_BLACK = {MENU_BLACK, 0xFF};

struct tms_atlas *gui_spritesheet::atlas = 0;
struct tms_atlas *gui_spritesheet::atlas_text = 0;
struct tms_atlas *gui_spritesheet::tmp_atlas = 0;

bool gui_spritesheet::tmp_atlas_modified = false;
bool gui_spritesheet::text_atlas_modified = false;
bool gui_spritesheet::all_fonts_loaded = false;

p_text *gui_spritesheet::tx_sock_tag[SOCK_TAG__COUNT];

p_text *gui_spritesheet::t_test_playing_back;
p_text *gui_spritesheet::t_get_ready;
p_text *gui_spritesheet::t_continue;
p_text *gui_spritesheet::t_win;
p_text *gui_spritesheet::t_lose;
p_text *gui_spritesheet::t_player_death;
p_text *gui_spritesheet::t_submit_score;

p_text *gui_spritesheet::t_out[NUM_SOCKET_SPRITES];
p_text *gui_spritesheet::t_in[NUM_SOCKET_SPRITES];

p_text *gui_spritesheet::tx_out[NUM_SOCKET_SPRITES];
p_text *gui_spritesheet::tx_in[NUM_SOCKET_SPRITES];

static int text_xppcm = 0;
float gui_spritesheet::text_factor = 1.f;

FT_Library gui_spritesheet::ft;

struct sprite_load_data gui_spritesheet::sprites[NUM_SPRITES] = {
    { "data/textures/ui/slider_2.png", &atlas },
    { "data/textures/ui/slider_3.png", &atlas },
    { "data/textures/ui/slider_handle.png", &atlas },
    { "data/textures/ui/btn_import.png", &atlas },
    { "data/textures/ui/btn_export.png", &atlas },
    { "data/textures/symbols/star.png", &atlas },
    { "data/textures/symbols/lock.png", &atlas },
    { "data/textures/symbols/warning.png", &atlas },
    { "data/textures/symbols/checkmark.png", &atlas },
    { "data/textures/ui/btn_marker.png", &atlas },
    { "data/icons/oil_01.png", &atlas },
    { "data/textures/ui/btn_motor_axisrot.png", &atlas },
    { "data/textures/ui/btn_conveyor_axisrot.png", &atlas },
    { "data/textures/ui/btn_select.png", &atlas },
    { "data/textures/ui/btn_wip.png", &atlas },
    { "data/textures/ui/btn_wip_2.png", &atlas },
    { "data/textures/ui/btn_plus.png", &atlas },
    { "data/textures/ui/btn_minus.png", &atlas },
    { "data/textures/ui/mouse.png", &atlas },
    { "data/textures/ui/btn_tpixel_material_multi.png", &atlas },
    { "data/textures/ui/btn_right.png", &atlas },
    { "data/textures/ui/btn_left.png", &atlas },
    { "data/textures/ui/btn_adventure_leftright.png", &atlas },
    { "data/textures/ui/btn_up.png", &atlas },
    { "data/textures/ui/btn_down.png", &atlas },
    { "data/textures/ui/dropdown_indicator.png", &atlas },
    { "data/textures/ui/btn_advup.png", &atlas },
    { "data/textures/ui/btn_advdown.png", &atlas },
    { "data/textures/ui/btn_config.png", &atlas },
    { "data/textures/ui/btn_floor.png", &atlas },
    { "data/textures/ui/btn_not_floor.png", &atlas },
    { "data/textures/ui/btn_info.png", &atlas },
    { "data/textures/ui/btn_help.png", &atlas },
    { "data/textures/ui/btn_dc.png", &atlas },
    { "data/textures/ui/btn_close.png", &atlas },
    { "data/textures/ui/btn_search.png", &atlas },
    { "data/textures/ui/btn_play.png", &atlas },
    { "data/textures/ui/btn_orthographic.png", &atlas },
    { "data/textures/ui/btn_pause.png", &atlas },
    { "data/textures/ui/btn_menu.png", &atlas },
    { "data/textures/ui/btn_radial_2.png", &atlas },
    { "data/textures/ui/btn_radial_3.png", &atlas },
    { "data/textures/ui/btn_radial_w_knob.png", &atlas },
    { "data/textures/ui/btn_radial_knob.png", &atlas },
    { "data/textures/ui/btn_field_2.png", &atlas },
    { "data/textures/ui/btn_field_3.png", &atlas },
    { "data/textures/ui/field_knob.png", &atlas },
    { "data/textures/ui/rounded_square.png", &atlas },

    { "data/textures/ui/btn_unplug.png", &atlas },
    { "data/textures/ui/btn_layervis_1.png", &atlas },
    { "data/textures/ui/btn_layervis_2.png", &atlas },
    { "data/textures/ui/btn_layervis_3.png", &atlas },
    { "data/textures/ui/btn_axis.png", &atlas },
    { "data/textures/symbols/attach.png", &atlas },
    { "data/textures/symbols/attach_rigid.png", &atlas },
    { "data/textures/symbols/attach_rotary.png", &atlas },
    { "data/textures/ui/sep.png", &atlas },
    { "data/textures/symbols/rotate.png", &atlas },
    { "data/textures/ui/btn_empty.png", &atlas },
    { "data/textures/ui/btn_multisel.png", &atlas },
    { "data/textures/ui/btn_multicable.png", &atlas },
    { "data/textures/ui/btn_multiconn.png", &atlas },
    { "data/textures/ui/btn_connedit.png", &atlas },
    { "data/textures/ui/btn_movable.png", &atlas },
    { "data/textures/ui/btn_cw.png", &atlas },
    { "data/textures/ui/btn_ccw.png", &atlas },
    { "data/textures/ui/btn_lock.png", &atlas },
    { "data/textures/ui/btn_unlock.png", &atlas },

    { "data/textures/menu/menu_play.png", &atlas },
    { "data/textures/menu/menu_create.png", &atlas },
    { "data/textures/menu/bithacklogo.png", &atlas },

    { "data/textures/ui/rounded_help.png", &atlas },

    { "data/textures/ui/btn_l1_inclusive.png", &atlas },
    { "data/textures/ui/btn_l2_inclusive.png", &atlas },
    { "data/textures/ui/btn_l3_inclusive.png", &atlas },

    { "data/textures/ui/btn_boxselect.png", &atlas },
    { "data/textures/ui/btn_select_through.png", &atlas },
    { "data/textures/ui/btn_layer_inclusion.png", &atlas },

    { "data/textures/ui/btn_l1.png", &atlas },
    { "data/textures/ui/btn_l2.png", &atlas },
    { "data/textures/ui/btn_l3.png", &atlas },

    { "data/textures/ui/btn_tpixel_material_grass.png", &atlas },
    { "data/textures/ui/btn_tpixel_material_dirt.png", &atlas },
    { "data/textures/ui/btn_tpixel_material_dirt_stone.png", &atlas },
    { "data/textures/ui/btn_tpixel_material_stone.png", &atlas },

    { "data/icons/ruby_03.png", &atlas },
    { "data/icons/sapphire_02.png", &atlas },
    { "data/icons/emerald_01.png", &atlas },
    { "data/icons/topaz_04.png", &atlas },
    { "data/icons/diamond_01.png", &atlas },
    { "data/icons/copper_01.png", &atlas },
    { "data/icons/iron_01.png", &atlas },
    { "data/icons/wood_01.png", &atlas },
    { "data/icons/aluminium.png", &atlas },
};

struct atlas_layout
{
    struct tms_atlas **atlas;
    uint32_t width;
    uint32_t height;
    uint32_t num_channels;
    int padding_x;
    int padding_y;
    GLenum filter;
};

enum {
    AL_DEFAULT,
    AL_TEXT,

    NUM_ATLASES
};

#define ATLAS_TEXT_WIDTH 2048
#define ATLAS_TEXT_HEIGHT 2048
#define ATLAS_TEXT_NUM_CHANS 1

static struct atlas_layout cache_atlases[NUM_ATLASES] = {
    {
        &gui_spritesheet::atlas,
        2048,
        1024,
        4,
        0,
        0,
        GL_LINEAR,
    },
    {
        &gui_spritesheet::atlas_text,
        ATLAS_TEXT_WIDTH,
        ATLAS_TEXT_HEIGHT,
        ATLAS_TEXT_NUM_CHANS,
        1,
        1,
        GL_LINEAR,
    },
};

static char cache_path[512];
bool gui_spritesheet::use_cache = false;

static bool
open_cache(lvlbuf *lb, const char *path)
{
    FILE *fp = fopen(path, "rb");
    if (fp) {
        fseek(fp, 0, SEEK_END);
        long size = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        lb->reset();
        lb->size = 0;
        lb->ensure((int)size);

        fread(lb->buf, 1, size, fp);

        fclose(fp);

        lb->size = size;

        return true;
    }

    return false;
}

static p_font*
read_p_font(lvlbuf *lb)
{
    int orig_height = lb->r_int32();
    uint32_t num_chars = lb->r_uint32();

    p_font *font = new p_font(FONT_PATH, orig_height);

    /* In case we've changed the amount of chars that are stored in the font. */
    if (num_chars != 128-CHAR_OFFSET) {
        return 0;
    }

    for (int x=CHAR_OFFSET; x<CHAR_OFFSET+num_chars; ++x) {
        struct glyph *g = font->get_glyph(x);

        g->ax = lb->r_int32();
        g->ay = lb->r_int32();

        g->bw = lb->r_int32();
        g->bh = lb->r_int32();

        g->obw = lb->r_int32();
        g->obh = lb->r_int32();

        g->bl = lb->r_int32();
        g->bt = lb->r_int32();

        g->minx = lb->r_int32();
        g->maxx = lb->r_int32();

        g->miny = lb->r_int32();
        g->maxy = lb->r_int32();

        g->yoffset = lb->r_int32();
        g->advance = lb->r_int32();

        g->m_sprite_buf = (unsigned char*)malloc(g->bw*g->bh);
        lb->r_buf((char*)g->m_sprite_buf, g->bw*g->bh);

        g->index = p_font::glyph_indices[x];

        g->sprite = (struct tms_sprite*)malloc(sizeof(struct tms_sprite));
        g->sprite->bl.x = lb->r_float();
        g->sprite->bl.y = lb->r_float();
        g->sprite->tr.x = lb->r_float();
        g->sprite->tr.y = lb->r_float();
        g->sprite->width = lb->r_float();
        g->sprite->height = lb->r_float();

        g->outline = (struct tms_sprite*)malloc(sizeof(struct tms_sprite));
        g->outline->bl.x = lb->r_float();
        g->outline->bl.y = lb->r_float();
        g->outline->tr.x = lb->r_float();
        g->outline->tr.y = lb->r_float();
        g->outline->width = lb->r_float();
        g->outline->height = lb->r_float();
    }

    return font;
}

static bool
read_font_cache(lvlbuf *lb)
{
    uint8_t version = lb->r_uint8();
    float read_text_factor = lb->r_float();

    if (version != FONT_CACHE_VERSION) {
        tms_errorf("Mismatching version code in model cache.");
        return false;
    }

    float diff = std::abs(read_text_factor - gui_spritesheet::text_factor);

    if (diff > 0.001f) {
        tms_errorf("Mismatching text factor. (i.e. dpi has changed)");
        return false;
    }

    struct atlas_layout *al = &cache_atlases[AL_TEXT];
    struct tms_atlas *a = *al->atlas;

    uint32_t width = lb->r_uint32();
    uint32_t height = lb->r_uint32();
    uint8_t num_channels = lb->r_uint8();

    if (width != al->width || height != al->height || num_channels != al->num_channels) {
        tms_errorf("Mismatching atlas in texture cache");
        tms_infof("%d != %d?", width, al->width);
        tms_infof("%d != %d?", height, al->height);
        tms_infof("%d != %d?", num_channels, al->num_channels);
        return false;
    }

    /* XXX: do we need to alloc texture.data?? */
    uint32_t buf_sz = width * height * num_channels;
    lb->r_buf((char*)a->texture.data, buf_sz);

    {
        p_font *f = read_p_font(lb);

        if (!f) {
            tms_errorf("Error reading small font!");
            return false;
        }

        font::small = f;
    }

    {
        p_font *f = read_p_font(lb);

        if (!f) {
            tms_errorf("Error reading medium font!");
            return false;
        }

        font::medium = f;
    }

    {
        p_font *f = read_p_font(lb);

        if (!f) {
            tms_errorf("Error reading xmedium font!");
            return false;
        }

        font::xmedium = f;
    }

    {
        p_font *f = read_p_font(lb);

        if (!f) {
            tms_errorf("Error reading large font!");
            return false;
        }

        font::large = f;
    }

    {
        p_font *f = read_p_font(lb);

        if (!f) {
            tms_errorf("Error reading xlarge font!");
            return false;
        }

        font::xlarge = f;
    }

    return true;
}

static bool
read_cache(lvlbuf *lb)
{
    uint8_t version = lb->r_uint8();
    float read_text_factor = lb->r_float();
    uint32_t num_atlases = lb->r_uint32();

    if (version != FONT_CACHE_VERSION) {
        tms_errorf("Mismatching version code in model cache.");
        return false;
    }

    if (num_atlases != NUM_ATLASES) {
        tms_errorf("Mismatching atlas count in texture cache.");
        return false;
    }

    if (read_text_factor != gui_spritesheet::text_factor) {
        tms_errorf("Mismatching text factor. (i.e. dpi has changed)");
        return false;
    }

    char *data;
    for (int x=0; x<num_atlases; ++x) {
        struct atlas_layout *al = &cache_atlases[x];
        struct tms_atlas *a = *al->atlas;

        uint32_t width = lb->r_uint32();
        uint32_t height = lb->r_uint32();
        uint8_t num_channels = lb->r_uint8();

        if (width != al->width || height != al->height || num_channels != al->num_channels) {
            tms_infof("%d != %d?", width, al->width);
            tms_infof("%d != %d?", height, al->height);
            tms_infof("%d != %d?", num_channels, al->num_channels);
            tms_fatalf("Mismatching atlas in texture cache");
            return false;
        }

        uint32_t buf_sz = width * height * num_channels;
        lb->r_buf((char*)a->texture.data, buf_sz);
    }

    /* Load all generic sprites */
    uint32_t n = lb->r_uint32();
    if (n != NUM_SPRITES) {
        tms_errorf("Mismatching number of sprites in texture cache");
        return false;
    }
    for (int x=0; x<n; ++x) {
        uint32_t id = lb->r_uint32();
        struct tms_sprite *s = gui_spritesheet::sprites[id].sprite;
        s = (struct tms_sprite*)malloc(sizeof(struct tms_sprite));

        s->bl.x = lb->r_float();
        s->bl.y = lb->r_float();
        s->tr.x = lb->r_float();
        s->tr.y = lb->r_float();
        s->width = lb->r_float();
        s->height = lb->r_float();
    }

    return true;
}

void
gui_spritesheet::load_cache()
{
    lvlbuf lb;

    if (!open_cache(&lb, cache_path)) {
        tms_errorf("Error openin cache, reverting to non-cached sprite loading.");
        gui_spritesheet::use_cache = false;
    } else {
        if (!read_cache(&lb)) {
            tms_errorf("Error reading cache, reverting to non-cached sprite loading.");
            gui_spritesheet::use_cache = false;
        }
    }

    if (lb.buf) {
        free(lb.buf);
    }
}

static void
write_sprite(lvlbuf *lb, struct tms_sprite *s)
{
    lb->w_s_float(s->bl.x);
    lb->w_s_float(s->bl.y);
    lb->w_s_float(s->tr.x);
    lb->w_s_float(s->tr.y);
    lb->w_s_float(s->width);
    lb->w_s_float(s->height);
}

static bool
write_cache(lvlbuf *lb)
{
    lb->w_s_uint8(FONT_CACHE_VERSION);
    lb->w_s_float(gui_spritesheet::text_factor);
    lb->w_s_uint32(NUM_ATLASES);

    for (int x=0; x<NUM_ATLASES; ++x) {
        struct atlas_layout *al = &cache_atlases[x];
        struct tms_atlas *a = *al->atlas;

        lb->w_s_uint32(al->width);
        lb->w_s_uint32(al->height);
        lb->w_s_uint8(al->num_channels);

        uint32_t buf_sz = al->width * al->height * al->num_channels;
        lb->w_s_buf((char*)a->texture.data, buf_sz);
    }

    /* Save all generic sprites */
    lb->w_s_uint32(NUM_SPRITES);
    for (int x=0; x<NUM_SPRITES; ++x) {
        struct tms_sprite *s = gui_spritesheet::sprites[x].sprite;

        lb->w_s_uint32(x);
        write_sprite(lb, s);
    }

    /* save small font */
    for (int x=0; x<128-CHAR_OFFSET; ++x) {

    }

    return true;
}

static void
write_p_font(lvlbuf *lb, p_font *font)
{
    lb->w_s_int32(font->get_orig_height()); /* font height */

    lb->w_s_uint32(128-CHAR_OFFSET); /* num chars */
    /* equal required */

    for (int x=CHAR_OFFSET; x<128; ++x) {
        struct glyph *g = font->get_glyph(x);

        lb->ensure(
                 sizeof(int32_t) /* Glyph advance X */
                +sizeof(int32_t) /* Glyph advance Y */

                +sizeof(int32_t) /* Glyph bitmap width */
                +sizeof(int32_t) /* Glyph bitmap height */

                +sizeof(int32_t) /* Glyph outline bitmap width */
                +sizeof(int32_t) /* Glyph outline bitmap height */

                +sizeof(int32_t) /* Glyph bitmap left offset */
                +sizeof(int32_t) /* Glyph bitmap top offset */

                +sizeof(int32_t) /* Glyph min X */
                +sizeof(int32_t) /* Glyph max X */

                +sizeof(int32_t) /* Glyph min Y */
                +sizeof(int32_t) /* Glyph max Y */

                +sizeof(int32_t) /* Glyph Y offset */
                +sizeof(int32_t) /* Glyph hori advance */

                +(g->bw*g->bh)+  /* Bitmap data buf */

                +(sizeof(float)*6) /* Sprite data */

                +(sizeof(float)*6) /* Outline sprite data */
                );

        lb->w_int32(g->ax);
        lb->w_int32(g->ay);

        lb->w_int32(g->bw);
        lb->w_int32(g->bh);

        lb->w_int32(g->obw);
        lb->w_int32(g->obh);

        lb->w_int32(g->bl);
        lb->w_int32(g->bt);

        lb->w_int32(g->minx);
        lb->w_int32(g->maxx);

        lb->w_int32(g->miny);
        lb->w_int32(g->maxy);

        lb->w_int32(g->yoffset);
        lb->w_int32(g->advance);

        lb->w_buf((char*)g->m_sprite_buf, g->bw*g->bh);

        lb->w_float(g->sprite->bl.x);
        lb->w_float(g->sprite->bl.y);
        lb->w_float(g->sprite->tr.x);
        lb->w_float(g->sprite->tr.y);
        lb->w_float(g->sprite->width);
        lb->w_float(g->sprite->height);

        lb->w_float(g->outline->bl.x);
        lb->w_float(g->outline->bl.y);
        lb->w_float(g->outline->tr.x);
        lb->w_float(g->outline->tr.y);
        lb->w_float(g->outline->width);
        lb->w_float(g->outline->height);
    }
}

static bool
write_font_cache(lvlbuf *lb)
{
    Uint32 begin, a1, a2, a3, end;

    begin = SDL_GetTicks();

    lb->w_s_uint8(FONT_CACHE_VERSION);
    /* equal required */

    lb->w_s_float(gui_spritesheet::text_factor);
    /* rough equal required */

    struct atlas_layout *al = &cache_atlases[AL_TEXT];
    struct tms_atlas *a = *al->atlas;

    lb->w_s_uint32(al->width);
    /* equal required */
    lb->w_s_uint32(al->height);
    /* equal required */
    lb->w_s_uint8(al->num_channels);
    /* equal required */

    uint32_t buf_sz = al->width * al->height * al->num_channels;
    lb->w_s_buf((char*)a->texture.data, buf_sz);

    a1 = SDL_GetTicks();
    write_p_font(lb, font::small);
    a2 = SDL_GetTicks();
    write_p_font(lb, font::medium);
    write_p_font(lb, font::xmedium);
    a3 = SDL_GetTicks();
    write_p_font(lb, font::large);
    tms_infof("OK FROM HERE --------------");
    write_p_font(lb, font::xlarge);
    tms_infof("DONE");

    end = SDL_GetTicks();

    tms_infof("begin: (%d) %d", begin-begin, begin);
    tms_infof("a1:    (%d) %d", a1-begin, a1);
    tms_infof("a2:    (%d) %d", a2-a1, a2);
    tms_infof("a3:    (%d) %d", a3-a2, a3);
    tms_infof("end:   (%d) %d", end-a3, end);

    return true;

}

static bool
save_cache(lvlbuf *lb, const char *path)
{
    FILE *fp = fopen(path, "wb");
    if (fp) {
        fwrite(lb->buf, 1, lb->size, fp);
        fclose(fp);

        return true;
    }

    return false;
}

static bool base_font_init = false;

void
gui_spritesheet::init_atlas()
{
    for (int x=0; x<NUM_ATLASES; ++x) {
        struct atlas_layout *al = &cache_atlases[x];
        struct tms_atlas *a = tms_atlas_alloc(al->width, al->height, al->num_channels);

        a->padding_x = al->padding_x;
        a->padding_y = al->padding_y;
        a->texture.filter = al->filter;

        *(al->atlas) = a;
    }

    text_xppcm = _tms.xppcm;
    if (text_xppcm > 120) {
        text_xppcm = 120;
    }

    text_factor = (float)_tms.xppcm / (float)text_xppcm;

    snprintf(cache_path, 511, "%s/textures.cache", tbackend_get_storage_path());
    gui_spritesheet::use_cache = false;

    tms_infof("Textures cache path: %s", cache_path);

    if (!settings["always_reload_data"]->v.b && file_exists(cache_path)) {
        tms_infof("Checking if we want to use cache...");
        /* The cache file exists, make sure we want to use it. */
        gui_spritesheet::use_cache = true;
#ifndef TMS_BACKEND_ANDROID
        time_t cache_mtime = get_mtime(cache_path);
        time_t sprite_mtime;

        for (int x=0; x<NUM_SPRITES; ++x) {
            if (!gui_spritesheet::sprites[x].path) continue;

            const char *path = gui_spritesheet::sprites[x].path;

            sprite_mtime = get_mtime(path);
            if (sprite_mtime >= cache_mtime) {
                tms_infof("Not using cache, %s has been modified", path);
                gui_spritesheet::use_cache = false;
                break;
            }
        }
#endif
    }

    tms_infof("Initialzing freetype...");
    if (FT_Init_FreeType(&gui_spritesheet::ft)) {
        tms_fatalf("Unable to init freetype library");
    }
}

static bool use_font_cache = true;
static char font_cache_path[512];

static const bool disable_font_cache = false;
static const bool disable_gui_cache = true;

void
gui_spritesheet::init_loading_font()
{
    if (base_font_init) {
        return;
    }

    snprintf(font_cache_path, 511, "%s/fonts.cache", tbackend_get_storage_path());

    lvlbuf lb;

    atlas_text->texture.filter = GL_LINEAR;

    if (!open_cache(&lb, font_cache_path) || disable_font_cache) {
        tms_errorf("Error opening font cache at '%s', reverting to non-cached font loading.", font_cache_path);
        use_font_cache = false;
    } else {
        if (!read_font_cache(&lb)) {
            tms_errorf("Error reading font cache at '%s', reverting to non-cached font loading.", font_cache_path);
            use_font_cache = false;
        }
    }

    if (lb.buf) {
        free(lb.buf);
    }

    if (use_font_cache) {
        tms_infof("Successfully read font info from cache at '%s'", font_cache_path);
        base_font_init = true;
        font_init = true;
        gui_spritesheet::all_fonts_loaded = true;

        gui_spritesheet::upload_text_atlas();

        return;
    }

    base_font_init = true;

    /* Can we load all fonts from cache? */

#define BG_RESERVE 4
    /* add a white piece that can be used to render backgrounds */
    uint8_t *tx = atlas_text->texture.data;
    atlas_text->current_x = BG_RESERVE;
    atlas_text->current_y = 0;
    atlas_text->current_height = BG_RESERVE;
    for (int y=0; y<BG_RESERVE; y++) {
        for (int x=0; x<BG_RESERVE; x++) {
            for (int z=0; z<ATLAS_TEXT_NUM_CHANS; z++) {
                tx[ATLAS_TEXT_NUM_CHANS*(ATLAS_TEXT_WIDTH*(ATLAS_TEXT_HEIGHT-1-y)) + ATLAS_TEXT_NUM_CHANS*x + z] = 255;
            }
        }
    }
#undef BG_RESERVE

    tms_debugf("Initializing medium sized font...");
    font::medium  = new p_font(atlas_text, FONT_PATH, (int)(round(text_xppcm/4.5)));
    tms_debugf("Done");

    gui_spritesheet::upload_text_atlas();
}

void
gui_spritesheet::init_fonts()
{
    if (font_init)
        return;

    font_init = true;

    font::small = new p_font(atlas_text, FONT_PATH, (int)(round(text_xppcm / 5.5)));
    font::xmedium = new p_font(atlas_text, FONT_PATH, (int)(round(text_xppcm/3.5)));
    font::large = new p_font(atlas_text, FONT_PATH, (int)(round(text_xppcm/2.0)));
    font::xlarge = new p_font(atlas_text, FONT_PATH, (int)(round(text_xppcm/1.5)));
    //font::small = new p_font(atlas_text, FONT_PATH, 16);

    gui_spritesheet::all_fonts_loaded = true;

    //gui_spritesheet::text_atlas_modified = true;
    // use text_atlas_modified if trying to multithread this shit

    if (!use_font_cache && !disable_font_cache) {
        lvlbuf lb;

        if (!write_font_cache(&lb)) {
            tms_errorf("Unable to write font cache to '%s'", font_cache_path);
        } else {
            if (!save_cache(&lb, font_cache_path)) {
                tms_errorf("Unable to save font cache to '%s'", font_cache_path);
            }
        }
    }

    gui_spritesheet::upload_text_atlas();
}

void
gui_spritesheet::upload_text_atlas()
{
    tms_debugf("Uploading text atlas...");
    tms_texture_upload(&atlas_text->texture);

    gui_spritesheet::text_atlas_modified = false;
    tms_debugf("Done");
}

void
gui_spritesheet::init()
{
    if (initialized) {
        return;
    }

    initialized = true;

    /* XXX FIXME: temporarily increased size of atlas from
     * 1024x1024 to 2048x1024 */
    atlas->padding_x = 2;
    atlas->padding_y = 2;

    for (int x=0; x< NUM_SPRITES; ++x) {
        struct sprite_load_data *sld = &gui_spritesheet::sprites[x];
        gui_spritesheet::add(*sld->atlas, &sld->sprite, sld->path);
    }

    t_test_playing_back = new p_text(font::small);
    t_get_ready = new p_text(font::large);
    t_continue = new p_text(font::small);
    t_win = new p_text(font::xlarge);
    t_lose = new p_text(font::xlarge);
    t_player_death = new p_text(font::xlarge);
    t_submit_score = new p_text(font::large);

#ifdef TMS_BACKEND_PC
    t_test_playing_back->set_text("Test-playing level. Press B to return to sandbox.");
    t_get_ready->set_text("CLICK TO BEGIN");
#else
    t_test_playing_back->set_text("Test-playing level. Press Back to return to sandbox.");
    t_get_ready->set_text("TOUCH TO BEGIN");
#endif

    t_continue->set_text("Continue >>");
    t_win->set_text("Level completed!");
    t_lose->set_text("Game over");
    t_player_death->set_text("You died");
    t_submit_score->set_text("Submit score");


    char ss[64];
    ss[1] = '\0';

    tms_texture_upload(&atlas->texture);

    for (int x=0; x<NUM_SOCKET_SPRITES; x++) {
        sprintf(ss, "OUT%d", x);
        t_out[x] = new p_text(font::medium);
        t_out[x]->set_text(ss);

        tx_out[x] = new p_text(font::xlarge, ALIGN_CENTER, ALIGN_BOTTOM);
        tx_out[x]->set_text(ss);
        tx_out[x]->color = TV_BLACK;

        sprintf(ss, "IN%d", x);
        t_in[x] = new p_text(font::medium);
        t_in[x]->set_text(ss);

        tx_in[x] = new p_text(font::xlarge, ALIGN_CENTER, ALIGN_BOTTOM);
        tx_in[x]->set_text(ss);
        tx_in[x]->color = TV_BLACK;
    }

    static const char *edev_tags[SOCK_TAG__COUNT] = {
        "",
        "speed",
        "force",
        "~reverse",
        "~focus",
        "value",
        "feedback",
        "set-value",
        "~set",
        "state",
        "f-state",
        "f-force",
        "tradeoff",
        "f-speed",
        "f-error",
        "angle",
        "distance",
        "tick",
        "~status",
        "~on/off",
        "~reset",
        "~",
        "increase",
        "decrease",
        "fraction",
        "multiplier",
        "win",
        "lose",
        "restart",
        "score",
        "+1",
        "+50",
        "+100",
        "+250",
        "+500",
        "-1",
        "-50",
        "-100",
        "-250",
        "-500",
        "<",
        ">",
        "^",
        "v",
        "+",
        "-",
        "attack",
        "respawn",
        "freeze",
    };

    for (int x=1; x<sizeof(edev_tags)/sizeof(char*); x++) {
        tx_sock_tag[x] = new p_text(font::xlarge);
        tx_sock_tag[x]->set_text(edev_tags[x]);
        tx_sock_tag[x]->color = TV_WHITE;
    }

    tmp_atlas = tms_atlas_alloc(1024, 128, 4);
    tmp_atlas->padding_x = 1;
    tmp_atlas->padding_y = 1;

    if (!gui_spritesheet::use_cache && !disable_gui_cache) {
        /* write cache file! */
        lvlbuf lb;

        /* dump models to cache file */
        tms_debugf("DUMP SPRITE DATA");

        if (!write_cache(&lb)) {
            tms_errorf("An error occured while trying write sprite cache.");
        } else {
            if (!save_cache(&lb, cache_path)) {
                tms_errorf("An error occured while trying to save sprite cache to a file. (not enough permission/disk space?)");
            } else {
                tms_infof("successfully saved cache");
            }
        }
    }
}

void
gui_spritesheet::deinit()
{
    delete font::small;
    delete font::medium;
    delete font::xmedium;
    delete font::large;
    delete font::xlarge;

    for (int x=0; x< NUM_SPRITES; ++x) {
        struct sprite_load_data *sld = &gui_spritesheet::sprites[x];

        if (sld->sprite) {
            free(sld->sprite);
        }
    }
}

void
gui_spritesheet::add(struct tms_atlas *atlas, struct tms_sprite **s, const char *path)
{
    *s = tms_atlas_add_file(atlas, path, 1);

    if (!*s) {
        tms_errorf("Error adding file %s", path);
    }
}

void
gui_spritesheet::cleanup()
{
    tms_atlas_free(gui_spritesheet::tmp_atlas);
    tms_atlas_free(gui_spritesheet::atlas);
    tms_atlas_free(gui_spritesheet::atlas_text);
}
