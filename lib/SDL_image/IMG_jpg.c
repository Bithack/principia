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

/* This is a JPEG image file loading framework */

#include <SDL_image.h>

#include <stdio.h>
#include <setjmp.h>

#include <jpeglib.h>

/* Define this for fast loading and not as good image quality */
/*#define FAST_JPEG*/

/* See if an image is contained in a data source */
bool IMG_isJPG(SDL_IOStream *src)
{
    Sint64 start;
    bool is_JPG;
    bool in_scan;
    Uint8 magic[4];

    /* This detection code is by Steaphan Greene <stea@cs.binghamton.edu> */
    /* Blame me, not Sam, if this doesn't work right. */
    /* And don't forget to report the problem to the the sdl list too! */

    if (!src) {
        return false;
    }

    start = SDL_TellIO(src);
    is_JPG = false;
    in_scan = false;
    if (SDL_ReadIO(src, magic, 2) == 2) {
        if ( (magic[0] == 0xFF) && (magic[1] == 0xD8) ) {
            is_JPG = true;
            while (is_JPG) {
                if (SDL_ReadIO(src, magic, 2) != 2) {
                    is_JPG = false;
                } else if ( (magic[0] != 0xFF) && !in_scan ) {
                    is_JPG = false;
                } else if ( (magic[0] != 0xFF) || (magic[1] == 0xFF) ) {
                    /* Extra padding in JPEG (legal) */
                    /* or this is data and we are scanning */
                    SDL_SeekIO(src, -1, SDL_IO_SEEK_CUR);
                } else if (magic[1] == 0xD9) {
                    /* Got to end of good JPEG */
                    break;
                } else if ( in_scan && (magic[1] == 0x00) ) {
                    /* This is an encoded 0xFF within the data */
                } else if ( (magic[1] >= 0xD0) && (magic[1] < 0xD9) ) {
                    /* These have nothing else */
                } else if (SDL_ReadIO(src, magic+2, 2) != 2) {
                    is_JPG = false;
                } else {
                    /* Yes, it's big-endian */
                    Sint64 innerStart;
                    Uint32 size;
                    Sint64 end;
                    innerStart = SDL_TellIO(src);
                    size = (magic[2] << 8) + magic[3];
                    end = SDL_SeekIO(src, size-2, SDL_IO_SEEK_CUR);
                    if ( end != innerStart + size - 2 ) {
                        is_JPG = false;
                    }
                    if ( magic[1] == 0xDA ) {
                        /* Now comes the actual JPEG meat */
                        /* Ok, I'm convinced.  It is a JPEG. */
                        break;
                    }
                }
            }
        }
    }
    SDL_SeekIO(src, start, SDL_IO_SEEK_SET);
    return is_JPG;
}

#define INPUT_BUFFER_SIZE   4096
typedef struct {
    struct jpeg_source_mgr pub;

    SDL_IOStream *ctx;
    Uint8 buffer[INPUT_BUFFER_SIZE];
} my_source_mgr;

/*
 * Initialize source --- called by jpeg_read_header
 * before any data is actually read.
 */
static void init_source (j_decompress_ptr cinfo)
{
    /* We don't actually need to do anything */
    return;
}

/*
 * Fill the input buffer --- called whenever buffer is emptied.
 */
static boolean fill_input_buffer (j_decompress_ptr cinfo)
{
    my_source_mgr * src = (my_source_mgr *) cinfo->src;
    size_t nbytes;

    nbytes = SDL_ReadIO(src->ctx, src->buffer, INPUT_BUFFER_SIZE);
    if (nbytes == 0) {
        /* Insert a fake EOI marker */
        src->buffer[0] = (Uint8) 0xFF;
        src->buffer[1] = (Uint8) JPEG_EOI;
        nbytes = 2;
    }
    src->pub.next_input_byte = src->buffer;
    src->pub.bytes_in_buffer = nbytes;

    return TRUE;
}


/*
 * Skip data --- used to skip over a potentially large amount of
 * uninteresting data (such as an APPn marker).
 *
 * Writers of suspendable-input applications must note that skip_input_data
 * is not granted the right to give a suspension return.  If the skip extends
 * beyond the data currently in the buffer, the buffer can be marked empty so
 * that the next read will cause a fill_input_buffer call that can suspend.
 * Arranging for additional bytes to be discarded before reloading the input
 * buffer is the application writer's problem.
 */
static void skip_input_data (j_decompress_ptr cinfo, long num_bytes)
{
    my_source_mgr * src = (my_source_mgr *) cinfo->src;

    /* Just a dumb implementation for now.  Could use fseek() except
     * it doesn't work on pipes.  Not clear that being smart is worth
     * any trouble anyway --- large skips are infrequent.
     */
    if (num_bytes > 0) {
        while (num_bytes > (long) src->pub.bytes_in_buffer) {
            num_bytes -= (long) src->pub.bytes_in_buffer;
            (void) src->pub.fill_input_buffer(cinfo);
            /* note we assume that fill_input_buffer will never
             * return FALSE, so suspension need not be handled.
             */
        }
        src->pub.next_input_byte += (size_t) num_bytes;
        src->pub.bytes_in_buffer -= (size_t) num_bytes;
    }
}

/*
 * Terminate source --- called by jpeg_finish_decompress
 * after all data has been read.
 */
static void term_source (j_decompress_ptr cinfo)
{
    /* We don't actually need to do anything */
    return;
}

/*
 * Prepare for input from a stdio stream.
 * The caller must have already opened the stream, and is responsible
 * for closing it after finishing decompression.
 */
static void jpeg_SDL_IO_src (j_decompress_ptr cinfo, SDL_IOStream *ctx)
{
  my_source_mgr *src;

  /* The source object and input buffer are made permanent so that a series
   * of JPEG images can be read from the same file by calling jpeg_stdio_src
   * only before the first one.  (If we discarded the buffer at the end of
   * one image, we'd likely lose the start of the next one.)
   * This makes it unsafe to use this manager and a different source
   * manager serially with the same JPEG object.  Caveat programmer.
   */
  if (cinfo->src == NULL) { /* first time for this JPEG object? */
    cinfo->src = (struct jpeg_source_mgr *)
      (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
                  sizeof(my_source_mgr));
    src = (my_source_mgr *) cinfo->src;
  }

  src = (my_source_mgr *) cinfo->src;
  src->pub.init_source = init_source;
  src->pub.fill_input_buffer = fill_input_buffer;
  src->pub.skip_input_data = skip_input_data;
  src->pub.resync_to_restart = jpeg_resync_to_restart; /* use default method */
  src->pub.term_source = term_source;
  src->ctx = ctx;
  src->pub.bytes_in_buffer = 0; /* forces fill_input_buffer on first read */
  src->pub.next_input_byte = NULL; /* until buffer loaded */
}

struct my_error_mgr {
    struct jpeg_error_mgr errmgr;
    jmp_buf escape;
};

static void my_error_exit(j_common_ptr cinfo)
{
    struct my_error_mgr *err = (struct my_error_mgr *)cinfo->err;
    longjmp(err->escape, 1);
}

static void output_no_message(j_common_ptr cinfo)
{
    /* do nothing */
}

struct loadjpeg_vars {
    const char *error;
    SDL_Surface *surface;
    struct jpeg_decompress_struct cinfo;
    struct my_error_mgr jerr;
};

/* Load a JPEG type image from an SDL datasource */
static bool LIBJPEG_LoadJPG_IO(SDL_IOStream *src, struct loadjpeg_vars *vars)
{
    JSAMPROW rowptr[1];

    /* Create a decompression structure and load the JPEG header */
    vars->cinfo.err = jpeg_std_error(&vars->jerr.errmgr);
    vars->jerr.errmgr.error_exit = my_error_exit;
    vars->jerr.errmgr.output_message = output_no_message;
    if (setjmp(vars->jerr.escape)) {
        /* If we get here, libjpeg found an error */
        jpeg_destroy_decompress(&vars->cinfo);
        vars->error = "JPEG loading error";
        return false;
    }

    jpeg_create_decompress(&vars->cinfo);
    jpeg_SDL_IO_src(&vars->cinfo, src);
    jpeg_read_header(&vars->cinfo, TRUE);

    if (vars->cinfo.num_components == 4) {
        /* Set 32-bit Raw output */
        vars->cinfo.out_color_space = JCS_CMYK;
        vars->cinfo.quantize_colors = FALSE;
        jpeg_calc_output_dimensions(&vars->cinfo);

        /* Allocate an output surface to hold the image */
        vars->surface = SDL_CreateSurface(vars->cinfo.output_width, vars->cinfo.output_height, SDL_PIXELFORMAT_RGBA32);
    } else {
        /* Set 24-bit RGB output */
        vars->cinfo.out_color_space = JCS_RGB;
        vars->cinfo.quantize_colors = FALSE;
#ifdef FAST_JPEG
        vars->cinfo.scale_num   = 1;
        vars->cinfo.scale_denom = 1;
        vars->cinfo.dct_method = JDCT_FASTEST;
        vars->cinfo.do_fancy_upsampling = FALSE;
#endif
        jpeg_calc_output_dimensions(&vars->cinfo);

        /* Allocate an output surface to hold the image */
        vars->surface = SDL_CreateSurface(vars->cinfo.output_width, vars->cinfo.output_height, SDL_PIXELFORMAT_RGB24);
    }

    if (!vars->surface) {
        jpeg_destroy_decompress(&vars->cinfo);
        return false;
    }

    /* Decompress the image */
    jpeg_start_decompress(&vars->cinfo);
    while (vars->cinfo.output_scanline < vars->cinfo.output_height) {
        rowptr[0] = (JSAMPROW)(Uint8 *)vars->surface->pixels +
                            vars->cinfo.output_scanline * vars->surface->pitch;
        jpeg_read_scanlines(&vars->cinfo, rowptr, (JDIMENSION) 1);
    }
    jpeg_finish_decompress(&vars->cinfo);
    jpeg_destroy_decompress(&vars->cinfo);

    if (vars->cinfo.num_components == 4) {
        // The CMYK image is essentially RGBA composed over black
        SDL_Surface *output = SDL_CreateSurface(vars->cinfo.output_width, vars->cinfo.output_height, SDL_PIXELFORMAT_RGB24);
        if (!output) {
            return false;
        }

        SDL_BlitSurface(vars->surface, NULL, output, NULL);
        SDL_DestroySurface(vars->surface);
        vars->surface = output;
    }
    return true;
}

SDL_Surface *IMG_LoadJPG_IO(SDL_IOStream *src)
{
    Sint64 start;
    struct loadjpeg_vars vars;

    if (!src) {
        /* The error message has been set in SDL_IOFromFile */
        return NULL;
    }

    start = SDL_TellIO(src);
    SDL_zero(vars);

    if (LIBJPEG_LoadJPG_IO(src, &vars)) {
        return vars.surface;
    }

    /* this may clobber a set error if seek fails: don't care. */
    SDL_SeekIO(src, start, SDL_IO_SEEK_SET);
    if (vars.surface) {
        SDL_DestroySurface(vars.surface);
    }
    if (vars.error) {
        SDL_SetError("%s", vars.error);
    }

    return NULL;
}
