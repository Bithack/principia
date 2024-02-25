#ifndef _TMS_TEXTURE__H_
#define _TMS_TEXTURE__H_

/** @relates tms_texture @{ */

#include <tms/backend/opengl.h>

#define TMS_FILTER_LINEAR GL_LINEAR
#define TMS_FILTER_NEAREST GL_NEAREST

#define TMS_MIPMAP -1

#define GL_ETC1_RGB8_OES 0x8D64

struct tms_context;

struct tms_texture {
    char *filename;
    unsigned char *data;

    GLuint gl_texture;
    GLenum filter;
    GLenum wrap;

    int width;
    int height;
    int buf_size;

    int format;
    int colors;

    int gamma_corrected:1;
    int gamma_correction:1;
    int is_buffered:1;
    int is_uploaded:1;
    int num_channels;

    void (*buffer_fn)(struct tms_texture *tex);
};

static inline void tms_texture_set_buffer_fn(struct tms_texture *tex, void(*fn)(struct tms_texture *tex)){ tex->buffer_fn = fn; };
struct tms_texture* tms_texture_alloc(void);
void tms_texture_free(struct tms_texture *tex);
unsigned char* tms_texture_alloc_buffer(struct tms_texture *tex, int width, int height, int num_channels);
unsigned char* tms_texture_get_buffer(struct tms_texture *tex);
int tms_texture_clear_buffer(struct tms_texture *tex, unsigned char clear_value);
int tms_texture_free_buffer(struct tms_texture *tex);
int tms_texture_load(struct tms_texture *tex, const char *filename);
int tms_texture_load_etc1(struct tms_texture *tex, const char *filename);
int tms_texture_load_mem(struct tms_texture *tex, const char *buf, int width, int height, int num_channels);
int tms_texture_load_mem2(struct tms_texture *tex, const char *buf, size_t size, int freesrc);
int tms_texture_upload(struct tms_texture *tex);
int tms_texture_download(struct tms_texture *tex);
int tms_texture_bind(struct tms_texture *tex);
int tms_texture_flip_y(struct tms_texture *tex);
int tms_texture_flip_x(struct tms_texture *tex);
int tms_texture_add_alpha(struct tms_texture *tex, float a);
void tms_texture_init(struct tms_texture *t);
void tms_texture_set_filtering(struct tms_texture *tex, int filter);
void tms_texture_render(struct tms_texture *t);

static inline int tms_texture_get_width(struct tms_texture *tex)
{
    return tex->width;
}

static inline int tms_texture_get_height(struct tms_texture *tex)
{
    return tex->height;
}

static inline int tms_texture_get_num_channels(struct tms_texture *tex)
{
    return tex->num_channels;
}

#endif
