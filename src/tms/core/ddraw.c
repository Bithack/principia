#include <math.h>
#include <stdlib.h>

#include <tms/core/tms.h>
#include <tms/core/ddraw.h>
#include <tms/core/shader.h>
#include <tms/core/program.h>
#include <tms/core/pipeline.h>
#include <tms/core/gbuffer.h>
#include <tms/core/varray.h>
#include <tms/core/mesh.h>
#include <tms/core/atlas.h>
#include <tms/core/texture.h>
#include <tms/core/backend.h>
#include <tms/math/matrix.h>

static int initialized = 0;
static struct tms_shader *shader   = 0;
static struct tms_program *program   = 0;
static struct tms_shader *sprite_shader   = 0;
static struct tms_program *sprite_program   = 0;
static struct tms_shader *rsprite_shader   = 0;
static struct tms_program *rsprite_program   = 0;
static struct tms_shader *sprite1c_shader   = 0;
static struct tms_program *sprite1c_program   = 0;
//static struct tms_shader *shader_textured   = 0;
static struct tms_mesh *square = 0;
static struct tms_mesh *lsquare = 0;
static struct tms_mesh *circle = 0;
static struct tms_mesh *line   = 0;
static struct tms_mesh *line3d   = 0;
static GLuint color_loc;
static GLuint color_sprite_loc;
static GLuint mvp_loc;
static GLuint mvp_sprite_loc;
static GLuint coords_loc;

static GLuint color_rsprite_loc;
static GLuint mvp_rsprite_loc;

static GLuint color_sprite1c_loc;
static GLuint mvp_sprite1c_loc;
static GLuint coords1c_loc;

static struct tms_mesh *rsquare = 0;
static struct tms_gbuffer *rsquare_buf = 0;
static struct tms_gbuffer *rsquare_ibuf = 0;

static struct tms_gbuffer *tmp_buf;

/* XXX */
struct tms_mesh *tms_ddraw_circle_mesh;

static const char *shader_src[] = {
"uniform mat4 MVP;"
"attribute vec3 position;"
"void main(void)"
"{"
    "gl_Position = MVP*vec4(position.xyz, 1.0);"
"}",

"uniform vec4 color;"
"void main(void)"
"{"
    "gl_FragColor=color;"
"}"
};

static const char *shader_sprite_src[] = {
"uniform mat4 MVP;"
"uniform vec4 coords;"
"attribute vec3 position;"
"attribute vec2 texcoord;"
"varying highp vec2 FS_texcoord;"
"void main(void)"
"{"
    "FS_texcoord = mix(coords.xy, coords.zw, texcoord.xy);"
    "gl_Position = MVP*vec4(position.xyz, 1.0);"
"}",

"uniform vec4 color;"
"uniform sampler2D sprite_0;"
"varying highp vec2 FS_texcoord;"
"void main(void)"
"{"
    "gl_FragColor=color * texture2D(sprite_0, FS_texcoord);"
"}"
};

static const char *shader_sprite1c_src[] = {
"uniform mat4 MVP;"
"uniform vec4 coords;"
"attribute vec3 position;"
"attribute vec2 texcoord;"
"varying highp vec2 FS_texcoord;"
"void main(void)"
"{"
    "FS_texcoord = mix(coords.xy, coords.zw, texcoord.xy);"
    "gl_Position = MVP*vec4(position.xyz, 1.0);"
"}",

"uniform vec4 color;"
"uniform sampler2D sprite_0;"
"varying highp vec2 FS_texcoord;"
"void main(void)"
"{"
    "gl_FragColor=vec4(color.rgb, texture2D(sprite_0, FS_texcoord).r*color.a);"
"}"
};

static const char *shader_rsprite_src[] = {
"uniform mat4 MVP;"
"attribute vec3 position;"
"attribute vec2 texcoord;"
"varying highp vec2 FS_texcoord;"
"void main(void)"
"{"
    "FS_texcoord = texcoord;"
    "gl_Position = MVP*vec4(position.xyz, 1.0);"
"}",

"uniform vec4 color;"
"uniform sampler2D sprite_0;"
"varying highp vec2 FS_texcoord;"
"void main(void)"
"{"
    "gl_FragColor=color * texture2D(sprite_0, FS_texcoord);"
"}"
};

static void
init_shaders(void)
{
    shader = tms_shader_alloc();
    tms_shader_compile(shader, GL_VERTEX_SHADER, shader_src[0]);
    tms_shader_compile(shader, GL_FRAGMENT_SHADER, shader_src[1]);

    sprite_shader = tms_shader_alloc();
    tms_shader_compile(sprite_shader, GL_VERTEX_SHADER, shader_sprite_src[0]);
    tms_shader_compile(sprite_shader, GL_FRAGMENT_SHADER, shader_sprite_src[1]);

    rsprite_shader = tms_shader_alloc();
    tms_shader_compile(rsprite_shader, GL_VERTEX_SHADER, shader_rsprite_src[0]);
    tms_shader_compile(rsprite_shader, GL_FRAGMENT_SHADER, shader_rsprite_src[1]);

    rsprite_program = tms_shader_get_program(rsprite_shader, TMS_NO_PIPELINE);
    color_rsprite_loc = tms_program_get_uniform(rsprite_program, "color");
    mvp_rsprite_loc = tms_program_get_uniform(rsprite_program, "MVP");

    sprite_program = tms_shader_get_program(sprite_shader, TMS_NO_PIPELINE);
    coords_loc = tms_program_get_uniform(sprite_program, "coords");
    color_sprite_loc = tms_program_get_uniform(sprite_program, "color");
    //tms_infof("coords loc %d", coords_loc);
    mvp_sprite_loc = tms_program_get_uniform(sprite_program, "MVP");

    program = tms_shader_get_program(shader, TMS_NO_PIPELINE);
    color_loc = tms_program_get_uniform(program, "color");
    mvp_loc = tms_program_get_uniform(program, "MVP");

    sprite1c_shader = tms_shader_alloc();
    tms_shader_compile(sprite1c_shader, GL_VERTEX_SHADER, shader_sprite1c_src[0]);
    tms_shader_compile(sprite1c_shader, GL_FRAGMENT_SHADER, shader_sprite1c_src[1]);

    sprite1c_program = tms_shader_get_program(sprite1c_shader, TMS_NO_PIPELINE);
    color_sprite1c_loc = tms_program_get_uniform(sprite1c_program, "color");
    mvp_sprite1c_loc = tms_program_get_uniform(sprite1c_program, "MVP");
    coords1c_loc = tms_program_get_uniform(sprite1c_program, "coords");

    //shader_textured = tms_get_shader("2d-texture");
}

static void
init_buffers(void)
{
    float v[] = {
        .5f, .5f,
        1, 1,
        -.5f, .5f,
        0, 1,
        -.5f, -.5f,
        0, 0,
        .5f, -.5f,
        1, 0,
    };

    struct tms_gbuffer *buf = tms_gbuffer_alloc_fill(v, sizeof(v));
    struct tms_varray *va = tms_varray_alloc(2);
    tms_varray_map_attribute(va, "position", 2, GL_FLOAT, buf);
    tms_varray_map_attribute(va, "texcoord", 2, GL_FLOAT, buf);
    tms_varray_upload_all(va);

    square = tms_mesh_alloc(va, 0);
    tms_mesh_set_primitive_type(square, TMS_TRIANGLE_FAN);
    tms_mesh_set_autofree_buffers(square, 1);

    buf = tms_gbuffer_alloc_fill(v, sizeof(v));
    va = tms_varray_alloc(2);
    tms_varray_map_attribute(va, "position", 2, GL_FLOAT, buf);
    tms_varray_map_attribute(va, "texcoord", 2, GL_FLOAT, buf);
    tms_varray_upload_all(va);

    lsquare = tms_mesh_alloc(va, 0);
    tms_mesh_set_primitive_type(lsquare, GL_LINE_LOOP);
    tms_mesh_set_autofree_buffers(lsquare, 1);

    float vs[64];
    float step = (M_PI*2.f)/32.f;
    for (int x=0; x<32; x++) {
        float a = x*step;
        vs[x*2] = cos(a);
        vs[x*2+1] = sin(a);
    }

    buf = tms_gbuffer_alloc_fill(vs, sizeof(vs));
    va = tms_varray_alloc(1);
    tms_varray_map_attribute(va, "position", 2, GL_FLOAT, buf);
    tms_varray_upload_all(va);

    circle = tms_mesh_alloc(va, 0);
    tms_mesh_set_primitive_type(circle, TMS_TRIANGLE_FAN);
    tms_mesh_set_autofree_buffers(circle, 1);

    tmp_buf = tms_gbuffer_alloc(3*2*sizeof(float));

    va = tms_varray_alloc(1);
    tms_varray_map_attribute(va, "position", 2, GL_FLOAT, tmp_buf);
    line = tms_mesh_alloc(va, 0);
    tms_mesh_set_primitive_type(line, TMS_LINES);

    va = tms_varray_alloc(1);
    tms_varray_map_attribute(va, "position", 3, GL_FLOAT, tmp_buf);
    line3d = tms_mesh_alloc(va, 0);
    tms_mesh_set_primitive_type(line3d, TMS_LINES);

    tms_ddraw_circle_mesh = circle;

    rsquare_ibuf = tms_gbuffer_alloc(18 * 3 * sizeof(uint16_t));

    uint16_t *i = (uint16_t*)tms_gbuffer_get_buffer(rsquare_ibuf);
    for (int iy=0; iy<3; ++iy) {
        for (int ix=0; ix<3; ++ix) {
            i[iy*3*6+ix*6+0] = ix + 1 + iy * 4;
            i[iy*3*6+ix*6+1] = ix + 0 + iy * 4;
            i[iy*3*6+ix*6+2] = ix + 4 + iy * 4;
            i[iy*3*6+ix*6+3] = ix + 1 + iy * 4;
            i[iy*3*6+ix*6+4] = ix + 4 + iy * 4;
            i[iy*3*6+ix*6+5] = ix + 5 + iy * 4;
        }
    }
    /*
    for (int x=0; x<9; ++x) {
        i[x*6+0] = (x*4)+1;
        i[x*6+1] = (x*4)+0;
        i[x*6+2] = (x*4)+4;
        i[x*6+3] = (x*4)+1;
        i[x*6+4] = (x*4)+4;
        i[x*6+5] = (x*4)+5;
        tms_debugf("[%d], %d - %d - %d",
                x,
                i[x*6+0],
                i[x*6+1],
                i[x*6+2]);
        tms_debugf("[%d], %d - %d - %d",
                x,
                i[x*6+3],
                i[x*6+4],
                i[x*6+5]);
    }
    */

    //tms_fatalf("x");

    tms_gbuffer_upload(rsquare_ibuf);

    rsquare_buf = tms_gbuffer_alloc(16*4*sizeof(float));
    va = tms_varray_alloc(2);
    tms_varray_map_attribute(va, "position", 2, GL_FLOAT, rsquare_buf);
    tms_varray_map_attribute(va, "texcoord", 2, GL_FLOAT, rsquare_buf);
    tms_varray_upload_all(va);

    rsquare = tms_mesh_alloc(va, rsquare_ibuf);
    //tms_mesh_set_primitive_type(rsquare, GL_POINTS);
    tms_mesh_set_autofree_buffers(rsquare, 1);
}

struct tms_ddraw*
tms_ddraw_alloc(void)
{
    struct tms_ddraw *d;

    if ((d = calloc(1, sizeof(struct tms_ddraw)))) {
        tms_ddraw_init(d);
    }

    return d;
}

void
tms_ddraw_init(struct tms_ddraw *d)
{
    tmat4_load_identity(d->modelview);
    tmat4_load_identity(d->projection);

    tmat4_load_identity(d->tmp_modelview);
    tmat4_load_identity(d->tmp_projection);

    if (!initialized) {
        init_shaders();
        init_buffers();
    }
}

void
tms_ddraw_set_color(struct tms_ddraw *d, float r, float g, float b, float a)
{
    tms_program_bind(program);
    glUniform4f(color_loc, r, g, b, a);
    tms_program_bind(sprite_program);
    glUniform4f(color_sprite_loc, r, g, b, a);

    d->color = (tvec4){r,g,b,a};
}

void
tms_ddraw_set_rsprite_color(struct tms_ddraw *d, float r, float g, float b, float a)
{
    tms_program_bind(rsprite_program);
    glUniform4f(color_rsprite_loc, r, g, b, a);
}

void
tms_ddraw_set_sprite1c_color(struct tms_ddraw *d, float r, float g, float b, float a)
{
    tms_program_bind(sprite1c_program);
    glUniform4f(color_sprite1c_loc, r, g, b, a);
}

void
tms_ddraw_set_matrices(struct tms_ddraw *d, float *mv, float *p)
{
    if (!mv)
        tmat4_load_identity(d->modelview);
    else
        tmat4_copy(d->modelview, mv);

    if (!p)
        tmat4_load_identity(d->projection);
    else
        tmat4_copy(d->projection, p);
}

/* horribly slow! */
int
tms_ddraw_line(struct tms_ddraw *d, float x1, float y1, float x2, float y2)
{
    float mat[16];
    tmat4_copy(mat, d->modelview);
    tmat4_multiply_reverse(mat, d->projection);

    float *buf = tms_gbuffer_get_buffer(tmp_buf);

    buf[0] = x1;
    buf[1] = y1;
    buf[2] = x2;
    buf[3] = y2;

    tms_gbuffer_upload_partial(tmp_buf, 4*sizeof(float));

    tms_program_bind(program);
    glUniformMatrix4fv(mvp_loc, 1, 0, mat);
    tms_mesh_render(line, program);

    return T_OK;
}

int
tms_ddraw_line3d(struct tms_ddraw *d, float x1, float y1, float z1,
                                      float x2, float y2, float z2)
{
    float mat[16];
    tmat4_copy(mat, d->modelview);
    tmat4_multiply_reverse(mat, d->projection);

    float *buf = tms_gbuffer_get_buffer(tmp_buf);

    buf[0] = x1;
    buf[1] = y1;
    buf[2] = z1;
    buf[3] = x2;
    buf[4] = y2;
    buf[5] = z2;

    tms_gbuffer_upload_partial(tmp_buf, 6*sizeof(float));

    tms_program_bind(program);
    glUniformMatrix4fv(mvp_loc, 1, 0, mat);
    tms_mesh_render(line3d, program);

    return T_OK;
}

int
tms_ddraw_square(struct tms_ddraw *d, float x, float y, float width, float height)
{
    float mat[16];
    tmat4_copy(mat, d->modelview);
    tmat4_translate(mat, x, y, 0);
    tmat4_scale(mat, width, height, 1);
    tmat4_multiply_reverse(mat, d->projection);

    tms_program_bind(program);
    glUniformMatrix4fv(mvp_loc, 1, 0, mat);
    tms_mesh_render(square, program);

    return T_OK;
}

int
tms_ddraw_square3d(struct tms_ddraw *d, float x, float y, float z, float width, float height)
{
    float mat[16];
    tmat4_copy(mat, d->modelview);
    tmat4_translate(mat, x, y, z);
    tmat4_scale(mat, width, height, 1);
    tmat4_multiply_reverse(mat, d->projection);

    tms_program_bind(program);
    glUniformMatrix4fv(mvp_loc, 1, 0, mat);
    tms_mesh_render(square, program);

    return T_OK;
}

int
tms_ddraw_sprite(struct tms_ddraw *d, struct tms_sprite *s,
                 float x, float y, float width, float height)
{
    float mat[16];
    tmat4_copy(mat, d->modelview);
    tmat4_translate(mat, x, y, 0);
    tmat4_scale(mat, width, height, 1);
    tmat4_multiply_reverse(mat, d->projection);

    tms_program_bind(sprite_program);
    glUniformMatrix4fv(mvp_sprite_loc, 1, 0, mat);
    glUniform4f(coords_loc, s->bl.x, s->bl.y, s->tr.x, s->tr.y);
    tms_mesh_render(square, sprite_program);

    return T_OK;
}

int
tms_ddraw_sprite1c(struct tms_ddraw *d, struct tms_sprite *s,
                   float x, float y, float width, float height)
{
    float mat[16];
    tmat4_copy(mat, d->modelview);
    tmat4_translate(mat, x, y, 0);
    tmat4_scale(mat, width, height, 1);
    tmat4_multiply_reverse(mat, d->projection);

    tms_program_bind(sprite1c_program);
    glUniformMatrix4fv(mvp_sprite1c_loc, 1, 0, mat);
    glUniform4f(coords1c_loc, s->bl.x, s->bl.y, s->tr.x, s->tr.y);
    tms_mesh_render(square, sprite1c_program);

    return T_OK;
}

int
tms_ddraw_sprite_r(struct tms_ddraw *d, struct tms_sprite *s,
                 float x, float y, float width, float height, float r)
{
    float mat[16];
    tmat4_copy(mat, d->modelview);
    tmat4_translate(mat, x, y, 0);
    tmat4_rotate(mat, r, 0, 0, 1);
    tmat4_scale(mat, width, height, 1);
    tmat4_multiply_reverse(mat, d->projection);

    tms_program_bind(sprite_program);
    glUniformMatrix4fv(mvp_sprite_loc, 1, 0, mat);
    glUniform4f(coords_loc, s->bl.x, s->bl.y, s->tr.x, s->tr.y);
    tms_mesh_render(square, sprite_program);

    return T_OK;
}

int
tms_ddraw_rsprite(struct tms_ddraw *d, struct tms_sprite *s,
        float x, float y,
        float width, float height, float border_outer)
{
    static const float border_inner = 0.f;
    const float w2 = width/2.f;
    const float h2 = height/2.f;
    float mat[16];
    tmat4_copy(mat, d->modelview);
    tmat4_translate(mat, x, y, 0);
    //tmat4_rotate(mat, 10.f * sin(_tms.last_time / 1000000.), 0.4, 0.4, 0.0);
    tmat4_multiply_reverse(mat, d->projection);

    float *buf = tms_gbuffer_get_buffer(rsquare_buf);

    for (int n=0; n<16; ++n) {
        if ((n % 4) == 0) {
            buf[n*4 + 0] = -border_outer - w2; /* pos x */
            buf[n*4 + 2] = s->bl.x; /* texcoord x */
        } else if ((n % 4) == 1) {
            buf[n*4 + 0] = -border_inner - w2; /* pos x */
            buf[n*4 + 2] = s->bl.x - (s->bl.x - s->tr.x) / 3.f; /* texcoord x */
        } else if ((n % 4) == 2) {
            buf[n*4 + 0] =  border_inner + w2; /* pos x */
            buf[n*4 + 2] = s->bl.x - (s->bl.x - s->tr.x) / 3.f * 2.f; /* texcoord x */
        } else if ((n % 4) == 3) {
            buf[n*4 + 0] =  border_outer + w2; /* pos x */
            buf[n*4 + 2] = s->tr.x; /* texcoord x */
        }

        if (n/4 == 0) {
            buf[n*4 + 1] =  border_outer + h2; /* pos y */
            buf[n*4 + 3] = s->tr.y; /* texcoord y */
        } else if (n/4 == 1) {
            buf[n*4 + 1] =  border_inner + h2; /* pos y */
            buf[n*4 + 3] = s->tr.y - (s->tr.y - s->bl.y) / 3.f; /* texcoord y */
        } else if (n/4 == 2) {
            buf[n*4 + 1] = -border_inner - h2; /* pos y */
            buf[n*4 + 3] = s->tr.y - (s->tr.y - s->bl.y) / 3.f * 2.f; /* texcoord y */
        } else if (n/4 == 3) {
            buf[n*4 + 1] = -border_outer - h2; /* pos y */
            buf[n*4 + 3] = s->bl.y; /* texcoord y */
        }
    }

    tms_gbuffer_upload(rsquare_buf);

    tms_program_bind(rsprite_program);
    glUniformMatrix4fv(mvp_rsprite_loc, 1, 0, mat);
    tms_mesh_render(rsquare, rsprite_program);

    return T_OK;
}

int
tms_ddraw_square_textured(struct tms_ddraw *d,
                          float x, float y,
                          float width, float height,
                          struct tms_texture *texture)
{
    /*
    tmat4_copy(d->tmp_modelview, d->modelview);
    tmat4_copy(d->tmp_projection, d->projection);
    tmat4_translate(d->tmp_modelview, x, y, 0);
    tmat4_scale(d->tmp_modelview, width, height, 1);

    tms_texture_bind(texture);

    tms_shader_bind(shader_textured);
    tms_shader_set_matrices(shader_textured, d->tmp_modelview, d->tmp_projection);
    tms_mesh_render(square, shader_textured);

    */
    return T_OK;
}

int
tms_ddraw_lcircle(struct tms_ddraw *d, float x, float y, float width, float height)
{
    float mat[16];
    tms_mesh_set_primitive_type(circle, GL_LINE_LOOP);
    tmat4_copy(mat, d->modelview);
    tmat4_translate(mat, x, y, 0);
    tmat4_scale(mat, width, height, 1);
    tmat4_multiply_reverse(mat, d->projection);

    tms_program_bind(program);
    glUniformMatrix4fv(mvp_loc, 1, 0, mat);
    tms_mesh_render(circle, program);

    return T_OK;
}

int
tms_ddraw_circle(struct tms_ddraw *d, float x, float y, float width, float height)
{
    float mat[16];
    tms_mesh_set_primitive_type(circle, TMS_TRIANGLE_FAN);
    tmat4_copy(mat, d->modelview);
    tmat4_translate(mat, x, y, 0);
    tmat4_scale(mat, width, height, 1);
    tmat4_multiply_reverse(mat, d->projection);

    tms_program_bind(program);
    glUniformMatrix4fv(mvp_loc, 1, 0, mat);
    tms_mesh_render(circle, program);

    return T_OK;
}

int
tms_ddraw_lcircle3d(struct tms_ddraw *d, float x, float y, float z, float width, float height)
{
    float mat[16];
    tms_mesh_set_primitive_type(circle, GL_LINE_LOOP);
    tmat4_copy(mat, d->modelview);
    tmat4_translate(mat, x, y, z);
    tmat4_scale(mat, width, height, 1);
    tmat4_multiply_reverse(mat, d->projection);

    tms_program_bind(program);
    glUniformMatrix4fv(mvp_loc, 1, 0, mat);
    tms_mesh_render(circle, program);

    return T_OK;
}

int
tms_ddraw_lsquare(struct tms_ddraw *d, float x, float y, float width, float height)
{
    float mat[16];
    tmat4_copy(mat, d->modelview);
    tmat4_translate(mat, x, y, 0);
    tmat4_scale(mat, width, height, 1);
    tmat4_multiply_reverse(mat, d->projection);

    tms_program_bind(program);
    glUniformMatrix4fv(mvp_loc, 1, 0, mat);
    tms_mesh_render(lsquare, program);

    return T_OK;
}

int
tms_ddraw_lsquare3d(struct tms_ddraw *d, float x, float y, float z, float width, float height)
{
    float mat[16];
    tmat4_copy(mat, d->modelview);
    tmat4_translate(mat, x, y, z);
    tmat4_scale(mat, width, height, 1);
    tmat4_multiply_reverse(mat, d->projection);

    tms_program_bind(program);
    glUniformMatrix4fv(mvp_loc, 1, 0, mat);
    tms_mesh_render(lsquare, program);

    return T_OK;
}
