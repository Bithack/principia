#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "backend.h"
#include "texture.h"
#include "framebuffer.h"
#include "tms.h"

#include "SDL_image.h"

struct tms_texture*
tms_texture_alloc(void)
{
    struct tms_texture *tex;

    tex = calloc(1, sizeof(struct tms_texture));
    if (tex)
        tms_texture_init(tex);

    return tex;
}

void
tms_texture_init(struct tms_texture *t)
{
    t->gl_texture = 0;
    t->filename = 0;
    t->data = 0;
    t->format = 0;
    t->colors = 0;
    t->wrap = GL_REPEAT;
    t->gamma_correction = 0;
    t->is_uploaded = 0;
    t->is_buffered = 0;
    t->filter = GL_LINEAR;
    t->buffer_fn = 0;
}

void
tms_texture_render(struct tms_texture *t)
{
    struct tms_fb fb;
    fb.num_textures = 1;
    fb.toggle = 0;
    fb.fb_texture[0][0] = t->gl_texture;

    tms_fb_render(&fb, _tms_fb_copy_program);
}

/**
 * Allocate a local buffer for this texture.
 * This function will return a pointer to a buffer that you can read and write to.
 * The buffer will be managed by the texture and should never be manually freed by
 * you.
 *
 * @relates tms_texture
 **/
unsigned char *tms_texture_alloc_buffer(struct tms_texture *tex, int width,
                                        int height, int num_channels)
{
    tms_assertf(num_channels > 0 && num_channels <= 4, "invalid number of color channels");
    tms_assertf(width > 0 && width <= 8192, "invalid texture width");
    tms_assertf(height > 0 && height <= 8192, "invalid texture height");

    (tex->data = realloc(tex->data, width*height*num_channels)) || tms_fatalf("out of mem (t_ab) (%d,%d,%d)", width, height, num_channels);
    tex->width = width;
    tex->height = height;
    tex->num_channels = num_channels;
    tex->is_buffered = 1;
    tex->gamma_corrected = 0;

    return tex->data;
}

/**
 *
 * @relates tms_texture
 **/
unsigned char* tms_texture_get_buffer(struct tms_texture *tex)
{
    return tex->data;
}

/**
 * Clear this textures buffer to the given clear_value. `clear_value` will be
 * copied into each colo component of the texture.
 *
 * @relates tms_texture
 **/
int tms_texture_clear_buffer(struct tms_texture *tex, unsigned char clear_value)
{
    if (!tex->data)
        return T_NO_DATA;

    memset(tex->data, (int)clear_value, tex->width * tex->height * tex->num_channels);
    tex->gamma_corrected = 0;

    return T_OK;
}

void
tms_texture_free(struct tms_texture *tex)
{
    tms_texture_free_buffer(tex);

    if (tex->is_uploaded) {
        glDeleteTextures(1, &tex->gl_texture);
    }

    if (tex->filename) {
        free(tex->filename);
    }

    free(tex);
}

int tms_texture_free_buffer(struct tms_texture *tex)
{
    if (tex->data)
        free(tex->data);
    tex->data = 0;
    tex->is_buffered = 0;

    return T_OK;
}

/**
 * Read texture from the given file.
 * @relates tms_texture
 **/
int
tms_texture_load(struct tms_texture *tex, const char *filename)
{
    int status;

    SDL_RWops *rw = SDL_RWFromFile(filename,"rb");

    if (!rw) {
        tms_infof("file not found: '%s'", SDL_GetError());
        return T_ERR;
    }

    SDL_Surface *s = IMG_Load_RW(rw, 1);

    if (!s) {
        tms_errorf("could not open file: %s", filename);
        return T_ERR;
    }

    tex->is_buffered = 1;
    tex->filename = strdup(filename);
    tex->gamma_corrected = 0;
    tex->width = s->w;
    tex->height = s->h;
    //tex->num_channels = 3 + s->format->Amask?1:0;
    tex->num_channels = s->format->BytesPerPixel;

    //tms_infof("bpp %d", s->format->BytesPerPixel);

    //tms_assertf(tex->num_channels == s->format->BytesPerPixel, "unsupported texture type BLAH");

    tex->data = malloc(tex->width*tex->height*tex->num_channels);

    for (int y=0; y<s->h; y++) {
        for (int x=0; x<s->w*tex->num_channels; x++) {
            int o = y*s->pitch;
            ((unsigned char*)tex->data)[(s->h-y-1)*s->w*tex->num_channels+x] =
                ((unsigned char*)s->pixels)[o+x];
        }
    }

    SDL_FreeSurface(s);
    //SDL_RWclose(rw);

    return T_OK;
}

/**
 * Load a texture from a memory buffer.
 * This will allocate a new buffer and copy the input buffer to it.
 * Expects one byte per pixel color channel. If `alpha_channel` is
 * 1, 4 bytes per pixel is expected, otherwise 3 bytes per pixel.
 *
 * An alternative to using this function is to instead call tms_texture_alloc_buffer()
 * and write directly to that buffer.
 *
 * @param alpha_channel 1 if the input buffer's format is RGBA
 *
 * @relates tms_texture
 **/
int
tms_texture_load_mem(struct tms_texture *tex, const char *buf,
                     int width, int height, int num_channels)
{
    tex->width = width;
    tex->height = height;
    tex->num_channels = num_channels;

    if (tex->data)
        free(tex->data);

    (tex->data = malloc(num_channels*width*height))
        || tms_fatalf("tms_texture_load_mem: out of mem (t_lm)");

    memcpy(tex->data, buf, num_channels*width*height);
    tex->is_buffered = 1;
    tex->gamma_corrected = 0;

    return T_OK;
}

/**
 * Loads a texture from a memory buffer
 * @relates tms_texture
 **/
int
tms_texture_load_mem2(struct tms_texture *tex, const char *buf, size_t size, int freesrc)
{
    int status;

    SDL_RWops *rw = SDL_RWFromConstMem(buf, size);

    if (!rw) {
        tms_errorf("Error creating RW from memory: %s", SDL_GetError());
        return T_COULD_NOT_OPEN;
    }

    SDL_Surface *s = IMG_Load_RW(rw, freesrc);

    if (!s) {
        tms_errorf("Error calling IMG_Load_RW: %s", IMG_GetError());
        return T_COULD_NOT_OPEN;
    }

    tex->is_buffered = 1;
    tex->filename = "mem";
    tex->gamma_corrected = 0;
    tex->width = s->w;
    tex->height = s->h;
    //tex->num_channels = 3 + s->format->Amask?1:0;
    tex->num_channels = s->format->BytesPerPixel;

    //tms_infof("bpp %d", s->format->BytesPerPixel);

    //tms_assertf(tex->num_channels == s->format->BytesPerPixel, "unsupported texture type BLAH");

    tex->data = malloc(tex->width*tex->height*tex->num_channels);

    for (int y=0; y<s->h; y++) {
        for (int x=0; x<s->w*tex->num_channels; x++) {
            int o = y*s->pitch;
            ((unsigned char*)tex->data)[(s->h-y-1)*s->w*tex->num_channels+x] =
                ((unsigned char*)s->pixels)[o+x];
        }
    }

    SDL_FreeSurface(s);
    //SDL_RWclose(rw);

    return T_OK;
}

static int
inv_gamma_correction(struct tms_texture *tex)
{
    int x,y,c;
    double v;
    uint8_t *p;

    p = tex->data;

    for (y=0; y<tex->height; y++) {
        for (x=0; x<tex->width; x++) {

            for (c=0; c<tex->num_channels; c++) {
                if (c != 3) {
                    v = (double)(*p) / 256.;
                    v = pow(v, tms.gamma);

                    v *= 256.;
                    v = round(v);
                    if (v < 0.) v = 0.;
                    else if (v > 256.) v = 256.;

                    /* converting a double to an uint8 just feels so wrong... */
                    *p = (uint8_t)v;
                }

                p++;
            }
        }
    }

    tex->gamma_corrected = 1;

    return T_OK;
}

int
tms_texture_flip_x(struct tms_texture *tex)
{
    if (!tex->is_buffered)
        return T_NO_DATA;

    int oy = tex->num_channels * tex->width;
    int ox = tex->num_channels;

    for (int y=0; y<tex->height; y++) {
        for (int x=0; x<tex->width/2; x++) {
            for (int z=0; z<tex->num_channels; z++) {
                unsigned char tmp = tex->data[y*oy+(tex->width-x-1)*ox+z];
                tex->data[y*oy+(tex->width-x-1)*ox+z] = tex->data[y*oy+x*ox+z];
                tex->data[y*oy+x*ox+z] = tmp;
            }
        }
    }

    return T_OK;
}

int
tms_texture_flip_y(struct tms_texture *tex)
{
    if (!tex->is_buffered)
        return T_NO_DATA;

    int oy = tex->num_channels * tex->width;
    int ox = tex->num_channels;

    for (int y=0; y<tex->height/2; y++) {
        for (int x=0; x<tex->width; x++) {
            for (int z=0; z<tex->num_channels; z++) {
                unsigned char tmp = tex->data[(tex->height-y-1)*oy+x*ox+z];
                tex->data[(tex->height-y-1)*oy+x*ox+z] = tex->data[y*oy+x*ox+z];
                tex->data[y*oy+x*ox+z] = tmp;
            }
        }
    }

    return T_OK;
}

int
tms_texture_add_alpha(struct tms_texture *tex, float a)
{
    tms_assertf(tex->num_channels == 3, "we can only add alpha if the texture has three channels!");

    if (!tex->is_buffered) {
        return T_NO_DATA;
    }

    tex->num_channels = 4;

    uint32_t new_size = tex->width*tex->height*tex->num_channels;
    unsigned char *new_data = calloc(1, new_size);

    uint32_t i, j;
    for (i=0,j=0; j<new_size;++j) {
        if ((j % 4) == 3) {
            new_data[j] = a*255.f;
        } else {
            new_data[j] = tex->data[i++];
        }
    }

    free(tex->data);
    tex->data = new_data;

    return T_OK;
}

/**
 * Upload the texture to the GPU.
 *
 * @relates tms_texture
 **/
int
tms_texture_upload(struct tms_texture *tex)
{
    if (!tex->is_buffered)
        return T_NO_DATA;

    tex->is_uploaded = 1;

    if (tex->gl_texture == 0) {
        glGenTextures(1, &tex->gl_texture);
    }

    int format = -1;
    int colors = GL_RGB;

    switch (tex->num_channels) {
        case 1:
            colors = GL_LUMINANCE;
            format = GL_LUMINANCE;
            break;

        case 3:
            colors = GL_RGB;
            format = GL_RGB;

#ifndef TMS_USE_GLES
            if (tex->gamma_correction) {
                format = GL_SRGB;
            }
#endif
            break;

        case 4:
            colors = GL_RGBA;
            format = GL_RGBA;

#ifndef TMS_USE_GLES
            if (tex->gamma_correction) {
                format = GL_SRGB8_ALPHA8;
            }
#endif
            break;

        default:
            tms_fatalf("Invalid numbers of channels specified: %d",
                    tex->num_channels);
            break;
    }

    if (0 && tex->gamma_correction && !tex->gamma_corrected)
        inv_gamma_correction(tex);

    glBindTexture(GL_TEXTURE_2D, tex->gl_texture);

#ifndef TMS_USE_GLES
    glEnable(GL_TEXTURE_2D);
    if (tex->filter == TMS_MIPMAP) {
        glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
    }
#endif

    glTexImage2D(GL_TEXTURE_2D, 0, format,
            tex->width, tex->height,
            0, colors, GL_UNSIGNED_BYTE, tex->data);

#ifdef DEBUG_TEXTURES
    tms_debugf("tex upload: %d: (%dx%d), f:%d, c:%d (%s)", glGetError(), tex->width, tex->height, tex->format, colors, tex->filename?tex->filename:"");
#endif

    if (tex->filter == TMS_MIPMAP) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#ifdef TMS_USE_GLES
        glGenerateMipmap(GL_TEXTURE_2D);

        int err = glGetError();
        if (err != 0) {
            tms_infof("error: could not create mipmaps (%d)", err);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        }
#endif
    } else {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, tex->filter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, tex->filter);
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, tex->wrap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, tex->wrap);

    return T_OK;
}

/**
 * Set the texture filtering
 *
 * @relates tms_texture
 **/
void
tms_texture_set_filtering(struct tms_texture *tex, int filter)
{
    if (tex->is_uploaded) {
        tms_texture_bind(tex);

        if (filter == TMS_MIPMAP) {
            glEnable(GL_TEXTURE_2D);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
#ifdef TMS_USE_GLEW
            if (GLEW_VERSION_3_0) { /* XXX */
                glGenerateMipmap(GL_TEXTURE_2D);
            } else {
                glGenerateMipmapEXT(GL_TEXTURE_2D);
            }
#else
            glGenerateMipmap(GL_TEXTURE_2D);
#endif
        } else {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
        }
    }

    tex->filter = filter;
}

/**
 * @relates tms_texture
 **/
int
tms_texture_download(struct tms_texture *tex)
{
    /* XXX */
    /*
    glBindTexture(GL_TEXTURE_2D, tex->gl_texture);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_UNSIGNED_BYTE, tex->data);
    tms_infof("err: %d", glGetError());
    */
    return T_OK;
}

/**
 * Bind the texture to the currently active opengl
 * texture unit.
 * Use glActiveTexture() to choose texture unit.
 *
 * @relates tms_texture
 **/
int
tms_texture_bind(struct tms_texture *tex)
{
    if (!tex->is_uploaded && !tex->is_buffered) {
        /* lazy load */
        if (tex->buffer_fn){
#ifdef DEBUG_TEXTURES
            tms_debugf("lazy-loading(%p): %p", tex, tex->buffer_fn);
#endif
            tex->buffer_fn(tex);
#ifdef DEBUG_TEXTURES
            tms_debugf("lazy-loading(%p): filename %s", tex, tex->filename?tex->filename:"???");
#endif
        } else return 1;

        if (!tex->is_uploaded) {
            tms_errorf("texture lazy-load did not upload anything! :(");
            return 1;
        }
    }

    glBindTexture(GL_TEXTURE_2D, tex->gl_texture);
    return T_OK;
}

