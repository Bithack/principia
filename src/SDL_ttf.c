/*
  SDL_ttf:  A companion library to SDL for working with TrueType (tm) fonts
  Copyright (C) 2001-2012 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#endif

#ifdef HAVE_ALLOCA
#define ALLOCA(n) ((void*)alloca(n))
#define FREEA(p)
#else
#define ALLOCA(n) malloc(n)
#define FREEA(p) free(p)
#endif

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H
#include FT_STROKER_H
#include FT_GLYPH_H
#include FT_TRUETYPE_IDS_H

#include "SDL.h"
#include "SDL_endian.h"
#include "SDL_ttf.h"

/* FIXME: Right now we assume the gray-scale renderer Freetype is using
   supports 256 shades of gray, but we should instead key off of num_grays
   in the result FT_Bitmap after the FT_Render_Glyph() call. */
#define NUM_GRAYS       256

/* Handy routines for converting from fixed point */
#define FT_FLOOR(X) ((X & -64) / 64)
#define FT_CEIL(X)  (((X + 63) & -64) / 64)

#define CACHED_METRICS  0x10
#define CACHED_BITMAP   0x01
#define CACHED_PIXMAP   0x02

/* Cached glyph information */
typedef struct cached_glyph {
    int stored;
    FT_UInt index;
    FT_Bitmap bitmap;
    FT_Bitmap pixmap;
    int minx;
    int maxx;
    int miny;
    int maxy;
    int yoffset;
    int advance;
    Uint16 cached;
} c_glyph;

/* The structure used to hold internal font information */
struct _TTF_Font {
    /* Freetype2 maintains all sorts of useful info itself */
    FT_Face face;

    /* We'll cache these ourselves */
    int height;
    int ascent;
    int descent;
    int lineskip;

    /* The font style */
    int face_style;
    int style;
    int outline;

    /* Whether kerning is desired */
    int kerning;

    /* Extra width in glyph bounds for text styles */
    int glyph_overhang;
    float glyph_italics;

    /* Information in the font for underlining */
    int underline_offset;
    int underline_height;

    /* Cache for style-transformed glyphs */
    c_glyph *current;
    c_glyph cache[257]; /* 257 is a prime */

    /* We are responsible for closing the font stream */
    SDL_RWops *src;
    int freesrc;
    FT_Open_Args args;

    /* For non-scalable formats, we must remember which font index size */
    int font_size_family;

    /* really just flags passed into FT_Load_Glyph */
    int hinting;
};

/* The FreeType font engine/library */
static FT_Library library;
static int TTF_initialized = 0;
static int TTF_byteswapped = 0;

int TTF_Init(void)
{
    int status = 0;

    if (! TTF_initialized) {
        FT_Error error = FT_Init_FreeType(&library);
        if (error) {
            TTF_SetError("Couldn't init FreeType engine");
            status = -1;
        }
    }
    if (status == 0) {
        ++TTF_initialized;
    }
    return status;
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
    return SDL_RWread(src, buffer, 1, (int)count);
}

TTF_Font* TTF_OpenFont(const char *file, int ptsize)
{
    SDL_RWops *src = SDL_RWFromFile(file, "rb");
    if (src == NULL) {
        TTF_SetError("%s", SDL_GetError());
        return NULL;
    }

    TTF_Font* font;
    FT_Error error;
    FT_Face face;
    FT_Fixed scale;
    FT_Stream stream;
    FT_CharMap found;
    int position, i;

    if (! TTF_initialized) {
        TTF_SetError("Library not initialized");
        return NULL;
    }

    /* Check to make sure we can seek in this stream */
    position = SDL_RWtell(src);
    if (position < 0) {
        TTF_SetError("Can't seek in stream");
        return NULL;
    }

    font = (TTF_Font*) malloc(sizeof *font);
    if (font == NULL) {
        TTF_SetError("Out of memory");
        return NULL;
    }
    memset(font, 0, sizeof(*font));

    font->src = src;
    font->freesrc = 1;

    stream = (FT_Stream)malloc(sizeof(*stream));
    if (stream == NULL) {
        TTF_SetError("Out of memory");
        TTF_CloseFont(font);
        return NULL;
    }
    memset(stream, 0, sizeof(*stream));

    stream->read = RWread;
    stream->descriptor.pointer = src;
    stream->pos = (unsigned long)position;
    SDL_RWseek(src, 0, RW_SEEK_END);
    stream->size = (unsigned long)(SDL_RWtell(src) - position);
    SDL_RWseek(src, position, RW_SEEK_SET);

    font->args.flags = FT_OPEN_STREAM;
    font->args.stream = stream;

    error = FT_Open_Face(library, &font->args, 0, &font->face);
    if (error) {
        TTF_SetError("Couldn't load font file");
        TTF_CloseFont(font);
        return NULL;
    }
    face = font->face;

    /* Set charmap for loaded font */
    found = 0;
    for (i = 0; i < face->num_charmaps; i++) {
        FT_CharMap charmap = face->charmaps[i];
        if ((charmap->platform_id == 3 && charmap->encoding_id == 1) /* Windows Unicode */
         || (charmap->platform_id == 3 && charmap->encoding_id == 0) /* Windows Symbol */
         || (charmap->platform_id == 2 && charmap->encoding_id == 1) /* ISO Unicode */
         || (charmap->platform_id == 0)) { /* Apple Unicode */
            found = charmap;
            break;
        }
    }
    if (found) {
        /* If this fails, continue using the default charmap */
        FT_Set_Charmap(face, found);
    }

    if (!FT_IS_SCALABLE(face))
        return NULL; // non-scalable not supported (TTF is scalable)

    /* Set the character size and use default DPI (72) */
    error = FT_Set_Char_Size(font->face, 0, ptsize * 64, 0, 0);
        if (error) {
        TTF_SetError("Couldn't set font size");
        TTF_CloseFont(font);
        return NULL;
    }

    /* Get the scalable font metrics for this font */
    scale = face->size->metrics.y_scale;
    font->ascent  = FT_CEIL(FT_MulFix(face->ascender, scale));
    font->descent = FT_CEIL(FT_MulFix(face->descender, scale));
    font->height  = font->ascent - font->descent + /* baseline */ 1;
    font->lineskip = FT_CEIL(FT_MulFix(face->height, scale));
    font->underline_offset = FT_FLOOR(FT_MulFix(face->underline_position, scale));
    font->underline_height = FT_FLOOR(FT_MulFix(face->underline_thickness, scale));

    if (font->underline_height < 1) {
        font->underline_height = 1;
    }

    /* Initialize the font face style */
    font->face_style = 0x00;

    /* Set the default font style */
    font->style = font->face_style;
    font->outline = 0;
    font->kerning = 1;
    font->glyph_overhang = face->size->metrics.y_ppem / 10;
    /* x offset = cos(((90.0-12)/360)*2*M_PI), or 12 degree angle */
    font->glyph_italics = 0.207f;
    font->glyph_italics *= font->height;

    return font;
}

static void Flush_Glyph(c_glyph* glyph)
{
    glyph->stored = 0;
    glyph->index = 0;
    if (glyph->bitmap.buffer) {
        free(glyph->bitmap.buffer);
        glyph->bitmap.buffer = 0;
    }
    if (glyph->pixmap.buffer) {
        free(glyph->pixmap.buffer);
        glyph->pixmap.buffer = 0;
    }
    glyph->cached = 0;
}

static void Flush_Cache(TTF_Font* font)
{
    int i;
    int size = sizeof(font->cache) / sizeof(font->cache[0]);

    for (i = 0; i < size; ++i) {
        if (font->cache[i].cached) {
            Flush_Glyph(&font->cache[i]);
        }
    }
}

static FT_Error Load_Glyph(TTF_Font* font, Uint16 ch, c_glyph* cached, int want)
{
    FT_Face face;
    FT_Error error;
    FT_GlyphSlot glyph;
    FT_Glyph_Metrics* metrics;
    FT_Outline* outline;

    if (!font || !font->face) {
        return FT_Err_Invalid_Handle;
    }

    face = font->face;

    /* Load the glyph */
    if (! cached->index) {
        cached->index = FT_Get_Char_Index(face, ch);
    }
    error = FT_Load_Glyph(face, cached->index, FT_LOAD_DEFAULT | font->hinting);
    if (error) {
        return error;
    }

    /* Get our glyph shortcuts */
    glyph = face->glyph;
    metrics = &glyph->metrics;
    outline = &glyph->outline;

    /* Get the glyph metrics if desired */
    if ((want & CACHED_METRICS) && !(cached->stored & CACHED_METRICS)) {
        /* Get the bounding box */
        cached->minx = FT_FLOOR(metrics->horiBearingX);
        cached->maxx = cached->minx + FT_CEIL(metrics->width);
        cached->maxy = FT_FLOOR(metrics->horiBearingY);
        cached->miny = cached->maxy - FT_CEIL(metrics->height);
        cached->yoffset = font->ascent - cached->maxy;
        cached->advance = FT_CEIL(metrics->horiAdvance);

        cached->stored |= CACHED_METRICS;
    }

    if (((want & CACHED_BITMAP) && !(cached->stored & CACHED_BITMAP)) ||
         ((want & CACHED_PIXMAP) && !(cached->stored & CACHED_PIXMAP))) {
        int i;
        FT_Bitmap* src;
        FT_Bitmap* dst;
        FT_Glyph bitmap_glyph = NULL;

        /* Render the glyph */
        error = FT_Render_Glyph(glyph, FT_RENDER_MODE_NORMAL);
        if (error) {
            return error;
        }
        src = &glyph->bitmap;

        /* Copy over information to cache */
        dst = &cached->pixmap;
        memcpy(dst, src, sizeof(*dst));

        if (dst->rows != 0) {
            dst->buffer = (unsigned char *)malloc(dst->pitch * dst->rows);
            if (!dst->buffer) {
                return FT_Err_Out_Of_Memory;
            }
            memset(dst->buffer, 0, dst->pitch * dst->rows);

            for (i = 0; i < src->rows; i++) {
                int soffset = i * src->pitch;
                int doffset = i * dst->pitch;

                memcpy(dst->buffer+doffset,
                        src->buffer+soffset, src->pitch);
            }
        }

        /* Mark that we rendered this format */
        cached->stored |= CACHED_PIXMAP;

        /* Free outlined glyph */
        if (bitmap_glyph) {
            FT_Done_Glyph(bitmap_glyph);
        }
    }

    /* We're done, mark this glyph cached */
    cached->cached = ch;

    return 0;
}

static FT_Error Find_Glyph(TTF_Font* font, Uint16 ch, int want)
{
    int retval = 0;
    int hsize = sizeof(font->cache) / sizeof(font->cache[0]);

    int h = ch % hsize;
    font->current = &font->cache[h];

    if (font->current->cached != ch)
        Flush_Glyph(font->current);

    if ((font->current->stored & want) != want) {
        retval = Load_Glyph(font, ch, font->current, want);
    }
    return retval;
}

void TTF_CloseFont(TTF_Font* font)
{
    if (font) {
        Flush_Cache(font);
        if (font->face) {
            FT_Done_Face(font->face);
        }
        if (font->args.stream) {
            free(font->args.stream);
        }
        if (font->freesrc) {
            SDL_RWclose(font->src);
        }
        free(font);
    }
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverflow"

static Uint16 *UTF8_to_UNICODE(Uint16 *unicode, const char *utf8, int len)
{
    int i, j;
    Uint16 ch;

    for (i=0, j=0; i < len; ++i, ++j) {
        ch = ((const unsigned char *)utf8)[i];

        if (ch >= 0xF0) {
            ch  =  (Uint16)(utf8[i]&0x07) << 18;
            ch |=  (Uint16)(utf8[++i]&0x3F) << 12;
            ch |=  (Uint16)(utf8[++i]&0x3F) << 6;
            ch |=  (Uint16)(utf8[++i]&0x3F);
        } else if (ch >= 0xE0) {
            ch  =  (Uint16)(utf8[i]&0x0F) << 12;
            ch |=  (Uint16)(utf8[++i]&0x3F) << 6;
            ch |=  (Uint16)(utf8[++i]&0x3F);
        } else if (ch >= 0xC0) {
            ch  =  (Uint16)(utf8[i]&0x1F) << 6;
            ch |=  (Uint16)(utf8[++i]&0x3F);
        }

        unicode[j] = ch;
    }
    unicode[j] = 0;

    return unicode;
}

#pragma GCC diagnostic pop

int TTF_FontLineSkip(const TTF_Font *font)
{
    return(font->lineskip);
}

int TTF_SizeUTF8(TTF_Font *font, const char *text, int *w, int *h)
{
    Uint16 *unicode_text;
    int unicode_len;
    int status;

    /* Copy the UTF-8 text to a UNICODE text buffer */
    unicode_len = strlen(text);
    unicode_text = (Uint16 *)ALLOCA((1+unicode_len+1)*(sizeof *unicode_text));
    if (unicode_text == NULL) {
        TTF_SetError("Out of memory");
        return -1;
    }
    *unicode_text = UNICODE_BOM_NATIVE;
    UTF8_to_UNICODE(unicode_text+1, text, unicode_len);

    /* Render the new text */
    status = TTF_SizeUNICODE(font, unicode_text, w, h);

    /* Free the text buffer and return */
    FREEA(unicode_text);
    return status;
}

int TTF_SizeUNICODE(TTF_Font *font, const Uint16 *text, int *w, int *h)
{
    int status;
    const Uint16 *ch;
    int swapped;
    int x, z;
    int minx, maxx;
    int miny, maxy;
    c_glyph *glyph;
    FT_Error error;
    FT_Long use_kerning;
    FT_UInt prev_index = 0;
    int outline_delta = 0;

    /* Initialize everything to 0 */
    if (! TTF_initialized) {
        TTF_SetError("Library not initialized");
        return -1;
    }
    status = 0;
    minx = maxx = 0;
    miny = maxy = 0;
    swapped = TTF_byteswapped;

    /* check kerning */
    use_kerning = FT_HAS_KERNING(font->face) && font->kerning;

    /* Init outline handling */
    if (font->outline  > 0) {
        outline_delta = font->outline * 2;
    }

    /* Load each character and sum it's bounding box */
    x= 0;
    for (ch=text; *ch; ++ch) {
        Uint16 c = *ch;
        if (c == UNICODE_BOM_NATIVE) {
            swapped = 0;
            if (text == ch) {
                ++text;
            }
            continue;
        }
        if (c == UNICODE_BOM_SWAPPED) {
            swapped = 1;
            if (text == ch) {
                ++text;
            }
            continue;
        }
        if (swapped) {
            c = SDL_Swap16(c);
        }

        error = Find_Glyph(font, c, CACHED_METRICS);
        if (error) {
            TTF_SetError("Couldn't find glyph");
            return -1;
        }
        glyph = font->current;

        /* handle kerning */
        if (use_kerning && prev_index && glyph->index) {
            FT_Vector delta;
            FT_Get_Kerning(font->face, prev_index, glyph->index, ft_kerning_default, &delta);
            x += delta.x >> 6;
        }

        z = x + glyph->minx;
        if (minx > z) {
            minx = z;
        }

        if (glyph->advance > glyph->maxx) {
            z = x + glyph->advance;
        } else {
            z = x + glyph->maxx;
        }
        if (maxx < z) {
            maxx = z;
        }
        x += glyph->advance;

        if (glyph->miny < miny) {
            miny = glyph->miny;
        }
        if (glyph->maxy > maxy) {
            maxy = glyph->maxy;
        }
        prev_index = glyph->index;
    }

    /* Fill the bounds rectangle */
    if (w) {
        /* Add outline extra width */
        *w = (maxx - minx) + outline_delta;
    }
    if (h) {
        /* Some fonts descend below font height (FletcherGothicFLF) */
        /* Add outline extra height */
        *h = (font->ascent - miny) + outline_delta;
        if (*h < font->height) {
            *h = font->height;
        }

    }
    return status;
}

/* Convert the UTF-8 text to UNICODE and render it
*/
SDL_Surface *TTF_RenderUTF8_Shaded(TTF_Font *font,
                const char *text, SDL_Color fg, SDL_Color bg)
{
    SDL_Surface *textbuf;
    Uint16 *unicode_text;
    int unicode_len;

    /* Copy the UTF-8 text to a UNICODE text buffer */
    unicode_len = strlen(text);
    unicode_text = (Uint16 *)ALLOCA((1+unicode_len+1)*(sizeof *unicode_text));
    if (unicode_text == NULL) {
        TTF_SetError("Out of memory");
        return(NULL);
    }
    *unicode_text = UNICODE_BOM_NATIVE;
    UTF8_to_UNICODE(unicode_text+1, text, unicode_len);

    /* Render the new text */
    textbuf = TTF_RenderUNICODE_Shaded(font, unicode_text, fg, bg);

    /* Free the text buffer and return */
    FREEA(unicode_text);
    return(textbuf);
}

SDL_Surface* TTF_RenderUNICODE_Shaded(TTF_Font* font,
                       const Uint16* text,
                       SDL_Color fg,
                       SDL_Color bg)
{
    int xstart;
    int width;
    int height;
    SDL_Surface* textbuf;
    SDL_Palette* palette;
    int index;
    int rdiff;
    int gdiff;
    int bdiff;
    const Uint16* ch;
    Uint8* src;
    Uint8* dst;
    Uint8* dst_check;
    int swapped;
    int row, col;
    FT_Bitmap* current;
    c_glyph *glyph;
    FT_Error error;
    FT_Long use_kerning;
    FT_UInt prev_index = 0;

    /* Get the dimensions of the text surface */
    if ((TTF_SizeUNICODE(font, text, &width, &height) < 0) || !width) {
        TTF_SetError("Text has zero width");
        return NULL;
    }

    /* Create the target surface */
    textbuf = SDL_CreateRGBSurface(SDL_SWSURFACE, width, height, 8, 0, 0, 0, 0);
    if (textbuf == NULL) {
        return NULL;
    }

    /* Adding bound checking to avoid all kinds of memory corruption errors
       that may occur. */
    dst_check = (Uint8*)textbuf->pixels + textbuf->pitch * textbuf->h;

    /* Fill the palette with NUM_GRAYS levels of shading from bg to fg */
    palette = textbuf->format->palette;
    rdiff = fg.r - bg.r;
    gdiff = fg.g - bg.g;
    bdiff = fg.b - bg.b;

    for (index = 0; index < NUM_GRAYS; ++index) {
        palette->colors[index].r = bg.r + (index*rdiff) / (NUM_GRAYS-1);
        palette->colors[index].g = bg.g + (index*gdiff) / (NUM_GRAYS-1);
        palette->colors[index].b = bg.b + (index*bdiff) / (NUM_GRAYS-1);
    }

    /* check kerning */
    use_kerning = FT_HAS_KERNING(font->face) && font->kerning;

    /* Load and render each character */
    xstart = 0;
    swapped = TTF_byteswapped;
    for (ch = text; *ch; ++ch) {
        Uint16 c = *ch;
        if (c == UNICODE_BOM_NATIVE) {
            swapped = 0;
            if (text == ch) {
                ++text;
            }
            continue;
        }
        if (c == UNICODE_BOM_SWAPPED) {
            swapped = 1;
            if (text == ch) {
                ++text;
            }
            continue;
        }
        if (swapped) {
            c = SDL_Swap16(c);
        }

        error = Find_Glyph(font, c, CACHED_METRICS|CACHED_PIXMAP);
        if (error) {
            TTF_SetError("Couldn't find glyph");
            SDL_FreeSurface(textbuf);
            return NULL;
        }
        glyph = font->current;
        /* Ensure the width of the pixmap is correct. On some cases,
         * freetype may report a larger pixmap than possible.*/
        width = glyph->pixmap.width;
        if (font->outline <= 0 && width > glyph->maxx - glyph->minx) {
            width = glyph->maxx - glyph->minx;
        }
        /* do kerning, if possible AC-Patch */
        if (use_kerning && prev_index && glyph->index) {
            FT_Vector delta;
            FT_Get_Kerning(font->face, prev_index, glyph->index, ft_kerning_default, &delta);
            xstart += delta.x >> 6;
        }
        /* Compensate for the wrap around with negative minx's */
        if ((ch == text) && (glyph->minx < 0)) {
            xstart -= glyph->minx;
        }

        current = &glyph->pixmap;
        for (row = 0; row < current->rows; ++row) {
            /* Make sure we don't go either over, or under the
             * limit */
            if (row+glyph->yoffset < 0) {
                continue;
            }
            if (row+glyph->yoffset >= textbuf->h) {
                continue;
            }
            dst = (Uint8*) textbuf->pixels +
                (row+glyph->yoffset) * textbuf->pitch +
                xstart + glyph->minx;
            src = current->buffer + row * current->pitch;
            for (col=width; col>0 && dst < dst_check; --col) {
                *dst++ |= *src++;
            }
        }

        xstart += glyph->advance;

        prev_index = glyph->index;
    }

    return textbuf;
}

void TTF_Quit(void)
{
    if (TTF_initialized) {
        if (--TTF_initialized == 0) {
            FT_Done_FreeType(library);
        }
    }
}
