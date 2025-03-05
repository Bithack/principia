#pragma once

#define NL_DIR_DOWN -1
#define NL_DIR_UP 1

#define CHAR_OFFSET 32

#ifndef FT_CEIL
#define FT_CEIL(X)  (((X + 63) & -64) / 64)
#endif

#ifndef FT_FLOOR
#define FT_FLOOR(X) ((X & -64) / 64)
#endif

#include <tms/math/vector.h>
#include <SDL_rwops.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H
#include FT_STROKER_H
#include FT_GLYPH_H
#include FT_TRUETYPE_IDS_H

enum text_align {
    ALIGN_CENTER,
    ALIGN_LEFT,
    ALIGN_RIGHT,
    ALIGN_BOTTOM,
    ALIGN_TOP
};

struct glyph
{
    ~glyph();
    unsigned char *get_sprite_buf();

    int ax;
    int ay;

    int bw;
    int bh;
    int obw;
    int obh;

    int bl;
    int bt;

    int minx;
    int maxx;
    int miny;
    int maxy;
    int yoffset;
    int advance;

    bool newline;
    FT_UInt index;

    unsigned char *m_sprite_buf;
    unsigned char *m_outline_buf;

    struct tms_sprite *sprite;
    struct tms_sprite *outline;

};

class p_font
{
  public:
    int orig_height;

    glyph glyphs[128-CHAR_OFFSET];
    FT_Face face;
    SDL_RWops *rw;

    p_font(const char *font_path, int height);
    p_font(struct tms_atlas *atlas, const char *font_path, int height);
    ~p_font();

    int height;
    int ascent;
    int descent;
    int lineskip;

    struct glyph *get_glyph(int c);
    inline FT_Face get_face()
    {
        return this->face;
    };

    inline int get_height()
    {
        return this->height;
    }

    inline int get_orig_height()
    {
        return this->orig_height;
    }

    static FT_UInt *glyph_indices;
};

#ifdef TMS_BACKEND_WINDOWS
#undef small
#endif

namespace font
{
extern p_font *small;
extern p_font *medium;
extern p_font *xmedium;
extern p_font *large;
extern p_font *xlarge;
}

void render_glyph(struct tms_ddraw *dd, struct glyph *g, float x, float y, tvec4 c, tvec4 oc, float scale=1.f, bool outline=false, bool call_opengl_stuff=true);
