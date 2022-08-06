#include <assert.h>

#include "transfade.h"
#include "tms.h"
#include "framebuffer.h"
#include "shader.h"
#include "backend_spec.h"

T TMOD_transfade_init(T_context *ctx);
T TMOD_transfade_begin(T_screen *old, T_screen *new);
T TMOD_transfade_render(long time);
T TMOD_transfade_end(void);

T_transition
TMOD_transfade =
{
    .begin = &TMOD_transfade_begin,
    .render = &TMOD_transfade_render,
    .end = &TMOD_transfade_end,
};

T_framebuffer *framebuffer;
T_screen      *old;
T_screen      *new;
T_shader      *shader;
T_shader      *mesh;
int           initialized = 0;
GLuint        alpha_loc;
float         fade = 0;

T
TMOD_transfade_init(T_context *ctx)
{
    framebuffer = T_framebuffer_alloc(ctx, ctx->width, ctx->height, 24);
    shader = T_shader_load("default+texturedtransp");

    mesh = T_mesh_alloc(2, 0);
    T_mesh_set_shader(mesh, shader);

    alpha_loc = T_shader_get_uniform(shader, "alpha");

    float position[] = {
        1.f, 1.f, 0,
        -1.f, 1.f, 0,
        -1.f, -1.f, 0,
        1.f, -1.f, 0,
    };
    float texcoords[] = {
        1.f, 1.f,
        0.f, 1.f,
        0.f, 0.f,
        1.f, 0.f,
    };

    T_mesh_set_attribute(mesh, 0, "position", position, 3, 4, GL_STATIC_DRAW);
    T_mesh_set_attribute(mesh, 1, "texcoord", texcoords, 2, 4, GL_STATIC_DRAW);
    T_mesh_upload(mesh);

    assert(framebuffer != 0);
    initialized = 1;
    return T_OK;
}

T
TMOD_transfade_begin(T_screen *o, T_screen *n)
{
    assert(initialized == 1);
    
    fade = 0;
    old = o;
    new = n;
    return T_OK;
}

T
TMOD_transfade_render(long time)
{
    fade += (float)time/500.f;

    if (fade > 1.f)
        fade = 1;

    glDisable(GL_BLEND);

    T_framebuffer_bind(framebuffer);
    new->spec->render(new);
    T_framebuffer_unbind(framebuffer);

    old->spec->render(old);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, framebuffer->fbo_texture);

    T_shader_bind(shader);

    glUniform1fv(alpha_loc, 1, &fade);
    
    float m[16] = TMATH_MAT4_IDENTITY;
    T_shader_set_matrices(shader, m, m);
    T_mesh_render(mesh);

    if (fade >= 1)
        return T_OK;

    return T_CONT;
}

T
TMOD_transfade_end(void)
{
    return T_OK;
}
