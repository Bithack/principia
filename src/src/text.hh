#pragma once

#include "font.hh"
#include <tms/math/glob.h>

namespace tms
{
class texture;
}

class p_text
{
  private:
    int width;
    int height;
    int max_height;
    float hmod;
    float x;
    float y;
    uint8_t num_lines;
    bool rendering;
    uint8_t horizontal_align;
    uint8_t vertical_align;
    float real_scale;

  public:
    uint32_t num_glyphs;
    p_font *font;
    char *text;
    bool active;

    class text_glyph
    {
      public:
        struct glyph *parent;
        char c;
        float x;
        float y;

        text_glyph()
            : x(0)
            , y(0)
        { }

        void set_parent(struct glyph *g)
        {
            this->parent = g;
        }

        inline int get_ax() { return this->parent->ax; }
        inline int get_ay() { return this->parent->ay; }

        inline int get_bw() { return this->parent->bw; }
        inline int get_bh() { return this->parent->bh; }

        inline int get_bl() { return this->parent->bl; }
        inline int get_bt() { return this->parent->bt; }

        inline bool is_newline() { return this->parent->newline; }

        inline struct tms_sprite *get_sprite() { return this->parent->sprite; }
        inline struct tms_sprite *get_outline() { return this->parent->outline; }
    } *glyphs;

    p_text(p_font *f, uint8_t horizontal_align=ALIGN_CENTER, uint8_t vertical_align=ALIGN_CENTER);

    ~p_text();

    void set_text(const char *new_text, bool calculate=true);

    tvec4 color;
    tvec4 outline_color;
    tvec2 scale;

    inline float get_x() const
    {
        return this->x;
    }

    inline float get_y() const
    {
        return this->y;
    }

    inline int get_width() const
    {
        return this->width * this->get_scale();
    }

    inline int get_height() const
    {
        //return this->height * this->scale.y;
        return this->height * this->get_scale();
    }

    inline int get_max_height() const
    {
        return this->max_height;
    }

    inline int get_num_lines() const
    {
        return this->num_lines;
    }

    void set_alignment(uint8_t horizontal_align, uint8_t vertical_align)
    {
        this->horizontal_align = horizontal_align;
        this->vertical_align = vertical_align;
    }

    void set_vertical_align(uint8_t vertical_align)
    {
        this->vertical_align = vertical_align;
    }

    /* set position must be called for a text before it can be rendered properly */
    void set_position(float x, float y);
    void set_scale(float scale)
    {
        this->real_scale = scale;
    }

    inline float get_scale() const
    {
        return this->real_scale;
    }

    void calculate(int nl_dir=NL_DIR_DOWN)
    {
        this->calculate(this->horizontal_align, this->vertical_align, nl_dir);
    }
    void calculate(uint8_t horizontal_align, uint8_t vertical_align, int nl_dir=NL_DIR_DOWN);

    void render(struct tms_ddraw *dd, bool outline=false, bool call_opengl_stuff=true);
    void render_at_pos(struct tms_ddraw *dd, float x, float y, bool outline=false, bool call_opengl_stuff=true);
    void tb_render(bool outline=false);
    struct tms_sprite *add_to_atlas(struct tms_atlas *a, const char *text);

  private:
    tms::texture *create_texture();
};
