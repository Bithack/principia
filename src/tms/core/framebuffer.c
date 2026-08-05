#include "tms.h"
#include "framebuffer.h"
#include "gbuffer.h"
#include "varray.h"
#include "mesh.h"
#include "glob.h"

#include <stdlib.h>
#include <assert.h>

static float verts[] = {
    1.f, 1.f,
    1.f, 1.f,
    -1.f, 1.f,
    0.f, 1.f,
    -1.f, -1.f,
    0.f, 0.f,
    1.f, -1.f,
    1.f, 0.f,
};

static struct tms_program *blur5x5v_512_program;
static struct tms_program *blur5x5h_512_program;
static struct tms_program *blur5x5v_256_program;
static struct tms_program *blur5x5h_256_program;

static struct tms_program *blur3x3v_program;
static struct tms_program *blur3x3h_program;

static struct tms_program *blur3x3_program;
static struct tms_program *blur3x3_128_program;

static struct tms_program *copy_program;
struct tms_program *_tms_fb_copy_program;
static struct tms_mesh *quad;
static int _i = 0;

static void init() {
    struct tms_varray *va = tms_varray_alloc(2);
    struct tms_gbuffer *v = tms_gbuffer_alloc_fill(verts, sizeof(verts));
    tms_gbuffer_upload(v);
    tms_varray_map_attribute(va, "position", 2, GL_FLOAT, v);
    tms_varray_map_attribute(va, "texcoord", 2, GL_FLOAT, v);
    quad = tms_mesh_alloc(va, 0);
    quad->primitive_type = TMS_TRIANGLE_FAN;
    tms_mesh_set_autofree_buffers(quad, 1);

    struct tms_shader *sh;
    sh = tms_shader_read("blur5x5h_256");
    blur5x5h_256_program = tms_shader_get_program(sh, TMS_NO_PIPELINE);

    sh = tms_shader_read("blur5x5v_256");
    blur5x5v_256_program = tms_shader_get_program(sh, TMS_NO_PIPELINE);

    sh = tms_shader_read("blur5x5h_512");
    blur5x5h_512_program = tms_shader_get_program(sh, TMS_NO_PIPELINE);

    sh = tms_shader_read("blur5x5v_512");
    blur5x5v_512_program = tms_shader_get_program(sh, TMS_NO_PIPELINE);

    sh = tms_shader_read("blur3x3");
    blur3x3_program = tms_shader_get_program(sh, TMS_NO_PIPELINE);

    sh = tms_shader_read("blur3x3_128");
    blur3x3_128_program = tms_shader_get_program(sh, TMS_NO_PIPELINE);

    sh = tms_shader_read("blur3x3h");
    blur3x3h_program = tms_shader_get_program(sh, TMS_NO_PIPELINE);

    sh = tms_shader_read("blur3x3v");
    blur3x3v_program = tms_shader_get_program(sh, TMS_NO_PIPELINE);

    sh = tms_shader_read("copy");
    _tms_fb_copy_program = (copy_program = tms_shader_get_program(sh, TMS_NO_PIPELINE));

    _i = 1;
}

struct tms_fb*
tms_fb_alloc(unsigned width, unsigned height,
             int double_buffering)
{
    struct tms_fb *fb = calloc(1, sizeof(struct tms_fb));

    if (!_i) init();

    fb->width = width;
    fb->height = height;
    fb->double_buffering = double_buffering;

    tms_fb_init(fb);

    return fb;
}

void
tms_fb_init(struct tms_fb* fb)
{
    int ierr;
    fb->toggle = 0;
    if (glad_glGenFramebuffers)
        glGenFramebuffers(fb->double_buffering ? 2 : 1, fb->fb_o);
    else
        glGenFramebuffersEXT(fb->double_buffering ? 2 : 1, fb->fb_o);
}

void
tms_fb_free(struct tms_fb *fb)
{
    for (int x=0; x<(fb->double_buffering ? 2 : 1); x++) {
        if (fb->fb_depth[x])
            glDeleteRenderbuffers(1, &fb->fb_depth[x]);

        for (int y=0; y<fb->num_textures; y++)
            glDeleteTextures(1, &fb->fb_texture[x][y]);
    }

    glDeleteFramebuffers(fb->double_buffering ? 2 : 1, fb->fb_o);

    free(fb);
}

void
tms_fb_enable_depth(struct tms_fb *fb, int format)
{
    int ierr;
    tms_assertf((ierr = glGetError()) == 0, "gl error %d in tms_fb_enable_depth (begin)", ierr);

    for (int x=0; x<(fb->double_buffering ? 2 : 1); x++) {
        if (glad_glBindFramebuffer)
            glBindFramebuffer(GL_FRAMEBUFFER, fb->fb_o[x]);
        else
            glBindFramebufferEXT(GL_FRAMEBUFFER, fb->fb_o[x]);

        if (glad_glGenRenderbuffers) {
            glGenRenderbuffers(1, &fb->fb_depth[x]);
            glBindRenderbuffer(GL_RENDERBUFFER, fb->fb_depth[x]);
        } else {
            glGenRenderbuffersEXT(1, &fb->fb_depth[x]);
            glBindRenderbufferEXT(GL_RENDERBUFFER, fb->fb_depth[x]);
        }

        if (glad_glRenderbufferStorage)
            glRenderbufferStorage(GL_RENDERBUFFER, format, fb->width, fb->height);
        else
            glRenderbufferStorageEXT(GL_RENDERBUFFER, format, fb->width, fb->height);

        if (glad_glFramebufferRenderbuffer)
            glad_glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, fb->fb_depth[x]);
        else
            glFramebufferRenderbufferEXT(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, fb->fb_depth[x]);

        if (glad_glBindRenderbuffer)
            glBindRenderbuffer(GL_RENDERBUFFER, 0);
        else
            glBindRenderbufferEXT(GL_RENDERBUFFER, 0);

    }

    if (glad_glBindFramebuffer)
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    else
        glBindFramebufferEXT(GL_FRAMEBUFFER, 0);

    tms_assertf((ierr = glGetError()) == 0, "gl error %d in tms_fb_enable_depth 7 (end)", ierr);
}

void
tms_fb_enable_depth_texture(struct tms_fb *fb, int format)
{
    for (int x=0; x<(fb->double_buffering ? 2 : 1); x++) {
        if (glad_glBindFramebuffer)
            glad_glBindFramebuffer(GL_FRAMEBUFFER, fb->fb_o[x]);
        else
            glad_glBindFramebufferEXT(GL_FRAMEBUFFER, fb->fb_o[x]);

        glGenTextures(1, &fb->fb_depth[x]);
        glBindTexture(GL_TEXTURE_2D, fb->fb_depth[x]);

        format = GL_DEPTH_COMPONENT;

        glTexImage2D(GL_TEXTURE_2D, 0, format, fb->width, fb->height, 0, format, GL_UNSIGNED_INT, 0);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, fb->fb_depth[x], 0);

        glBindTexture(GL_TEXTURE_2D, 0);
    }

    if (glad_glBindFramebuffer)
        glad_glBindFramebuffer(GL_FRAMEBUFFER, 0);
    else
        glad_glBindFramebufferEXT(GL_FRAMEBUFFER, 0);
}

void
tms_fb_add_texture(struct tms_fb *fb, int format,
                   int wrap_s, int wrap_t,
                   int filter_min, int filter_mag)
{
    if (fb->num_textures >= TMS_FB_MAX_TEXTURES)
        return;

    int ierr;
    tms_assertf((ierr = glGetError()) == 0, "gl error %d in tms_fb_add_texture (begin)", ierr);

    for (int x=0; x<(fb->double_buffering ? 2 : 1); x++) {

        if (glad_glBindFramebuffer)
            glBindFramebuffer(GL_FRAMEBUFFER, fb->fb_o[x]);
        else
            glBindFramebufferEXT(GL_FRAMEBUFFER, fb->fb_o[x]);

        glGenTextures(1, &fb->fb_texture[x][fb->num_textures]);

        glBindTexture(GL_TEXTURE_2D, fb->fb_texture[x][fb->num_textures]);

        // Note: GL_R32F, GL_RGB16F and GL_RGB32F aren't supported in OpenGL ES 2.0
        if (format == GL_R32F)
            glTexImage2D(GL_TEXTURE_2D, 0, format, fb->width, fb->height, 0, GL_RED, GL_FLOAT, 0);
        else if (format == GL_RGB16F)
            glTexImage2D(GL_TEXTURE_2D, 0, format, fb->width, fb->height, 0, GL_RGB, GL_FLOAT, 0);
        else if (format == GL_RGB32F)
            glTexImage2D(GL_TEXTURE_2D, 0, format, fb->width, fb->height, 0, GL_RGB, GL_FLOAT, 0);
        else if (format == GL_ALPHA)
            glTexImage2D(GL_TEXTURE_2D, 0, format, fb->width, fb->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
        else if (format == GL_RGBA4)
            glTexImage2D(GL_TEXTURE_2D, 0, format, fb->width, fb->height, 0, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4, 0);
        else if (format == GL_RGB565)
            glTexImage2D(GL_TEXTURE_2D, 0, format, fb->width, fb->height, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, 0);
        else
            glTexImage2D(GL_TEXTURE_2D, 0, format, fb->width, fb->height, 0, format, GL_UNSIGNED_BYTE, 0);

        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap_s);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap_t);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter_min);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter_mag);

        if (glad_glFramebufferTexture2D)
            glad_glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0+fb->num_textures, GL_TEXTURE_2D, fb->fb_texture[x][fb->num_textures], 0);
        else
            glad_glFramebufferTexture2DEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0+fb->num_textures, GL_TEXTURE_2D, fb->fb_texture[x][fb->num_textures], 0);

        glBindTexture(GL_TEXTURE_2D, 0);

		/* update draw buffers */
        GLenum bufs[fb->num_textures+1];

		for (int y = 0; y < fb->num_textures + 1; y++) {
			bufs[y] = GL_COLOR_ATTACHMENT0 + y;
		}

        if (!tms.use_gles) {
            glDrawBuffers(fb->num_textures+1, bufs);
            tms_assertf((ierr = glGetError()) == 0, "gl error %d in tms_fb_add_texture %d 15", ierr, x);
        }
    }

    fb->num_textures ++;

    if (glad_glBindFramebuffer)
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    else
        glBindFramebufferEXT(GL_FRAMEBUFFER, 0);

    tms_assertf((ierr = glGetError()) == 0, "gl error %d in tms_fb_add_texture 16 (end)", ierr);
}

void
tms_fb_swap_blur5x5(struct tms_fb *f)
{
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    struct tms_program *prg;

    prg = f->width == 256 ? blur5x5h_256_program : blur5x5h_512_program;

    tms_program_bind(prg);
    tms_fb_swap(f, prg);

    prg = f->width == 256 ? blur5x5v_256_program : blur5x5v_512_program;

    tms_program_bind(prg);
    tms_fb_swap(f, prg);

    glEnable(GL_DEPTH_TEST);
}

void
tms_fb_swap_blur3x3(struct tms_fb *f)
{
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);

    struct tms_program *prg = (f->width == 256 ? blur3x3_program : blur3x3_128_program);

    tms_program_bind(prg);
    tms_fb_swap(f, prg);

    glEnable(GL_DEPTH_TEST);
}


/**
 * Swap the buffers
 **/
void
tms_fb_swap(struct tms_fb *fb, struct tms_program *p)
{
    int was_enabled = 0;
    int last;

    if (!fb->double_buffering)
        return;

    if (tms.framebuffer == fb) {
        tms_fb_unbind(fb);
        was_enabled = 1;
    }

    last = fb->toggle;
    fb->toggle = !last;

    if (p) {
        tms_program_bind(p);

        for (int x=0; x<fb->num_textures; x++) {
            glActiveTexture(GL_TEXTURE0+x);
            glBindTexture(GL_TEXTURE_2D, fb->fb_texture[last][x]);
        }

        tms_fb_bind(fb);
        tms_mesh_render(quad, p);

        for (int x=0; x<fb->num_textures; x++) {
            glActiveTexture(GL_TEXTURE0+x);
            glBindTexture(GL_TEXTURE_2D, 0);
        }

        if (!was_enabled)
            tms_fb_unbind(fb);
    } else if (was_enabled)
        tms_fb_bind(fb);
}

int
tms_fb_bind_current_textures(struct tms_fb *fb, int first_unit)
{
    for (int x=0; x<fb->num_textures; x++) {
        glActiveTexture(first_unit+x);
        //tms_infof("binding %d", fb->fb_texture[fb->toggle][x]);
        glBindTexture(GL_TEXTURE_2D, fb->fb_texture[fb->toggle][x]);
    }

    return fb->num_textures;
}

int
tms_fb_bind_last_textures(struct tms_fb *fb, int first_unit)
{
    for (int x=0; x<fb->num_textures; x++) {
        glActiveTexture(first_unit+x);
        glBindTexture(GL_TEXTURE_2D, fb->fb_texture[!fb->toggle][x]);
    }

    return fb->num_textures;
}

static int _bind(struct tms_fb *f)
{
    if (f != 0) {
        if (glad_glBindFramebuffer)
            glBindFramebuffer(GL_FRAMEBUFFER, f->fb_o[f->toggle]);
        else
            glBindFramebufferEXT(GL_FRAMEBUFFER, f->fb_o[f->toggle]);

        glViewport(0, 0, f->width, f->height);
    } else {
        if (glad_glBindFramebuffer)
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        else
            glBindFramebufferEXT(GL_FRAMEBUFFER, 0);

        glViewport(0, 0, tms.opengl_width, tms.opengl_height);
    }

    return T_OK;
}

/**
 * Bind the framebuffer for rendering, framebuffers attempt to work like a stack,
 * keeping track of the previously bound framebuffer, but if the developer mess
 * up the bind/unbind order, bad things will happen.
 *
 * Very bad things will also happen if you use any opengl framebuffer
 * functions manually.
 *
 * @relates tms_framebuffer
 **/
int
tms_fb_bind(struct tms_fb *f)
{
    //tms_infof("  bind. t.f: % 10p. f: % 10p. f.p: % 10p", tms.framebuffer, f, f->previous);

    assert(tms.framebuffer != f);

    f->previous = tms.framebuffer;
    tms.framebuffer = f;

    _bind(f);

    return T_OK;
}

int
tms_fb_unbind(struct tms_fb *f)
{
    //tms_infof("unbind. t.f: % 10p. f: % 10p. f.p: % 10p", tms.framebuffer, f, f->previous);

    assert(tms.framebuffer == f);

    _bind(tms.framebuffer = f->previous);
    f->previous = 0;

    return T_OK;
}

/**
 * Render the framebuffer as a fullscreen textured
 * quad using the provided shader.
 **/
void
tms_fb_render(struct tms_fb *f,
              struct tms_program *p)
{
    tms_assertf(tms.framebuffer != f, "can not render framebuffer to itself");

    for (int x=0; x<f->num_textures; x++) {
        glActiveTexture(GL_TEXTURE0+x);
        glBindTexture(GL_TEXTURE_2D, f->fb_texture[f->toggle][x]);
    }

    tms_mesh_render(quad, p);

    for (int x=0; x<f->num_textures; x++) {
        glActiveTexture(GL_TEXTURE0+x);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

void
tms_fb_render_to(struct tms_fb *f,
                 struct tms_fb *dest,
                 struct tms_program *p)
{
    if (!p) p = copy_program;

    tms_fb_bind(dest);
    tms_fb_render(f, p);
    tms_fb_unbind(dest);
}

