#include "font.hh"
#include "gui.hh"

p_font *font::small = 0;
p_font *font::medium = 0;
p_font *font::xmedium = 0;
p_font *font::large = 0;
p_font *font::xlarge = 0;

FT_UInt *p_font::glyph_indices = 0;

#define FT_FLOOR(X) ((X & -64) / 64)
#define FT_CEIL(X)  (((X + 63) & -64) / 64)

glyph::~glyph()
{
    if (this->m_sprite_buf) {
        free(this->m_sprite_buf);
    }

    if (this->m_outline_buf) {
        free(this->m_outline_buf);
    }

    if (this->sprite) {
        free(this->sprite);
    }

    if (this->outline) {
        free(this->outline);
    }
}

unsigned char*
glyph::get_sprite_buf()
{
    if (this->m_sprite_buf == 0) {
        if (this->bw == 0 || this->bh == 0) {
            return 0;
        }

        struct tms_texture *tex = &gui_spritesheet::atlas_text->texture;

        int oy = tex->num_channels * tex->width;
        int ox = tex->num_channels;

        this->m_sprite_buf = (unsigned char*)malloc(this->bw * this->bh);
        for (int y=0; y<this->bh; ++y) {
            for (int x=0; x<this->bw; ++x) {
                int _y = (this->bh-y) + (int)(this->sprite->bl.y*tex->height);
                int _x = x + (int)(this->sprite->bl.x*tex->width);
                // we're cheeky so we only care about alpha
                this->m_sprite_buf[(y*(int)this->bw)+x] =
                    tex->data[_y*oy+_x*ox+3];
            }
        }
    }

    return this->m_sprite_buf;
}

static unsigned long RWread(
    FT_Stream stream,
    unsigned long offset,
    unsigned char* buffer,
    unsigned long count
)
{
    SDL_RWops *src;

    src = (SDL_RWops *)stream->descriptor.pointer;
    SDL_RWseek(src, (int)offset, RW_SEEK_SET);
    if (count == 0) {
        return 0;
    }
    return SDL_RWread( src, buffer, 1, (int)count );
}

void
init_p_font(p_font *font, const char *font_path)
{
    memset(font->glyphs, 0, sizeof(font->glyphs));

    FT_Error error;
    FT_Open_Args args;
    FT_Stream stream;
    font->rw = SDL_RWFromFile(font_path, "rb");
    if (font->rw == NULL) {
        tms_fatalf("Unable to open font file: %s", SDL_GetError());
    }

    int position = SDL_RWtell(font->rw);
    if (position < 0) {
        tms_fatalf("Unable to seek in font stream.");
    }

    stream = (FT_Stream)malloc(sizeof(*stream));
    if (!stream) {
        tms_fatalf("Out of memory.");
    }

    stream->read = RWread;
    stream->descriptor.pointer = font->rw;
    stream->pos = (unsigned long)position;
    SDL_RWseek(font->rw, 0, RW_SEEK_END);
    stream->size = (unsigned long)(SDL_RWtell(font->rw) - position);
    SDL_RWseek(font->rw, position, RW_SEEK_SET);

    args.flags = FT_OPEN_STREAM;
    args.stream = stream;

    error = FT_Open_Face(gui_spritesheet::ft, &args, 0, &font->face);

    if (error) {
        tms_fatalf("Unable to open font: 0x%04X", error);
    }

    error = FT_Set_Char_Size(font->face, 0, font->orig_height * 64, 0, 0);
    if (error) {
        tms_fatalf("Unable to set font size: 0x%04X", error);
    }

    FT_Fixed scale = font->face->size->metrics.y_scale;
    font->ascent  = FT_CEIL(FT_MulFix(font->face->ascender, scale));
    font->descent = FT_CEIL(FT_MulFix(font->face->descender, scale));
    font->height = font->ascent - font->descent + 1;
    font->lineskip = FT_CEIL(FT_MulFix(font->face->height, scale));

    if (!p_font::glyph_indices) {
        p_font::glyph_indices = new FT_UInt[128];
        for (int i=CHAR_OFFSET; i<128; ++i) {
            p_font::glyph_indices[i] = FT_Get_Char_Index(font->face, i);
        }
    }
}

p_font::p_font(const char *font_path, int height)
    : orig_height(height)
{
    init_p_font(this, font_path);
}

p_font::p_font(struct tms_atlas *atlas, const char *font_path, int height)
    : orig_height(height)
{
    init_p_font(this, font_path);

    FT_Error error;

    //FT_Set_Pixel_Sizes(this->face, 0, this->height);
    FT_GlyphSlot slot = this->face->glyph;
    FT_Glyph_Metrics *metrics;
    FT_Glyph ft_glyph;
    FT_Bitmap ft_bitmap;

    struct glyph *cur_glyph = 0;

    int ft_bitmap_width = 0;
    int ft_bitmap_rows = 0;
    int ft_glyph_top = 0;
    int ft_glyph_left = 0;

    for (int i=CHAR_OFFSET; i<128; ++i) {
        cur_glyph = this->get_glyph(i);

        if (FT_Load_Glyph(face, glyph_indices[i], FT_LOAD_NO_BITMAP)) {
            tms_fatalf("Loading character %c failed.", i);
            continue;
        }

        /* Render outline */
        FT_Stroker stroker;
        FT_BitmapGlyph ft_bitmap_glyph;
        error = FT_Stroker_New(gui_spritesheet::ft, &stroker);

        if (error) {
            tms_fatalf("Error creating stroker");
            continue;
        }

        error = FT_Get_Glyph(this->face->glyph, &ft_glyph);
        if (error) {
            tms_fatalf("Error getting glyph");
            continue;
        }

        FT_Stroker_Set(stroker,
                       this->height * OUTLINE_MODIFIER,
                       FT_STROKER_LINECAP_ROUND,
                       FT_STROKER_LINEJOIN_ROUND,
                       0);

        error = FT_Glyph_Stroke(&ft_glyph, stroker, 1);
        if (error) {
            tms_fatalf("Error Applying stroke to glyph");
            continue;
        }

        error = FT_Glyph_To_Bitmap(&ft_glyph, FT_RENDER_MODE_NORMAL, 0, 1);
        if (error) {
            tms_fatalf("Error doing glyph to bitmap");
            continue;
        }

        slot            = this->face->glyph;
        metrics         = &slot->metrics;
        ft_bitmap_glyph = (FT_BitmapGlyph) ft_glyph;
        ft_bitmap       = ft_bitmap_glyph->bitmap;
        ft_bitmap_width = ft_bitmap.width;
        ft_bitmap_rows  = ft_bitmap.rows;

        FT_Stroker_Done(stroker);

        cur_glyph->outline = tms_atlas_add_bitmap(atlas,
                ft_bitmap_width,
                ft_bitmap_rows,
                TEXT_ATLAS_CHANNELS,
                ft_bitmap.buffer
                );

        cur_glyph->obw = ft_bitmap_width;
        cur_glyph->obh = ft_bitmap_rows;

        cur_glyph->m_outline_buf = (unsigned char*)malloc(ft_bitmap_width*ft_bitmap_rows);
        memcpy(cur_glyph->m_outline_buf, ft_bitmap.buffer, ft_bitmap_width*ft_bitmap_rows);

        /* Render bitmap */
        if (FT_Load_Glyph(face, glyph_indices[i], FT_LOAD_RENDER)) {
            tms_fatalf("Loading character %c failed.", i);
            continue;
        }

        slot            = this->face->glyph;
        metrics         = &slot->metrics;
        ft_bitmap       = slot->bitmap;
        ft_bitmap_width = slot->bitmap.width;
        ft_bitmap_rows  = slot->bitmap.rows;
        ft_glyph_top    = slot->bitmap_top;
        ft_glyph_left   = slot->bitmap_left;

        cur_glyph->ax = slot->advance.x >> 6;
        cur_glyph->ay = slot->advance.y >> 6;

        cur_glyph->bw = ft_bitmap_width;
        cur_glyph->bh = ft_bitmap_rows;

        cur_glyph->bl = ft_glyph_left;
        cur_glyph->bt = ft_glyph_top;

        if (FT_IS_SCALABLE(this->face)) {
            cur_glyph->minx = FT_FLOOR(metrics->horiBearingX);
            cur_glyph->maxx = cur_glyph->minx + FT_CEIL(metrics->width);
            cur_glyph->maxy = FT_FLOOR(metrics->horiBearingY);
            cur_glyph->miny = cur_glyph->maxy - FT_CEIL(metrics->height);
            cur_glyph->yoffset = this->ascent - cur_glyph->maxy;
            cur_glyph->advance = FT_CEIL(metrics->horiAdvance);
            cur_glyph->ax = cur_glyph->advance;
            //cur_glyph->ay = cur_glyph->miny;
            cur_glyph->index = glyph_indices[i];
            //tms_fatalf("is scalable");
        } else {
            tms_fatalf("is not scalable");
        }

        cur_glyph->m_sprite_buf = (unsigned char*)malloc(ft_bitmap_width*ft_bitmap_rows);
        memcpy(cur_glyph->m_sprite_buf, ft_bitmap.buffer, ft_bitmap_width*ft_bitmap_rows);

        cur_glyph->sprite = tms_atlas_add_bitmap(atlas,
                cur_glyph->bw,
                cur_glyph->bh,
                TEXT_ATLAS_CHANNELS,
                ft_bitmap.buffer
                );

        FT_Done_Glyph(ft_glyph);
    }
}

p_font::~p_font()
{
    if (this->rw) {
        this->rw->close(rw);
    }

    //FT_Done_Face(this->face);
}

static struct glyph* nl_glyph = 0;

struct glyph*
p_font::get_glyph(int c)
{
    if (c >= CHAR_OFFSET && c <= 128) {
        return &this->glyphs[c-CHAR_OFFSET];
    }

    switch (c) {
        case '\n':
            {
                if (!nl_glyph) {
                    nl_glyph = (struct glyph*)calloc(1, sizeof(struct glyph));
                    nl_glyph->newline = true;
                }

                return nl_glyph;
            }
            break;
    }

    return 0;

}

void
render_glyph(struct tms_ddraw *dd, struct glyph *g, float x, float y, tvec4 c, tvec4 oc, float scale/*=1.f*/, bool outline/*=false*/, bool call_opengl_stuff/*=true*/)
{
    if (call_opengl_stuff) {
        glEnable(GL_BLEND);
        glDisable(GL_DEPTH_TEST);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gui_spritesheet::atlas_text->texture.gl_texture);
    }

    if (outline) {
        tms_ddraw_set_sprite1c_color(dd, oc.r, oc.g, oc.b, oc.a);
        tms_ddraw_sprite1c(dd,
                g->outline,
                x,
                y,
                g->outline->width*scale,
                g->outline->height*scale
                );
    }

    tms_ddraw_set_sprite1c_color(dd, c.r, c.g, c.b, c.a);
    tms_ddraw_sprite1c(dd,
            g->sprite,
            x,
            y,
            g->sprite->width*scale,
            g->sprite->height*scale
            );
}
