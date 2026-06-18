/*
  SDL_image:  An example image loading library for use with SDL
  Copyright (C) 1997-2026 Sam Lantinga <slouken@libsdl.org>

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

/* This is a PNG image file loading framework */

#include <SDL_image.h>
#include <png.h>

static void png_read_data(png_structp png_ptr, png_bytep area, png_size_t size)
{
    SDL_IOStream *src = (SDL_IOStream *)png_get_io_ptr(png_ptr);
    if (SDL_ReadIO(src, area, size) != size) {
        png_error(png_ptr, "Failed to read all expected data from SDL_IOStream for PNG image.");
    }
}

static void png_write_data(png_structp png_ptr, png_bytep src, png_size_t size)
{
    SDL_IOStream *dst = (SDL_IOStream *)png_get_io_ptr(png_ptr);
    if (SDL_WriteIO(dst, src, size) != size) {
        png_error(png_ptr, "Failed to write all expected data to SDL_IOStream for PNG image.");
    }
}

static void png_flush_data(png_structp png_ptr)
{
    png_write_flush(png_ptr);
}

struct png_load_vars
{
    const char *error;
    SDL_Surface *surface;
    png_structp png_ptr;
    png_infop info_ptr;
    png_bytep *row_pointers;
    png_colorp color_ptr;
    SDL_Surface *source_surface_for_save;

    png_uint_32 width, height;
    int bit_depth, color_type, interlace_type;
    Uint32 format;
    unsigned char header[8];
    png_colorp png_palette;
    int num_palette;
    png_bytep trans;
    int num_trans;
    png_color_16p trans_values;
};

static bool LIBPNG_LoadPNG_IO_Internal(SDL_IOStream *src, struct png_load_vars *vars)
{
    if (SDL_ReadIO(src, vars->header, sizeof(vars->header)) != sizeof(vars->header)) {
        vars->error = "Failed to read PNG header from SDL_IOStream";
        return false;
    }
    if (png_sig_cmp(vars->header, 0, 8)) {
        vars->error = "Not a valid PNG file signature";
        return false;
    }

    vars->png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (vars->png_ptr == NULL) {
        vars->error = "Couldn't allocate memory for PNG read struct";
        return false;
    }
    vars->info_ptr = png_create_info_struct(vars->png_ptr);
    if (vars->info_ptr == NULL) {
        vars->error = "Couldn't create image information for PNG file";
        return false;
    }

    if (setjmp(*png_set_longjmp_fn(vars->png_ptr, longjmp, sizeof(jmp_buf)))) {
        vars->error = "Error during PNG read operation";
        return false;
    }

    png_set_read_fn(vars->png_ptr, src, png_read_data);
    png_set_sig_bytes(vars->png_ptr, 8);

    png_read_info(vars->png_ptr, vars->info_ptr);
    png_get_IHDR(vars->png_ptr, vars->info_ptr, &vars->width, &vars->height, &vars->bit_depth,
                     &vars->color_type, &vars->interlace_type, NULL, NULL);

    // Only convert non-palette formats to RGB/RGBA
    // TODO: Convert this to a colour key - the specs say that there's
    // only one transparent colour for non-palette images
    if (vars->color_type != PNG_COLOR_TYPE_PALETTE) {
        if (png_get_valid(vars->png_ptr, vars->info_ptr, PNG_INFO_tRNS)) {
            png_set_tRNS_to_alpha(vars->png_ptr);
            vars->color_type |= PNG_COLOR_MASK_ALPHA;
        }
    }

    /* SDL doesn't currently support this format, so we convert to RGB for now. */
    if (vars->color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
        png_set_gray_to_rgb(vars->png_ptr);
    }

    if (vars->color_type == PNG_COLOR_TYPE_PALETTE ||
        vars->color_type == PNG_COLOR_TYPE_GRAY) {
        if (vars->bit_depth == 1) {
            vars->format = SDL_PIXELFORMAT_INDEX1MSB;
        } else if (vars->bit_depth == 2) {
            vars->format = SDL_PIXELFORMAT_INDEX2MSB;
        } else if (vars->bit_depth == 4) {
            vars->format = SDL_PIXELFORMAT_INDEX4MSB;
        } else /* if (vars->bit_depth == 8) */ {
            vars->format = SDL_PIXELFORMAT_INDEX8;
        }
    } else if (vars->bit_depth == 16) {
        if (vars->color_type & PNG_COLOR_MASK_ALPHA) {
            vars->format = SDL_PIXELFORMAT_RGBA64;
        } else {
            vars->format = SDL_PIXELFORMAT_RGB48;
        }
    } else {
        if (vars->color_type & PNG_COLOR_MASK_ALPHA) {
            vars->format = SDL_PIXELFORMAT_RGBA32;
        } else {
            vars->format = SDL_PIXELFORMAT_RGB24;
        }
    }

    png_read_update_info(vars->png_ptr, vars->info_ptr);
    png_get_IHDR(vars->png_ptr, vars->info_ptr, &vars->width, &vars->height, &vars->bit_depth,
                     &vars->color_type, &vars->interlace_type, NULL, NULL);

    vars->surface = SDL_CreateSurface(vars->width, vars->height, vars->format);
    if (vars->surface == NULL) {
        vars->error = SDL_GetError();
        return false;
    }

    // For palette images, set up the palette
    if (vars->color_type == PNG_COLOR_TYPE_PALETTE) {
        if (png_get_PLTE(vars->png_ptr, vars->info_ptr, &vars->png_palette, &vars->num_palette)) {
            SDL_Palette *palette = SDL_CreateSurfacePalette(vars->surface);
            if (!palette) {
                vars->error = "Failed to create palette for PNG image";
                return false;
            }

            for (int i = 0; i < palette->ncolors; i++) {
                palette->colors[i].r = vars->png_palette[i].red;
                palette->colors[i].g = vars->png_palette[i].green;
                palette->colors[i].b = vars->png_palette[i].blue;
                palette->colors[i].a = 255;
            }

            if (png_get_tRNS(vars->png_ptr, vars->info_ptr, &vars->trans, &vars->num_trans, &vars->trans_values)) {
                for (int i = 0; i < vars->num_trans && i < palette->ncolors; i++) {
                    palette->colors[i].a = vars->trans[i];
                }
                SDL_SetSurfaceBlendMode(vars->surface, SDL_BLENDMODE_BLEND);
            }
        }
    } else if (vars->color_type == PNG_COLOR_TYPE_GRAY) {
        SDL_Palette *palette = SDL_CreateSurfacePalette(vars->surface);
        if (!palette) {
            vars->error = "Failed to create palette for PNG image";
            return false;
        }

        for (int i = 0; i < palette->ncolors; i++) {
            palette->colors[i].r = (i * 255) / palette->ncolors;
            palette->colors[i].g = (i * 255) / palette->ncolors;
            palette->colors[i].b = (i * 255) / palette->ncolors;
            palette->colors[i].a = 255;
        }
    }

    vars->row_pointers = (png_bytep *)SDL_malloc(sizeof(png_bytep) * vars->height);
    if (!vars->row_pointers) {
        vars->error = "Out of memory allocating row pointers";
        return false;
    }
    for (png_uint_32 y = 0; y < vars->height; y++) {
        vars->row_pointers[y] = (png_bytep)((Uint8 *)vars->surface->pixels + y * (size_t)vars->surface->pitch);
    }

    png_read_image(vars->png_ptr, vars->row_pointers);

#if SDL_BYTEORDER != SDL_BIG_ENDIAN
    if (vars->format == SDL_PIXELFORMAT_RGBA64) {
        Uint16 *pixels = (Uint16 *)vars->surface->pixels;
        int num_pixels = vars->width * vars->height * 4;
        for (int i = 0; i < num_pixels; i++) {
            pixels[i] = SDL_Swap16(pixels[i]);
        }
    }
#endif

    return true;
}

/* Load a PNG type image from an SDL datasource */
SDL_Surface *IMG_LoadPNG_IO(SDL_IOStream *src)
{
    Sint64 start_pos;
    bool success = false;

    if (!src) {
        SDL_SetError("SDL_IOStream is NULL");
        return NULL;
    }

    start_pos = SDL_TellIO(src);

    struct png_load_vars vars;
    SDL_zero(vars);

    success = LIBPNG_LoadPNG_IO_Internal(src, &vars);

    if (vars.png_ptr) {
        png_destroy_read_struct(&vars.png_ptr,
                                    vars.info_ptr ? &vars.info_ptr : (png_infopp)NULL,
                                    (png_infopp)NULL);
    }
    if (vars.row_pointers) {
        SDL_free(vars.row_pointers);
    }

    if (success) {
        return vars.surface;
    } else {
        SDL_SeekIO(src, start_pos, SDL_IO_SEEK_SET);
        if (vars.surface) {
            SDL_DestroySurface(vars.surface);
        }
        if (vars.error) {
            SDL_SetError("%s", vars.error);
        }
        return NULL;
    }
}

// Save PNGs

struct png_save_vars
{
    const char *error;
    SDL_Surface *surface;
    png_structp png_ptr;
    png_infop info_ptr;
    png_bytep *row_pointers;
    png_colorp color_ptr;
    SDL_Surface *source_surface_for_save;

    Uint8 transparent_table[256];
    SDL_Palette *palette;
    int png_color_type;
    int bit_depth; // default to 8
};

static bool LIBPNG_SavePNG_IO_Internal(struct png_save_vars *vars, SDL_Surface *surface, SDL_IOStream *dst)
{
    vars->source_surface_for_save = surface;

    vars->png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (vars->png_ptr == NULL) {
        vars->error = "Couldn't allocate memory for PNG write struct";
        return false;
    }
    vars->info_ptr = png_create_info_struct(vars->png_ptr);
    if (vars->info_ptr == NULL) {
        vars->error = "Couldn't create image information for PNG file";
        return false;
    }

    if (setjmp(*png_set_longjmp_fn(vars->png_ptr, longjmp, sizeof(jmp_buf)))) {
        vars->error = "Error during PNG write operation";
        return false;
    }

    png_set_write_fn(vars->png_ptr, dst, png_write_data, png_flush_data);

    vars->palette = SDL_GetSurfacePalette(surface);
    if (vars->palette) {
        const int ncolors = vars->palette->ncolors;
        int i;
        int last_transparent = -1;

        vars->color_ptr = (png_colorp)SDL_malloc(sizeof(png_color) * ncolors);
        if (vars->color_ptr == NULL) {
            vars->error = "Couldn't allocate palette for PNG file";
            return false;
        }
        for (i = 0; i < ncolors; i++) {
            vars->color_ptr[i].red = vars->palette->colors[i].r;
            vars->color_ptr[i].green = vars->palette->colors[i].g;
            vars->color_ptr[i].blue = vars->palette->colors[i].b;
            if (vars->palette->colors[i].a != 255) {
                last_transparent = i;
            }
        }
        png_set_PLTE(vars->png_ptr, vars->info_ptr, vars->color_ptr, ncolors);
        vars->png_color_type = PNG_COLOR_TYPE_PALETTE;

        if (last_transparent >= 0) {
            for (i = 0; i <= last_transparent; ++i) {
                vars->transparent_table[i] = vars->palette->colors[i].a;
            }
            png_set_tRNS(vars->png_ptr, vars->info_ptr, vars->transparent_table, last_transparent + 1, NULL);
        }
    } else if (surface->format == SDL_PIXELFORMAT_RGB24) {
        vars->png_color_type = PNG_COLOR_TYPE_RGB;
    } else if (!SDL_ISPIXELFORMAT_ALPHA(surface->format)) {
        vars->png_color_type = PNG_COLOR_TYPE_RGB;
        vars->source_surface_for_save = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGB24);
        if (!vars->source_surface_for_save) {
            vars->error = SDL_GetError();
            return false;
        }
    } else {
        vars->png_color_type = PNG_COLOR_TYPE_RGBA;
        vars->source_surface_for_save = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
        if (!vars->source_surface_for_save) {
            vars->error = SDL_GetError();
            return false;
        }
    }

    png_set_IHDR(vars->png_ptr, vars->info_ptr, vars->source_surface_for_save->w, vars->source_surface_for_save->h,
                     vars->bit_depth, vars->png_color_type, PNG_INTERLACE_NONE,
                     PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    png_write_info(vars->png_ptr, vars->info_ptr);

    vars->row_pointers = (png_bytep *)SDL_malloc(sizeof(png_bytep) * vars->source_surface_for_save->h);
    if (!vars->row_pointers) {
        vars->error = "Out of memory allocating row pointers";
        return false;
    }
    for (int row = 0; row < (int)vars->source_surface_for_save->h; row++) {
        vars->row_pointers[row] = (png_bytep)((Uint8 *)vars->source_surface_for_save->pixels + row * (size_t)vars->source_surface_for_save->pitch);
    }

    png_write_image(vars->png_ptr, vars->row_pointers);
    png_write_end(vars->png_ptr, vars->info_ptr);

    return true;
}

bool IMG_SavePNG_IO(SDL_Surface *surface, SDL_IOStream *dst, bool closeio)
{
    if (!surface) {
        return SDL_InvalidParamError("surface");
    }

    if (SDL_ISPIXELFORMAT_INDEXED(surface->format) && !SDL_GetSurfacePalette(surface)) {
        return SDL_SetError("Indexed surfaces must have a palette");
    }

    if (!surface || !dst) {
        SDL_SetError("Surface or SDL_IOStream is NULL");
        return false;
    }

    struct png_save_vars vars;
    bool result = false;

    SDL_zero(vars);
    vars.bit_depth = 8;

    result = LIBPNG_SavePNG_IO_Internal(&vars, surface, dst);

    if (vars.png_ptr) {
        png_destroy_write_struct(&vars.png_ptr, &vars.info_ptr);
    }
    if (vars.color_ptr) {
        SDL_free(vars.color_ptr);
    }
    if (vars.row_pointers) {
        SDL_free(vars.row_pointers);
    }
    if (vars.source_surface_for_save && vars.source_surface_for_save != surface) {
        SDL_DestroySurface(vars.source_surface_for_save);
    }

    if (!result && vars.error) {
        SDL_SetError("%s", vars.error);
    }

    if (closeio) {
        result &= SDL_CloseIO(dst);
    }

    return result;
}

bool IMG_SavePNG(SDL_Surface *surface, const char *file)
{
    SDL_IOStream *dst = SDL_IOFromFile(file, "wb");
    if (dst) {
        return IMG_SavePNG_IO(surface, dst, true);
    } else {
        return false;
    }
}
