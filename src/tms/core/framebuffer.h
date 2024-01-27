#ifndef _TMS_FB__H_
#define _TMS_FB__H_

/** @relates tms_fb @{ */

#include <tms/backend/opengl.h>

struct tms_program;

#define TMS_FB_MAX_TEXTURES 4

/**
 * Used to render content off-screen.
 * When tms_fb_bind() is called, the currently bound framebuffer will be
 * remembered until unbind. Framebuffers will thus act like a stack, making it
 * safe to wrap screens in framebuffers even if the screen itself renders to its own
 * custom framebuffers.
 **/
struct tms_fb {
    GLuint fb_o[2];
    GLuint fb_texture[2][TMS_FB_MAX_TEXTURES];
    GLuint fb_depth[2];

    int double_buffering;
    int toggle;
    int num_textures;

    unsigned width;
    unsigned height;
    unsigned depth;
    int format;

    struct tms_fb *previous; /**< previous framebuffer that was bound before this one.
                              *  Will be rebound when this framebuffer is unbounded. */
};

extern struct tms_program *_tms_fb_copy_program;

struct tms_fb *tms_fb_alloc(unsigned width, unsigned height, int double_buffering);
void tms_fb_free(struct tms_fb *fb);
void tms_fb_init(struct tms_fb* fb);
void tms_fb_enable_depth_and_stencil(struct tms_fb *fb);
void tms_fb_enable_depth(struct tms_fb *fb, int format);
void tms_fb_enable_depth_texture(struct tms_fb *fb, int format);
void tms_fb_add_texture(struct tms_fb *fb, int format, int wrap_s, int wrap_t, int filter_min, int filter_mag);
int tms_fb_bind(struct tms_fb *f);
int tms_fb_unbind(struct tms_fb *f);
int tms_fb_bind_current_textures(struct tms_fb *f, int first_unit);
int tms_fb_bind_last_textures(struct tms_fb *f, int first_unit);
void tms_fb_swap(struct tms_fb *f, struct tms_program *p);
void tms_fb_swap_blur5x5(struct tms_fb *f);
void tms_fb_swap_blur3x3(struct tms_fb *f);
void tms_fb_render(struct tms_fb *f, struct tms_program *p);
void tms_fb_render_to(struct tms_fb *f, struct tms_fb *dest, struct tms_program *p);
void tms_fb_attach_depth(struct tms_fb *fb, int attachment, int name);

#endif

