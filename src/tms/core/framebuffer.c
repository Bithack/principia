#include "tms.h"
#include "framebuffer.h"
#include "gbuffer.h"
#include "varray.h"
#include "mesh.h"
#include "glob.h"

#include <stdlib.h>
#include <assert.h>

#ifndef GLSL
#define GLSL(...) #__VA_ARGS__
#endif

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

static const char *copy_sources[] = {
    "attribute vec2 position;"
    "attribute vec2 texcoord;"
    "varying lowp vec2 FS_texcoord;"

    "void main(void) {"
        "FS_texcoord = texcoord;"
        "gl_Position = vec4(position, 0, 1.);"
    "}",

    "uniform sampler2D tex_0;"
    "varying lowp vec2 FS_texcoord;"

    "void main(void) {"
        "gl_FragColor = texture2D(tex_0, FS_texcoord);"
    "}"
};


static const char *blur3x3v_sources[] = {
    "attribute vec2 position;"
    "attribute vec2 texcoord;"

    "void main(void) {"
        "gl_Position = vec4(position, 0, 1.);"
    "}",

    "uniform lowp sampler2D tex_0;"

#if 0
GLSL(

void main ()
{
  mediump ivec2 tmpvar_1;
  tmpvar_1 = ivec2(gl_FragCoord.xy);
  gl_FragColor = ((((2.0 * texture2D (tex_0, (vec2((tmpvar_1 + ivec2(0, -1))) * vec2(0.00390625, 0.00390625)))) + (4.0 * texture2D (tex_0, (vec2(tmpvar_1) * vec2(0.00390625, 0.00390625))))) + (2.0 * texture2D (tex_0, (vec2((tmpvar_1 + ivec2(0, 1))) * vec2(0.00390625, 0.00390625))))) / 8.0);
}

)
#endif

#if 1

    "void main(void) {"
        "lowp vec4 color;"
        "vec2 tx = gl_FragCoord.xy;"
        "color = .25 * texture2D(tex_0, (tx+vec2(0.0, -1.0)) * vec2(0.00390625, 0.00390625));"
        "color += .5 * texture2D(tex_0, tx * vec2(0.00390625, 0.00390625));"
        "color += .25 * texture2D(tex_0, (tx+vec2(0.0, 1.0)) * vec2(0.00390625, 0.00390625));"
        "gl_FragColor = color;"

        /*
        "color = texture(tex_1, FS_texcoord+vec2(0, -2./size));"
        "color += 4.*texture(tex_1, FS_texcoord+vec2(0, -1./size));"
        "color += 6.*texture(tex_1, FS_texcoord+vec2(0, 0));"
        "color += 4.*texture(tex_1, FS_texcoord+vec2(0, 1./size));"
        "color += texture(tex_1, FS_texcoord+vec2(0, 2./size));"
        "gl_FragData[1] = color/16.;"

        "color = texture(tex_2, FS_texcoord+vec2(0, -2./size));"
        "color += 4.*texture(tex_2, FS_texcoord+vec2(0, -1./size));"
        "color += 6.*texture(tex_2, FS_texcoord+vec2(0, 0));"
        "color += 4.*texture(tex_2, FS_texcoord+vec2(0, 1./size));"
        "color += texture(tex_2, FS_texcoord+vec2(0, 2./size));"
        "gl_FragData[2] = color/16.;"
        */
    "}"
#endif
};

static const char *blur3x3_128_sources[] = {

    "attribute vec2 position;"
    "attribute vec2 texcoord;"

    "varying lowp vec2 tx0;"
    "varying lowp vec2 tx1;"
    "varying lowp vec2 tx2;"
    "varying lowp vec2 tx3;"
    "varying lowp vec2 tx4;"
    "varying lowp vec2 tx5;"
    "varying lowp vec2 tx6;"
    "varying lowp vec2 tx7;"
    "varying lowp vec2 tx8;"

    "void main(void) {"
        "tx0 = texcoord;"
        "tx1 = texcoord + vec2(-1./128.,0.);"
        "tx2 = texcoord + vec2(1./128.,0.);"
        "tx3 = texcoord + vec2(0., 1./128.);"
        "tx4 = texcoord + vec2(0.,-1./128.);"

        /*
        "tx5 = texcoord + vec2(-1./256.,-1./256.);"
        "tx6 = texcoord + vec2(1./256.,-1./256.);"
        "tx7 = texcoord + vec2(-1./256.,1./256.);"
        "tx8 = texcoord + vec2(1./256.,1./256.);"
        */

        "gl_Position = vec4(position, 0, 1.);"
    "}",

    "uniform lowp sampler2D tex_0;"

    "varying lowp vec2 tx0;"
    "varying lowp vec2 tx1;"
    "varying lowp vec2 tx2;"
    "varying lowp vec2 tx3;"
    "varying lowp vec2 tx4;"
    "varying lowp vec2 tx5;"
    "varying lowp vec2 tx6;"
    "varying lowp vec2 tx7;"
    "varying lowp vec2 tx8;"

#if 0
GLSL(
void main ()
{
  mediump ivec2 tmpvar_1;
  tmpvar_1 = ivec2(gl_FragCoord.xy);
  gl_FragColor = ((((2.0 * texture2D (tex_0, (vec2((tmpvar_1 + ivec2(-1, 0))) * vec2(0.00390625, 0.00390625)))) + (4.0 * texture2D (tex_0, (vec2(tmpvar_1) * vec2(0.00390625, 0.00390625))))) + (2.0 * texture2D (tex_0, (vec2((tmpvar_1 + ivec2(1, 0))) * vec2(0.00390625, 0.00390625))))) / 8.0);
}

)
#endif


#if 1
    "void main(void) {"
        "lowp vec4 color;"
        "color = .25 * texture2D(tex_0, tx0);"
        "color += .125 * texture2D(tex_0, tx1);"
        "color += .125 * texture2D(tex_0, tx2);"
        "color += .125 * texture2D(tex_0, tx3);"
        "color += .125 * texture2D(tex_0, tx4);"
        /*
        "color += .0625 * texture2D(tex_0, tx5);"
        "color += .0625 * texture2D(tex_0, tx6);"
        "color += .0625 * texture2D(tex_0, tx7);"
        "color += .0625 * texture2D(tex_0, tx8);"
        */
        "gl_FragColor = color;"

    /*
        "ivec2 tx = ivec2(gl_FragCoord.xy);"
        "vec4 color = .25*texelFetch(tex_0, tx+ivec2(-1,0), 0);"
        "color += .5*texelFetch(tex_0, tx, 0);"
        "color += .25*texelFetch(tex_0, tx+ivec2(1,0), 0);"
        "gl_FragColor = color;"
        */

        /*
        "color = texture(tex_1, FS_texcoord+vec2(0, -2./size));"
        "color += 4.*texture(tex_1, FS_texcoord+vec2(0, -1./size));"
        "color += 6.*texture(tex_1, FS_texcoord+vec2(0, 0));"
        "color += 4.*texture(tex_1, FS_texcoord+vec2(0, 1./size));"
        "color += texture(tex_1, FS_texcoord+vec2(0, 2./size));"
        "gl_FragData[1] = color/16.;"

        "color = texture(tex_2, FS_texcoord+vec2(0, -2./size));"
        "color += 4.*texture(tex_2, FS_texcoord+vec2(0, -1./size));"
        "color += 6.*texture(tex_2, FS_texcoord+vec2(0, 0));"
        "color += 4.*texture(tex_2, FS_texcoord+vec2(0, 1./size));"
        "color += texture(tex_2, FS_texcoord+vec2(0, 2./size));"
        "gl_FragData[2] = color/16.;"
        */
    "}"
#endif
};

static const char *blur3x3_sources[] = {

    "attribute vec2 position;"
    "attribute vec2 texcoord;"

    "varying lowp vec2 tx0;"
    "varying lowp vec2 tx1;"
    "varying lowp vec2 tx2;"
    "varying lowp vec2 tx3;"
    "varying lowp vec2 tx4;"
    "varying lowp vec2 tx5;"
    "varying lowp vec2 tx6;"
    "varying lowp vec2 tx7;"
    "varying lowp vec2 tx8;"

    "void main(void) {"
        "tx0 = texcoord;"
        "tx1 = texcoord + vec2(-1./256.,0.);"
        "tx2 = texcoord + vec2(1./256.,0.);"
        "tx3 = texcoord + vec2(0., 1./256.);"
        "tx4 = texcoord + vec2(0.,-1./256.);"

        /*
        "tx5 = texcoord + vec2(-1./256.,-1./256.);"
        "tx6 = texcoord + vec2(1./256.,-1./256.);"
        "tx7 = texcoord + vec2(-1./256.,1./256.);"
        "tx8 = texcoord + vec2(1./256.,1./256.);"
        */

        "gl_Position = vec4(position, 0, 1.);"
    "}",

    "uniform lowp sampler2D tex_0;"

    "varying lowp vec2 tx0;"
    "varying lowp vec2 tx1;"
    "varying lowp vec2 tx2;"
    "varying lowp vec2 tx3;"
    "varying lowp vec2 tx4;"
    "varying lowp vec2 tx5;"
    "varying lowp vec2 tx6;"
    "varying lowp vec2 tx7;"
    "varying lowp vec2 tx8;"

#if 0
GLSL(
void main ()
{
  mediump ivec2 tmpvar_1;
  tmpvar_1 = ivec2(gl_FragCoord.xy);
  gl_FragColor = ((((2.0 * texture2D (tex_0, (vec2((tmpvar_1 + ivec2(-1, 0))) * vec2(0.00390625, 0.00390625)))) + (4.0 * texture2D (tex_0, (vec2(tmpvar_1) * vec2(0.00390625, 0.00390625))))) + (2.0 * texture2D (tex_0, (vec2((tmpvar_1 + ivec2(1, 0))) * vec2(0.00390625, 0.00390625))))) / 8.0);
}

)
#endif


#if 1
    "void main(void) {"
        "lowp vec4 color;"
        "color = .25 * texture2D(tex_0, tx0);"
        "color += .125 * texture2D(tex_0, tx1);"
        "color += .125 * texture2D(tex_0, tx2);"
        "color += .125 * texture2D(tex_0, tx3);"
        "color += .125 * texture2D(tex_0, tx4);"
        /*
        "color += .0625 * texture2D(tex_0, tx5);"
        "color += .0625 * texture2D(tex_0, tx6);"
        "color += .0625 * texture2D(tex_0, tx7);"
        "color += .0625 * texture2D(tex_0, tx8);"
        */
        "gl_FragColor = color*1.3333333;"

    /*
        "ivec2 tx = ivec2(gl_FragCoord.xy);"
        "vec4 color = .25*texelFetch(tex_0, tx+ivec2(-1,0), 0);"
        "color += .5*texelFetch(tex_0, tx, 0);"
        "color += .25*texelFetch(tex_0, tx+ivec2(1,0), 0);"
        "gl_FragColor = color;"
        */

        /*
        "color = texture(tex_1, FS_texcoord+vec2(0, -2./size));"
        "color += 4.*texture(tex_1, FS_texcoord+vec2(0, -1./size));"
        "color += 6.*texture(tex_1, FS_texcoord+vec2(0, 0));"
        "color += 4.*texture(tex_1, FS_texcoord+vec2(0, 1./size));"
        "color += texture(tex_1, FS_texcoord+vec2(0, 2./size));"
        "gl_FragData[1] = color/16.;"

        "color = texture(tex_2, FS_texcoord+vec2(0, -2./size));"
        "color += 4.*texture(tex_2, FS_texcoord+vec2(0, -1./size));"
        "color += 6.*texture(tex_2, FS_texcoord+vec2(0, 0));"
        "color += 4.*texture(tex_2, FS_texcoord+vec2(0, 1./size));"
        "color += texture(tex_2, FS_texcoord+vec2(0, 2./size));"
        "gl_FragData[2] = color/16.;"
        */
    "}"
#endif
};

static const char *blur3x3h_sources[] = {

    "attribute vec2 position;"
    "attribute vec2 texcoord;"

    "void main(void) {"
        "gl_Position = vec4(position, 0, 1.);"
    "}",

    "uniform lowp sampler2D tex_0;"

#if 0
GLSL(
void main ()
{
  mediump ivec2 tmpvar_1;
  tmpvar_1 = ivec2(gl_FragCoord.xy);
  gl_FragColor = ((((2.0 * texture2D (tex_0, (vec2((tmpvar_1 + ivec2(-1, 0))) * vec2(0.00390625, 0.00390625)))) + (4.0 * texture2D (tex_0, (vec2(tmpvar_1) * vec2(0.00390625, 0.00390625))))) + (2.0 * texture2D (tex_0, (vec2((tmpvar_1 + ivec2(1, 0))) * vec2(0.00390625, 0.00390625))))) / 8.0);
}

)
#endif


#if 1
    "void main(void) {"
        "lowp vec4 color;"
        "vec2 tx = gl_FragCoord.xy;"
        "color = .25 * texture2D(tex_0, (tx+vec2(-1.0, 0.0)) * vec2(0.00390625, 0.00390625));"
        "color += .5 * texture2D(tex_0, tx * vec2(0.00390625, 0.00390625));"
        "color += .25 * texture2D(tex_0, (tx+vec2(1.0, 0.0)) * vec2(0.00390625, 0.00390625));"
        "gl_FragColor = color;"

    /*
        "ivec2 tx = ivec2(gl_FragCoord.xy);"
        "vec4 color = .25*texelFetch(tex_0, tx+ivec2(-1,0), 0);"
        "color += .5*texelFetch(tex_0, tx, 0);"
        "color += .25*texelFetch(tex_0, tx+ivec2(1,0), 0);"
        "gl_FragColor = color;"
        */

        /*
        "color = texture(tex_1, FS_texcoord+vec2(0, -2./size));"
        "color += 4.*texture(tex_1, FS_texcoord+vec2(0, -1./size));"
        "color += 6.*texture(tex_1, FS_texcoord+vec2(0, 0));"
        "color += 4.*texture(tex_1, FS_texcoord+vec2(0, 1./size));"
        "color += texture(tex_1, FS_texcoord+vec2(0, 2./size));"
        "gl_FragData[1] = color/16.;"

        "color = texture(tex_2, FS_texcoord+vec2(0, -2./size));"
        "color += 4.*texture(tex_2, FS_texcoord+vec2(0, -1./size));"
        "color += 6.*texture(tex_2, FS_texcoord+vec2(0, 0));"
        "color += 4.*texture(tex_2, FS_texcoord+vec2(0, 1./size));"
        "color += texture(tex_2, FS_texcoord+vec2(0, 2./size));"
        "gl_FragData[2] = color/16.;"
        */
    "}"
#endif
};

static const char *blur5x5v_512_sources[] = {
    "attribute vec2 position;"
    "attribute vec2 texcoord;"
    "varying vec2 FS_texcoord;"

    "void main(void) {"
        "FS_texcoord = texcoord;"
        "gl_Position = vec4(position, 0, 1.);"
    "}",

    "uniform sampler2D tex_0;"
    "varying vec2 FS_texcoord;"

    "void main(void) {"
        "vec4 color = texture2D(tex_0, FS_texcoord+vec2(0.,-2.*1./512.));"
        "color += 4.*texture2D(tex_0, FS_texcoord+vec2(0.,-1.*1./512.));"
        "color += 6.*texture2D(tex_0, FS_texcoord);"
        "color += 4.*texture2D(tex_0, FS_texcoord+vec2(0.,1.*1./512.));"
        "color += texture2D(tex_0, FS_texcoord+vec2(0.,2.*1./512.));"
        "gl_FragColor = color/16.;"
    "}"
};
static const char *blur5x5h_512_sources[] = {
    "attribute vec2 position;"
    "attribute vec2 texcoord;"
    "varying vec2 FS_texcoord;"

    "void main(void) {"
        "FS_texcoord = texcoord;"
        "gl_Position = vec4(position, 0, 1.);"
    "}",

    "uniform sampler2D tex_0;"
    "varying vec2 FS_texcoord;"

    "void main(void) {"
        "vec4 color = texture2D(tex_0, FS_texcoord+vec2(-2.*1./512.,0.));"
        "color += 4.*texture2D(tex_0, FS_texcoord+vec2(-1.*1./512.,0.));"
        "color += 6.*texture2D(tex_0, FS_texcoord);"
        "color += 4.*texture2D(tex_0, FS_texcoord+vec2(1.*1./512.,0.));"
        "color += texture2D(tex_0, FS_texcoord+vec2(2.*1./512.,0.));"
        "gl_FragColor = color/16.;"
    "}"
};

static const char *blur5x5v_256_sources[] = {
    "attribute vec2 position;"
    "attribute vec2 texcoord;"
    "varying vec2 FS_texcoord;"

    "void main(void) {"
        "FS_texcoord = texcoord;"
        "gl_Position = vec4(position, 0, 1.);"
    "}",

    "uniform sampler2D tex_0;"
    "varying vec2 FS_texcoord;"

    "void main(void) {"
        "vec4 color = texture2D(tex_0, FS_texcoord+vec2(0.,-2.*1./256.));"
        "color += 4.*texture2D(tex_0, FS_texcoord+vec2(0.,-1.*1./256.));"
        "color += 6.*texture2D(tex_0, FS_texcoord);"
        "color += 4.*texture2D(tex_0, FS_texcoord+vec2(0.,1.*1./256.));"
        "color += texture2D(tex_0, FS_texcoord+vec2(0.,2.*1./256.));"
        "gl_FragColor = color/16.;"
    "}"
};
static const char *blur5x5h_256_sources[] = {
    "attribute vec2 position;"
    "attribute vec2 texcoord;"
    "varying vec2 FS_texcoord;"

    "void main(void) {"
        "FS_texcoord = texcoord;"
        "gl_Position = vec4(position, 0, 1.);"
    "}",

    "uniform sampler2D tex_0;"
    "varying vec2 FS_texcoord;"

    "void main(void) {"
        "vec4 color = texture2D(tex_0, FS_texcoord+vec2(-2.*1./256.,0.));"
        "color += 4.*texture2D(tex_0, FS_texcoord+vec2(-1.*1./256.,0.));"
        "color += 6.*texture2D(tex_0, FS_texcoord);"
        "color += 4.*texture2D(tex_0, FS_texcoord+vec2(1.*1./256.,0.));"
        "color += texture2D(tex_0, FS_texcoord+vec2(2.*1./256.,0.));"
        "gl_FragColor = color/16.;"
    "}"
};

static void init()
{
    struct tms_varray *va = tms_varray_alloc(2);
    struct tms_gbuffer *v = tms_gbuffer_alloc_fill(verts, sizeof(verts));
    tms_gbuffer_upload(v);
    tms_varray_map_attribute(va, "position", 2, GL_FLOAT, v);
    tms_varray_map_attribute(va, "texcoord", 2, GL_FLOAT, v);
    quad = tms_mesh_alloc(va, 0);
    quad->primitive_type = TMS_TRIANGLE_FAN;
    tms_mesh_set_autofree_buffers(quad, 1);


    struct tms_shader *sh;
    sh = tms_shader_alloc();
    tms_shader_compile(sh, GL_VERTEX_SHADER, blur5x5h_256_sources[0]);
    tms_shader_compile(sh, GL_FRAGMENT_SHADER, blur5x5h_256_sources[1]);
    blur5x5h_256_program = tms_shader_get_program(sh, TMS_NO_PIPELINE);

    sh = tms_shader_alloc();
    tms_shader_compile(sh, GL_VERTEX_SHADER, blur5x5v_256_sources[0]);
    tms_shader_compile(sh, GL_FRAGMENT_SHADER, blur5x5v_256_sources[1]);
    blur5x5v_256_program = tms_shader_get_program(sh, TMS_NO_PIPELINE);

    sh = tms_shader_alloc();
    tms_shader_compile(sh, GL_VERTEX_SHADER, blur5x5h_512_sources[0]);
    tms_shader_compile(sh, GL_FRAGMENT_SHADER, blur5x5h_512_sources[1]);
    blur5x5h_512_program = tms_shader_get_program(sh, TMS_NO_PIPELINE);

    sh = tms_shader_alloc();
    tms_shader_compile(sh, GL_VERTEX_SHADER, blur5x5v_512_sources[0]);
    tms_shader_compile(sh, GL_FRAGMENT_SHADER, blur5x5v_512_sources[1]);
    blur5x5v_512_program = tms_shader_get_program(sh, TMS_NO_PIPELINE);

    sh = tms_shader_alloc();
    tms_shader_compile(sh, GL_VERTEX_SHADER, blur3x3_sources[0]);
    tms_shader_compile(sh, GL_FRAGMENT_SHADER, blur3x3_sources[1]);
    blur3x3_program = tms_shader_get_program(sh, TMS_NO_PIPELINE);

    sh = tms_shader_alloc();
    tms_shader_compile(sh, GL_VERTEX_SHADER, blur3x3_128_sources[0]);
    tms_shader_compile(sh, GL_FRAGMENT_SHADER, blur3x3_128_sources[1]);
    blur3x3_128_program = tms_shader_get_program(sh, TMS_NO_PIPELINE);

    sh = tms_shader_alloc();
    tms_shader_compile(sh, GL_VERTEX_SHADER, blur3x3h_sources[0]);
    tms_shader_compile(sh, GL_FRAGMENT_SHADER, blur3x3h_sources[1]);
    blur3x3h_program = tms_shader_get_program(sh, TMS_NO_PIPELINE);

    sh = tms_shader_alloc();
    tms_shader_compile(sh, GL_VERTEX_SHADER, blur3x3v_sources[0]);
    tms_shader_compile(sh, GL_FRAGMENT_SHADER, blur3x3v_sources[1]);
    blur3x3v_program = tms_shader_get_program(sh, TMS_NO_PIPELINE);

    sh = tms_shader_alloc();
    tms_shader_compile(sh, GL_VERTEX_SHADER, copy_sources[0]);
    tms_shader_compile(sh, GL_FRAGMENT_SHADER, copy_sources[1]);
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
    tms_assertf((ierr = glGetError()) == 0, "gl error %d before tms_fb_init", ierr);
#ifdef TMS_USE_GLEW
    if (__glewGenFramebuffers) {
        __glewGenFramebuffers(fb->double_buffering ? 2 : 1, fb->fb_o);
    } else {
        __glewGenFramebuffersEXT(fb->double_buffering ? 2 : 1, fb->fb_o);
    }
#else
    glGenFramebuffers(fb->double_buffering ? 2 : 1, fb->fb_o);
#endif
    tms_assertf((ierr = glGetError()) == 0, "gl error %d after tms_fb_init", ierr);
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
tms_fb_attach_depth(struct tms_fb *fb, int attachment, int name)
{
    //if (fb->double_buffering)
        //tms_fatalf("tms_fb_attach functions not compatible with double buffered fb's");

    tms_fatalf("not implemented");

    /*
    for (int x=0; x<(fb->double_buffering ? 2 : 1); x++) {
        glBindFramebuffer(GL_FRAMEBUFFER, fb->fb_o[x]);
        fb->fb_depth[x] = name;
        glBindRenderbuffer(GL_RENDERBUFFER, fb->fb_depth[x]);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, fb->fb_depth[x]);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            tms_fatalf("JSDAKLJDKLASKDJKLAJ");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    */

}

void
tms_fb_enable_depth_and_stencil(struct tms_fb *fb)
{
    tms_fatalf("not implemented");
    /*
    for (int x=0; x<(fb->double_buffering ? 2 : 1); x++) {
        glBindFramebuffer(GL_FRAMEBUFFER, fb->fb_o[x]);

        glGenRenderbuffers(1, &fb->fb_depth[x]);
        glBindRenderbuffer(GL_RENDERBUFFER, fb->fb_depth[x]);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, fb->width, fb->height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, fb->fb_depth[x]);

        glBindRenderbuffer(GL_RENDERBUFFER, 0);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    */

}

void
tms_fb_enable_depth(struct tms_fb *fb, int format)
{
    int ierr;
    tms_assertf((ierr = glGetError()) == 0, "gl error %d in tms_fb_enable_depth (begin)", ierr);

    for (int x=0; x<(fb->double_buffering ? 2 : 1); x++) {
#ifdef TMS_USE_GLEW
        //if (GLEW_VERSION_1_5) {
        if (__glewBindFramebuffer) {
            __glewBindFramebuffer(GL_FRAMEBUFFER, fb->fb_o[x]);
        } else {
            __glewBindFramebufferEXT(GL_FRAMEBUFFER, fb->fb_o[x]);
        }
#else
        glBindFramebuffer(GL_FRAMEBUFFER, fb->fb_o[x]);
#endif

        tms_assertf((ierr = glGetError()) == 0, "gl error %d in tms_fb_enable_depth %d 1", ierr, x);

        //tms_infof("enable depth after bind %d", glGetError());

#ifdef TMS_USE_GLEW
        if (__glewGenRenderbuffers) {
            __glewGenRenderbuffers(1, &fb->fb_depth[x]);
            __glewBindRenderbuffer(GL_RENDERBUFFER, fb->fb_depth[x]);
        } else {
            __glewGenRenderbuffersEXT(1, &fb->fb_depth[x]);
            __glewBindRenderbufferEXT(GL_RENDERBUFFER, fb->fb_depth[x]);
        }
#else
        glGenRenderbuffers(1, &fb->fb_depth[x]);
        glBindRenderbuffer(GL_RENDERBUFFER, fb->fb_depth[x]);
#endif

        tms_assertf((ierr = glGetError()) == 0, "gl error %d in tms_fb_enable_depth %d 2-3", ierr, x);
        //tms_infof("enable depth after bindrender %d", glGetError());
        //glRenderbufferStorage(GL_RENDERBUFFER, format, fb->width, fb->height);
        //

#ifdef TMS_USE_GLES
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, fb->width, fb->height);
#elif defined TMS_USE_GLEW
        if (__glewRenderbufferStorage) {
            __glewRenderbufferStorage(GL_RENDERBUFFER, format, fb->width, fb->height);
        } else {
            __glewRenderbufferStorageEXT(GL_RENDERBUFFER, format, fb->width, fb->height);
        }
#else
        glRenderbufferStorage(GL_RENDERBUFFER, format, fb->width, fb->height);
#endif
        tms_assertf((ierr = glGetError()) == 0, "gl error %d in tms_fb_enable_depth %d 4", ierr, x);

#ifdef TMS_USE_GLEW
        if (__glewFramebufferRenderbuffer) {
            __glewFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, fb->fb_depth[x]);
        } else {
            __glewFramebufferRenderbufferEXT(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, fb->fb_depth[x]);
        }
#else
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, fb->fb_depth[x]);
#endif

        tms_assertf((ierr = glGetError()) == 0, "gl error %d in tms_fb_enable_depth %d 5", ierr, x);
        //tms_infof("enable depth after hello %d", glGetError());

#ifdef TMS_USE_GLEW
        if (__glewBindRenderbuffer) {
            __glewBindRenderbuffer(GL_RENDERBUFFER, 0);
        } else {
            __glewBindRenderbufferEXT(GL_RENDERBUFFER, 0);
        }
#else
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
#endif
        tms_assertf((ierr = glGetError()) == 0, "gl error %d in tms_fb_enable_depth %d 6", ierr, x);
    }

#ifdef TMS_USE_GLEW
    if (__glewBindFramebuffer) {
        __glewBindFramebuffer(GL_FRAMEBUFFER, 0);
    } else {
        __glewBindFramebufferEXT(GL_FRAMEBUFFER, 0);
    }
#else
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
#endif
    tms_assertf((ierr = glGetError()) == 0, "gl error %d in tms_fb_enable_depth 7 (end)", ierr);
}

void
tms_fb_enable_depth_texture(struct tms_fb *fb, int format)
{
    for (int x=0; x<(fb->double_buffering ? 2 : 1); x++) {
#ifdef TMS_USE_GLEW
        if (__glewBindFramebuffer) {
            __glewBindFramebuffer(GL_FRAMEBUFFER, fb->fb_o[x]);
        } else {
            __glewBindFramebufferEXT(GL_FRAMEBUFFER, fb->fb_o[x]);
        }
#else
        glBindFramebuffer(GL_FRAMEBUFFER, fb->fb_o[x]);
#endif

        glGenTextures(1, &fb->fb_depth[x]);
        glBindTexture(GL_TEXTURE_2D, fb->fb_depth[x]);

        format = GL_DEPTH_COMPONENT;

        glTexImage2D(GL_TEXTURE_2D, 0, format, fb->width, fb->height, 0, format, GL_UNSIGNED_INT, 0);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
        //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
        //glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_INTENSITY);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, fb->fb_depth[x], 0);

        glBindTexture(GL_TEXTURE_2D, 0);
    }

#ifdef TMS_USE_GLEW
    if (__glewBindFramebuffer) {
        __glewBindFramebuffer(GL_FRAMEBUFFER, 0);
    } else {
        __glewBindFramebufferEXT(GL_FRAMEBUFFER, 0);
    }
#else
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
#endif
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
        //tms_infof("add texture before bind %d", glGetError());
#ifdef TMS_USE_GLEW
        if (__glewBindFramebuffer) {
            __glewBindFramebuffer(GL_FRAMEBUFFER, fb->fb_o[x]);
        } else {
            __glewBindFramebufferEXT(GL_FRAMEBUFFER, fb->fb_o[x]);
        }
#else
        glBindFramebuffer(GL_FRAMEBUFFER, fb->fb_o[x]);
#endif
        tms_assertf((ierr = glGetError()) == 0, "gl error %d in tms_fb_add_texture %d 1", ierr, x);

        glGenTextures(1, &fb->fb_texture[x][fb->num_textures]);
        tms_assertf((ierr = glGetError()) == 0, "gl error %d in tms_fb_add_texture %d 2", ierr, x);

        glBindTexture(GL_TEXTURE_2D, fb->fb_texture[x][fb->num_textures]);
        tms_assertf((ierr = glGetError()) == 0, "gl error %d in tms_fb_add_texture %d 3", ierr, x);

#ifndef TMS_USE_GLES
        if (format == GL_R32F) {
            //glTexImage2D(GL_TEXTURE_2D, 0, format, fb->width, fb->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
            glTexImage2D(GL_TEXTURE_2D, 0, format, fb->width, fb->height, 0, GL_RED, GL_FLOAT, 0);
            tms_assertf((ierr = glGetError()) == 0, "gl error %d in tms_fb_add_texture %d 4", ierr, x);
        } else
        if (format == GL_RGB16F) {
            //glTexImage2D(GL_TEXTURE_2D, 0, format, fb->width, fb->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
            glTexImage2D(GL_TEXTURE_2D, 0, format, fb->width, fb->height, 0, GL_RGB, GL_FLOAT, 0);
            tms_assertf((ierr = glGetError()) == 0, "gl error %d in tms_fb_add_texture %d 4", ierr, x);
        } else
        if (format == GL_RGB32F) {
            //glTexImage2D(GL_TEXTURE_2D, 0, format, fb->width, fb->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
            glTexImage2D(GL_TEXTURE_2D, 0, format, fb->width, fb->height, 0, GL_RGB, GL_FLOAT, 0);
            tms_assertf((ierr = glGetError()) == 0, "gl error %d in tms_fb_add_texture %d 4", ierr, x);
        } else
#endif
        if (format == GL_ALPHA) {
            //glTexImage2D(GL_TEXTURE_2D, 0, format, fb->width, fb->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
            glTexImage2D(GL_TEXTURE_2D, 0, format, fb->width, fb->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
            tms_assertf((ierr = glGetError()) == 0, "gl error %d in tms_fb_add_texture %d 5", ierr, x);
        } else if (format == GL_RGBA4) {
            glTexImage2D(GL_TEXTURE_2D, 0, format, fb->width, fb->height, 0, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4, 0);
            tms_assertf((ierr = glGetError()) == 0, "gl error %d in tms_fb_add_texture %d 6", ierr, x);
        } else if (format == GL_RGB565) {
            glTexImage2D(GL_TEXTURE_2D, 0, format, fb->width, fb->height, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, 0);
            tms_assertf((ierr = glGetError()) == 0, "gl error %d in tms_fb_add_texture %d 7", ierr, x);
        } else {
            glTexImage2D(GL_TEXTURE_2D, 0, format, fb->width, fb->height, 0, format, GL_UNSIGNED_BYTE, 0);
            tms_assertf((ierr = glGetError()) == 0, "gl error %d in tms_fb_add_texture %d 8", ierr, x);
        }

        //tms_infof("add texture after teximage2d %d", glGetError());

        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap_s);
        tms_assertf((ierr = glGetError()) == 0, "gl error %d in tms_fb_add_texture %d 9", ierr, x);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap_t);
        tms_assertf((ierr = glGetError()) == 0, "gl error %d in tms_fb_add_texture %d 10", ierr, x);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter_min);
        tms_assertf((ierr = glGetError()) == 0, "gl error %d in tms_fb_add_texture %d 11", ierr, x);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter_mag);
        tms_assertf((ierr = glGetError()) == 0, "gl error %d in tms_fb_add_texture %d 12", ierr, x);

        //tms_infof("add texture after parameters %d", glGetError());

#ifdef TMS_USE_GLEW
        if (__glewFramebufferTexture2D) {
            __glewFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0+fb->num_textures, GL_TEXTURE_2D, fb->fb_texture[x][fb->num_textures], 0);
        } else {
            __glewFramebufferTexture2DEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0+fb->num_textures, GL_TEXTURE_2D, fb->fb_texture[x][fb->num_textures], 0);
        }
#else
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0+fb->num_textures,
                GL_TEXTURE_2D, fb->fb_texture[x][fb->num_textures], 0);
#endif
        tms_assertf((ierr = glGetError()) == 0, "gl error %d in tms_fb_add_texture %d 13", ierr, x);

        //tms_infof("add texture after fbtex2d %d", glGetError());

        glBindTexture(GL_TEXTURE_2D, 0);
        tms_assertf((ierr = glGetError()) == 0, "gl error %d in tms_fb_add_texture %d 14", ierr, x);

		/* update draw buffers */
        GLenum bufs[fb->num_textures+1];

		for (int y = 0; y < fb->num_textures + 1; y++) {
			bufs[y] = GL_COLOR_ATTACHMENT0 + y;
		}

#ifndef TMS_USE_GLES
        glDrawBuffers(fb->num_textures+1, bufs);
        tms_assertf((ierr = glGetError()) == 0, "gl error %d in tms_fb_add_texture %d 15", ierr, x);
#endif
    }

    fb->num_textures ++;

#ifdef TMS_USE_GLEW
    if (__glewBindFramebuffer) {
        __glewBindFramebuffer(GL_FRAMEBUFFER, 0);
    } else {
        __glewBindFramebufferEXT(GL_FRAMEBUFFER, 0);
    }
#else
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
#endif
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
/*
    tms_program_bind(blur3x3v_program);
    tms_fb_swap(f, blur3x3v_program);*/

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
#ifdef TMS_USE_GLEW
        if (__glewBindFramebuffer) {
            __glewBindFramebuffer(GL_FRAMEBUFFER, f->fb_o[f->toggle]);
        } else {
            __glewBindFramebufferEXT(GL_FRAMEBUFFER, f->fb_o[f->toggle]);
        }
#else
        glBindFramebuffer(GL_FRAMEBUFFER, f->fb_o[f->toggle]);
#endif
        glViewport(0, 0, f->width, f->height);
    } else {
#ifdef TMS_USE_GLEW
        if (__glewBindFramebuffer) {
            __glewBindFramebuffer(GL_FRAMEBUFFER, 0);
        } else {
            __glewBindFramebufferEXT(GL_FRAMEBUFFER, 0);
        }
#elif defined (TMS_BACKEND_IOS)
        glBindFramebuffer(GL_FRAMEBUFFER, viewFramebuffer);
#else
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
#endif
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

