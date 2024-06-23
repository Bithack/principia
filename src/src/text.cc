#include "text.hh"
#include "textbuffer.hh"
#include "gui.hh"

//#define DRAW_GLYPH_BOUNDING_BOX

p_text::p_text(p_font *font, uint8_t horizontal_align/*=ALIGN_CENTER*/, uint8_t vertical_align/*=ALIGN_CENTER*/)
    : width(0.f)
    , height(0.f)
    , max_height(0.f)
    , hmod(0.f)
    , x(0.f)
    , y(0.f)
    , num_lines(0)
    , real_scale(1.f)
{
    this->font = font;
    this->rendering = false;
    this->text = 0;
    this->glyphs = 0;
    this->num_glyphs = 0;
    this->active = true;
    this->scale = tvec2f(1.f, 1.f);

    this->horizontal_align = horizontal_align;
    this->vertical_align = vertical_align;

    this->color = tvec4f(1.f, 1.f, 1.f, 1.f);
    this->outline_color = tvec4f(0.f, 0.f, 0.0f, 1.f);
}

p_text::~p_text()
{
    delete [] this->glyphs;

    if (this->text) {
        free(this->text);
    }
}

/* NOTE: This function is not threadsafe */
void
p_text::set_text(const char *new_text, bool calculate/*=true*/)
{
    tms_assertf(this->rendering == false, "p_text::set_text rendering == true");

    if (this->text && new_text) {
        if (strcmp(this->text, new_text) == 0) {
            return;
        }
    }

    if (this->text) {
        free(this->text);
        this->text = 0;
    }

    if (this->glyphs) {
        delete [] this->glyphs;
        this->glyphs = 0;
    }

    if (new_text) {
        this->text = strdup(new_text);

        this->num_glyphs = strlen(this->text);

        this->glyphs = new p_text::text_glyph[this->num_glyphs];

        for (int i=0; i<this->num_glyphs; ++i) {
            glyph *g = this->font->get_glyph(this->text[i]);
            if (!g) {
                g = this->font->get_glyph('?');
            }
            this->glyphs[i].set_parent(g);
            this->glyphs[i].c = this->text[i];
        }

        if (calculate) {
            this->calculate();
        }
    }
}

void
p_text::set_position(float x, float y)
{
    this->x = x;
    this->y = y;
}

void
p_text::calculate(uint8_t horizontal_align/*=ALIGN_CENTER*/, uint8_t vertical_align/*=ALIGN_CENTER*/, int nl_dir/*=0*/)
{
    this->num_lines = 1;

    if (nl_dir == 0) {
        if (vertical_align == ALIGN_BOTTOM) {
            nl_dir = NL_DIR_UP;
        } else {
            nl_dir = NL_DIR_DOWN;
        }
    }

    float x = 0;
    float y = 0;

    int _x, _z;
    int minx, maxx;
    int miny, maxy;

    minx = maxx = 0;
    miny = maxy = 0;
    _x = 0;

    p_text::text_glyph *g = 0;
    int c = 0;
    int p = -1;
    int max_width = 0;
    int max_height = 0;
    for (int i=0; i<this->num_glyphs; ++i) {
        g = &this->glyphs[i];

        if (g->is_newline()) {
            if (x > max_width) {
                max_width = x;
            }
            x = 0.f;
            y += nl_dir * this->font->get_height();
            ++ this->num_lines;
            continue;
        }

        c = g->c;

        if (p != -1) {
            FT_Vector kerning;
            FT_Get_Kerning(this->font->get_face(),
                    FT_Get_Char_Index(this->font->get_face(), p),
                    FT_Get_Char_Index(this->font->get_face(), c),
                    FT_KERNING_DEFAULT,
                    &kerning
                    );

            x += kerning.x >> 6;
        }

        float x2 = x + g->get_bl() + g->get_bw()/2.f;
        float y2 = y + g->get_bt() - g->get_bh()/2.f;

        x += g->get_ax();
        y += g->get_ay();

        if (!g->get_bw() || !g->get_bh()) {
            continue;
        }

        g->x = x2;
        g->y = y2;

        if (g->y + g->get_bh() > max_height) {
            max_height = g->y + g->get_bh();
        }

        _z = _x + g->parent->minx;
        if (minx > _z) {
            minx = _z;
        }

        if (g->parent->advance > g->parent->maxx) {
            _z = _x + g->parent->advance;
        } else {
            _z = _x + g->parent->maxx;
        }

        if (maxx < _z) {
            maxx = _z;
        }
        _x += g->parent->advance;

        if (g->parent->miny < miny) {
            miny = g->parent->miny;
        }
        if (g->parent->maxy > maxy) {
            maxy = g->parent->maxy;
        }

        p = c;
    }

    this->width = (x > max_width ? x : max_width);
    //this->width = (maxx - minx);
    this->height = this->num_lines * this->font->get_height();
    //this->max_height = max_height;
    //this->max_height = (maxy - miny);
    this->max_height = (this->font->ascent - miny);
    float height_sub1 = (this->num_lines-1) * this->font->get_height();

    float x_offset = 0.f;
    float y_offset = 0.f;

    switch (horizontal_align) {
        case ALIGN_CENTER:
            x_offset = -this->get_width()/2.f;
            break;

        case ALIGN_RIGHT:
            x_offset = -this->get_width();
            break;

        case ALIGN_LEFT:
        default:
            break;
    }

    switch (vertical_align) {
        case ALIGN_TOP:
            if (nl_dir == NL_DIR_UP) {
                y_offset = -this->get_height();
            } else {
                y_offset = -this->font->get_height();
            }
            break;
        case ALIGN_CENTER:
            if (nl_dir == NL_DIR_UP) {
                y_offset = -this->get_height() / 2.f;
            } else {
                //y_offset = -this->font->get_height() + this->get_height() / 2.f;
                //y_offset = (maxy - miny) / 2.f;
                y_offset = -this->get_height() / 2.f - miny;
                float cool = -((maxy+miny)/2.f);

                y_offset -= y_offset - cool;
            }
            break;

        case ALIGN_BOTTOM:
        default:
            if (nl_dir == NL_DIR_DOWN) {
                y_offset = height_sub1;
            }
            break;
    }

    for (int i=0; i<this->num_glyphs; ++i) {
        g = &this->glyphs[i];

        g->x += x_offset;
        g->y += y_offset;
    }
}

void
p_text::render(struct tms_ddraw *dd, bool outline/*=false*/, bool call_opengl_stuff/*=true*/)
{
    this->render_at_pos(dd, this->get_x(), this->get_y(), outline, call_opengl_stuff);
}

void
p_text::render_at_pos(struct tms_ddraw *dd, float x, float y, bool outline/*=false*/, bool call_opengl_stuff/*=true*/)
{
    if (!this->text) {
        return;
    }

    this->rendering = true;

    if (call_opengl_stuff) {
        glEnable(GL_BLEND);
        glDisable(GL_DEPTH_TEST);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gui_spritesheet::atlas_text->texture.gl_texture);
    }


    p_text::text_glyph *g = 0;
    float base_x = x;
    float base_y = y;

#ifdef DRAW_GLYPH_BOUNDING_BOX
    tms_ddraw_set_color(dd, 1.f, 0.f, 1.f, 1.f);
    tms_ddraw_circle(dd, base_x, base_y, 2.f, 2.f);
#endif

    for (int i=0; i<this->num_glyphs; ++i) {
        g = &this->glyphs[i];

        if (!g->get_sprite()) continue;

        if (outline) {
            tms_ddraw_set_sprite1c_color(dd, outline_color.r, outline_color.g, outline_color.b, outline_color.a);
            tms_ddraw_sprite1c(dd,
                    g->get_outline(),
                    base_x + g->x * this->real_scale,
                    base_y + g->y * this->real_scale,
                    this->real_scale * g->get_outline()->width,
                    this->real_scale * g->get_outline()->height
                    );
        }

#ifdef DRAW_GLYPH_BOUNDING_BOX
        tms_ddraw_set_color(dd, 1.f, 1.f, 1.f, .5f);
        tms_ddraw_lsquare(dd,
                base_x + g->x * this->real_scale,
                base_y + g->y * this->real_scale,
                this->real_scale * g->get_bw(),
                this->real_scale * g->get_bh()
                );
#endif

        tms_ddraw_set_sprite1c_color(dd, color.r, color.g, color.b, color.a);
        tms_ddraw_sprite1c(dd,
                g->get_sprite(),
                base_x + g->x * this->real_scale,
                base_y + g->y * this->real_scale,
                this->real_scale * g->get_bw(),
                this->real_scale * g->get_bh()
                );
    }

    this->rendering = false;
}

void
p_text::tb_render(bool outline/*=false*/)
{
    p_text::text_glyph *g = 0;
    for (int i=0; i<this->num_glyphs; ++i) {
        g = &this->glyphs[i];

        if (outline) {
            /*
            tms_ddraw_set_sprite1c_color(dd, outline_color.r, outline_color.g, outline_color.b, outline_color.a);
            tms_ddraw_sprite1c(dd,
                    g->get_outline(),
                    g->x, g->y,
                    g->get_outline()->width, g->get_outline()->height
                    );
                    */
        }

        textbuffer::add_char2(g->parent,
                g->x, g->y,
                2.0,
                color.r, color.g, color.b, color.a,
                g->get_bw(),
                g->get_bh()
                );
    }
}

tms::texture*
p_text::create_texture()
{
    //tms_infof("\n\n\n -------------\n\n");
    tms::texture *tex = new tms::texture();
    unsigned char *texbuf = tex->alloc_buffer(this->get_width(), this->get_max_height(), 4);

    //tms_infof("Width/height: %d/%d", this->get_width(), this->get_max_height());
    tex->clear_buffer(0);

    int pitch = tex->get_width() * tex->get_num_channels();
    pitch = (pitch + 3) & ~3;

    //tms_infof("Pitch: %d", pitch);

    uint8_t *src;
    uint32_t *dst;
    uint32_t *dst_check;
    uint32_t alpha;
    int xstart = 0;
    int width;

    FT_UInt prev_index = 0;

    uint32_t pixel;
    pixel = (255 << 16) | (255 << 8) | (255);

    int row, col;

    dst_check = (uint32_t*)texbuf + pitch/4 * tex->get_height();

    p_text::text_glyph *g = 0;
    for (int i=0; i<this->num_glyphs; ++i) {
        g = &this->glyphs[i];

        if (!g) {
            continue;
        }

        width = g->get_bw();
        if (width > g->parent->maxx - g->parent->minx) {
            width = g->parent->maxx - g->parent->minx;
        }

        if (prev_index && g->parent->index) {
            FT_Vector delta;
            FT_Get_Kerning(this->font->get_face(), prev_index, g->parent->index, FT_KERNING_DEFAULT, &delta);
            xstart += delta.x >> 6;
        }

        unsigned char *sprite_buf = g->parent->get_sprite_buf();

        //tms_infof("sprite buf: '%s'", sprite_buf);

        int glyph_pitch = width * 1;

        int bh = g->get_bh();
        for (row=0; row<bh; ++row) {
            if (row + g->parent->yoffset < 0) {
                continue;
            }

            if (row + g->parent->yoffset >= tex->get_height()) {
                continue;
            }

            dst = (uint32_t*)texbuf +
                (row+g->parent->yoffset) * pitch/4 +
                xstart + g->parent->minx;

            src = (uint8_t*)(sprite_buf + glyph_pitch * row);
            for (col = width; col>0 && dst < dst_check; --col) {
                alpha = *src++;
                *dst++ |= pixel | (alpha << 24);
            }
        }

        xstart += g->parent->advance;

        //tms_infof("[%d] maxx: %d", i, g->parent->maxy);
        //tms_infof("[%d] minx: %d", i, g->parent->miny);

        prev_index = g->parent->index;
    }

    //tex->flip_y();
    tex->upload();

    return tex;
}

struct tms_sprite*
p_text::add_to_atlas(struct tms_atlas *a, const char *text)
{
    struct tms_sprite *s;
    tms::texture *tex;
    this->set_text(text);

    tex = this->create_texture();
    s = tms_atlas_add_bitmap(a, tex->width, tex->height, 4, tex->data);
    delete tex;

    return s;
}
