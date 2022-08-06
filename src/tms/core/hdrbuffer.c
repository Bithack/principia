#include "hdrbuffer.h"
#include "shader.h"
#include "tms.h"
#include <tms/backend/opengl.h>

#define GLSL(a) #a

static const char* src_luminance4x4[] =
{
GLSL (
    attribute vec3 position;
    uniform mat4 _tms_MVP;

    void main(void) {
        gl_Position = _tms_MVP*vec4(position, 1.0);
    }
) ,
GLSL(
    uniform sampler2D tex;

    float luminance(vec3 a)
    {
        return .2126*a.r + .7152*a.g + .0722*a.b;
    }

    void main(void) {
        float nmax = 0;
        float lmax = 0;
        float lum = 0.0;

        vec2 fc = ((gl_FragCoord.xy-vec2(.5,.5)) * 4.0) / 512.0;

        for (int y=0; y<4; y++) {
            float h = float(y)/512.0;
            float l0 = luminance(texture2D(tex, fc + vec2(0, h)));
            float l1 = luminance(texture2D(tex, fc + vec2(1.0/512.0, h)));
            float l2 = luminance(texture2D(tex, fc + vec2(2.0/512.0, h)));
            float l3 = luminance(texture2D(tex, fc + vec2(3.0/512.0, h)));

            lum +=
                log(.00001 + l0)
                +log(.00001 + l1)
                +log(.00001 + l2)
                +log(.00001 + l3);

            lmax = max(max(l0, l1), max(l2, l3));
            nmax = max(lmax, nmax);
        }

        float llum = lum * .0625;
        gl_FragColor = vec4(llum, nmax, 0, 1);
    }
)
};

static const char* src_downsample4x4[] =
{
GLSL (
    attribute vec3 position;
    //attribute vec2 texcoord;
    uniform mat4 _tms_MVP;

    //varying vec2 frag_uv;

    void main(void) {
        //frag_uv = texcoord;
        gl_Position = _tms_MVP*vec4(position, 1.0);
    }
) ,
GLSL(
    uniform sampler2D tex;
    uniform float size;

    //varying vec2 frag_uv;

    void main(void) {
        float nmax = 0;
        float lmax = 0;
        float lum = 0.0;

        //frag_uv += vec2(.5/size, .5/size);

        vec2 fc = ((gl_FragCoord.xy-vec2(.5,.5)) * 4.0) / size;

        for (int y=0; y<4; y++) {
            float h = float(y)/size;
            vec4 c0 = texture2D(tex, fc + vec2(0, h));
            vec4 c1 = texture2D(tex, fc + vec2(1.0/size, h));
            vec4 c2 = texture2D(tex, fc + vec2(2.0/size, h));
            vec4 c3 = texture2D(tex, fc + vec2(3.0/size, h));

            lum += c0.x+c1.x+c2.x+c3.x;

            lmax = max(max(c0.y, c1.y), max(c2.y, c3.y));
            nmax = max(lmax, nmax);
        }

        gl_FragColor = vec4(lum*.0625, nmax,  0, 1);
    }
)
};

static const char* src_downsample2x2[] =
{
GLSL (
    attribute vec3 position;
    uniform mat4 _tms_MVP;

    void main(void) {
        gl_Position = _tms_MVP*vec4(position, 1.0);
    }
) ,
GLSL(
    uniform sampler2D tex;

    void main(void) {
        //frag_uv -= vec2(.5/2, .5/2);

        vec4 c0 = texture2D(tex, vec2(0,0));
        vec4 c1 = texture2D(tex, vec2(1,0));
        vec4 c2 = texture2D(tex, vec2(1,1));
        vec4 c3 = texture2D(tex, vec2(0,1));

        float nmax = max(max(c0.y, c1.y), max(c2.y, c3.y));

        gl_FragColor = vec4(
            exp((c0.x+c1.x+c2.x+c3.x)*.25),
            nmax, 0, 1);
    }
)
};

/*
static const char* src_brightpass[] =
{
GLSL (
    attribute vec3 position;
    attribute vec2 texcoord;
    uniform mat4 _tms_MVP;

    varying vec2 frag_uv;

    void main(void) {
        frag_uv = texcoord;
        gl_Position = _tms_MVP*position;
    }
) ,
GLSL(
    uniform sampler2D tex;
    uniform float tone;
    uniform float width;
    uniform float height;

    varying vec2 frag_uv;

    float luminance(vec3 a)
    {
        return .2126*a.r + .7152*a.g + .0722*a.b;
    }

    void main(void) {
        vec4 c = texture2D(tex, frag_uv);
        float lum = luminance(c.rgb);

    }
)
};
*/

static const char* src_tonemap[] =
{
GLSL (
    attribute vec3 position;
    attribute vec2 texcoord;
    uniform mat4 _tms_MVP;

    varying vec2 frag_uv;

    void main(void) {
        frag_uv = texcoord;
        gl_Position = _tms_MVP*vec4(position, 1.0);
    }
) ,
GLSL(
    uniform sampler2D tex;
    uniform sampler2D tone;
    varying vec2 frag_uv;

    float luminance(vec3 a)
    {
        return .2126*a.r + .7152*a.g + .0722*a.b;
    }

    void main(void) {
        float exposure = 0.8f;
        vec4 color = texture2D(tex, frag_uv);
        vec2 l = texture2D(tone, vec2(.5,.5)).rg;

        //l.g = max(1.1, l.g);
        l.g = 1.1;

        float lp = (exposure / l.r) * luminance(color);
        //float lmsqr = (l.g*l.g)*(l.g*l.g);
        float lmsqr = pow(l.g, 3);
        float tonesc = (lp * (1.0 + (lp / lmsqr))) / (1.0 + lp);
        color.rgb *= tonesc/l.g;

        gl_FragColor = vec4(color.rgb, 1.0);

    /*
        vec4 c = texture2D(tex, frag_uv);
        vec3 col = c.rgb;
        float lum = luminance(c.rgb);
        float wmax = texture2D(tone, vec2(.5,.5)).y;
        float key = texture2D(tone, vec2(.5,.5)).x;

        col *= .72 / (.0001 + key);
        col *= 1.0 + col/1.5;
        col /= 1.0 + col;

        gl_FragColor = vec4(
            col
            , 1.0);
            */
    }
)
};

int initialized = 0;
static struct tms_shader *shader_luminance4x4;
static struct tms_shader *shader_downsample4x4;
static struct tms_shader *shader_downsample2x2;
static struct tms_shader *shader_tonemap;

static void init(void)
{
    /*
    tms_infof("compiling luminance4x4");
    shader_luminance4x4 = tms_shader_alloc();
    tms_shader_compile(shader_luminance4x4, src_luminance4x4[0], src_luminance4x4[1]);

    tms_infof("compiling downsample4x4");
    shader_downsample4x4 = tms_shader_alloc();
    tms_shader_compile(shader_downsample4x4, src_downsample4x4[0], src_downsample4x4[1]);

    tms_infof("compiling downsample2x2");
    shader_downsample2x2 = tms_shader_alloc();
    tms_shader_compile(shader_downsample2x2, src_downsample2x2[0], src_downsample2x2[1]);

    tms_infof("compiling tonemap");
    shader_tonemap = tms_shader_alloc();
    tms_shader_compile(shader_tonemap, src_tonemap[0], src_tonemap[1]);
    */
}

struct tms_hdrbuffer*
tms_hdrbuffer_alloc(void)
{
    struct tms_hdrbuffer *r = calloc(1, sizeof(struct tms_hdrbuffer));

    if (!initialized)
        init();
    /*

    r->super.width = tms.window_width;
    r->super.height = tms.window_height;
    r->super.depth = 1;
    r->super.format = GL_RGBA16F;
    //r->super.format = GL_RGBA;
    tms_framebuffer_init(r);

    r->luminance[0] = tms_framebuffer_alloc_hdr(128, 128);
    r->luminance[1] = tms_framebuffer_alloc_hdr(32, 32);
    r->luminance[2] = tms_framebuffer_alloc_hdr(8, 8);
    r->luminance[3] = tms_framebuffer_alloc_hdr(2, 2);
    r->luminance[4] = tms_framebuffer_alloc_hdr(1, 1);
    r->downsampled = tms_framebuffer_alloc_hdr(512, 512);
    */

    return r;
}

void
tms_hdrbuffer_bind(struct tms_hdrbuffer *h)
{
    //tms_framebuffer_bind(h);
}

void
tms_hdrbuffer_unbind(struct tms_hdrbuffer *h)
{
#if 0
    //tms_framebuffer_unbind(h);

    //tms_framebuffer_render_to(h, h->downsampled, tms_get_shader("2d-texture"));

    /* downsampled = 512x512
     * luminance[0] = 128x128
     * luminance[1] = 32x32
     * luminance[2] = 8x8
     * luminance[3] = 2x2
     * luminance[4] = 1x1
     * */

    tms_framebuffer_render_to(
            h->downsampled,
            h->luminance[0],
            shader_luminance4x4
            );

    //tms_shader_bind(shader_downsample4x4);

    GLuint loc;
    //GLuint loc = tms_shader_get_uniform(shader_downsample4x4, "size");
    glUniform1f(loc, 128.f);

    tms_framebuffer_render_to(
            h->luminance[0],
            h->luminance[1],
            shader_downsample4x4
            );

    glUniform1f(loc, 32.f);

    tms_framebuffer_render_to(
            h->luminance[1],
            h->luminance[2],
            shader_downsample4x4
            );

    glUniform1f(loc, 8.f);

    tms_framebuffer_render_to(
            h->luminance[2],
            h->luminance[3],
            shader_downsample4x4
            );

    tms_framebuffer_render_to(
            h->luminance[3],
            h->luminance[4],
            shader_downsample2x2
            );

    //tms_shader_bind(shader_tonemap);
    //loc = tms_shader_get_uniform(shader_tonemap, "tone");
    glUniform1i(loc, 1);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, h->luminance[4]->fbo_texture);
    glActiveTexture(GL_TEXTURE0);

    tms_framebuffer_render(h, shader_tonemap);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);

    /* set brightpass uniform luminance */

    /*
    tms_framebuffer_render_to(
            h->downsampled,
            h->bloom[0],
            shader_brightpass
            );

    tms_framebuffer_render_to(
            h->downsampled,
            h->bloom[0],
            shader_brightpass
            );
            */
#endif
}

