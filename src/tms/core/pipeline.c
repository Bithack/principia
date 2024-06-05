#include "pipeline.h"
#include "framebuffer.h"
#include "shader.h"
#include "err.h"
#include <tms/backend/opengl.h>

#pragma GCC push_options
#pragma GCC optimize ("O0")

static struct tms_pipeline pipelines[TMS_NUM_PIPELINES] = {0};
static int n_local_uniforms = 0;

#if defined(TMS_BACKEND_PC) || !defined(TMS_BACKEND_LINUX_SS)
// Some things in the pipeline are different for Windows/Linux as compared to Android (GL/GLES differences? dunno)
// The desktop pipeline is the "cool" pipeline as decided by me.
#define TMS_COOL_PIPELINE
#endif

#ifdef TMS_COOL_PIPELINE

typedef void (*TMS_UNIFORM_FN)(GLint, GLsizei, const GLfloat*);
typedef void (*TMS_UNIFORM_MAT_FN)(GLint, GLsizei, GLboolean, const GLfloat*);

TMS_UNIFORM_FN uniform_fn[7];
TMS_UNIFORM_MAT_FN uniform_mat_fn[5];

#else

void (*uniform_fn[])(GLint, GLsizei, void *) = {
    0,
    glUniform4fv,
    glUniform3fv,
    glUniform1iv,
    glUniform2fv,
    glUniform1fv,
    glUniform1iv,
};

void (*uniform_mat_fn[])(GLint, GLsizei, GLboolean, void *) = {
    glUniformMatrix4fv,
    glUniformMatrix3fv,
    glUniformMatrix4fv,
    glUniformMatrix4fv,
    glUniformMatrix4fv,
};

#endif

struct tms_pipeline *tms_get_pipeline(int num)
{
    tms_assertf(num < TMS_NUM_PIPELINES, "%d > TMS_NUM_PIPELINES (%d)", num, TMS_NUM_PIPELINES);

    return &pipelines[num];
}

void
tms_pipeline_init()
{
#ifdef TMS_COOL_PIPELINE
    uniform_fn[0] = 0;
    uniform_fn[1] = glUniform4fv;
    uniform_fn[2] = glUniform3fv;
    uniform_fn[3] = (TMS_UNIFORM_FN)glUniform1iv;
    uniform_fn[4] = glUniform2fv;
    uniform_fn[5] = glUniform1fv;
    uniform_fn[6] = (TMS_UNIFORM_FN)glUniform1iv;

    uniform_mat_fn[0] = glUniformMatrix4fv;
    uniform_mat_fn[1] = glUniformMatrix3fv;
    uniform_mat_fn[2] = glUniformMatrix4fv;
    uniform_mat_fn[3] = glUniformMatrix4fv;
    uniform_mat_fn[4] = glUniformMatrix4fv;
#endif
}

void
tms_pipeline_set_framebuffer(int p, struct tms_fb *fb)
{
    pipelines[p].fb = fb;
}

struct tms_fb*
tms_pipeline_get_framebuffer(int p)
{
    return pipelines[p].fb;
}

void
tms_pipeline_set_begin_fn(int p, void (*fn)(void))
{
    pipelines[p].begin_fn = fn;
}

void
tms_pipeline_set_end_fn(int p, void (*fn)(void))
{
    pipelines[p].end_fn = fn;
}

void
tms_pipeline_begin_render(int p)
{
    n_local_uniforms = 0;

    //glFinish();
    //Uint32 t = SDL_GetTicks();

    if (pipelines[p].fb)
        tms_fb_bind(pipelines[p].fb);

    //glFinish();
    //tms_infof("bind time: %u", SDL_GetTicks()-t);

    if (pipelines[p].begin_fn != 0)
        pipelines[p].begin_fn();
}


void
tms_pipeline_end_render(int p)
{
    if (pipelines[p].end_fn != 0)
        pipelines[p].end_fn();

    //glFinish();
    //Uint32 t = SDL_GetTicks();

    if (pipelines[p].fb)
        tms_fb_unbind(pipelines[p].fb);

    //glFinish();
    //tms_infof("unbind time: %u", SDL_GetTicks()-t);
}

void
tms_pipeline_set_uniform1i(int pipeline, const char *name, int val)
{
    struct tms_pipeline *p = &pipelines[pipeline];

    p->constants[p->num_constants].name = name;
    p->constants[p->num_constants].type = TMS_INT;
    p->constants[p->num_constants].val.i = val;
    p->num_constants ++;
}

void
tms_pipeline_apply_combined_uniforms(int p,
        struct tms_rstate *state,
        struct tms_program *s, struct tms_entity *e)
{
    float mat[16];
    GLuint *locs = s->p_combined;
    for (int x=0; x<pipelines[p].num_combined; x++) {
        GLuint loc = locs[x];

        if (loc == -1)
            continue;

#ifdef TMS_COOL_PIPELINE
        if (pipelines[p].combined[x].type1 == TMS_MAT4 && pipelines[p].combined[x].type2 == TMS_MAT4) {

            float *m1 = (float*)(((char*)(state->data)) + pipelines[p].combined[x].offs1);
            float *m2 = (float*)(((char*)(e)) + pipelines[p].combined[x].offs2);

            tmat4_copy(mat, m1);
            tmat4_multiply(mat, m2);

            glUniformMatrix4fv(loc, 1, 0, mat);
        }
#else
        struct uniform_combined *u = &pipelines[p].combined[x];

        if (u->type1 == TMS_MAT4 && u->type2 == TMS_MAT4) {

            float *m1 = (float*)(((char*)(state->data)) + u->offs1);
            float *m2 = (float*)(((char*)(e)) + u->offs2);

            tmat4_copy(mat, m1);
            tmat4_multiply(mat, m2);

            glUniformMatrix4fv(loc, 1, 0, mat);
        }
#endif
    }
}

void apply_global_uniform(struct tms_pipeline *p, int x, struct tms_rstate *state, GLuint loc)
{
    if (loc == -1) {
        return;
    }

    if (p->global[x].type == TMS_P) {
        glUniformMatrix4fv(loc, 1, 0, state->projection);
    } else if (p->global[x].type >= 128)
        (uniform_mat_fn[p->global[x].type - 128])(loc, 1, 0, (const GLfloat *)(((char*)(state->data))+p->global[x].offs));
    else {
        (uniform_fn[p->global[x].type])(loc, 1, (const GLfloat *)(((char*)(state->data))+p->global[x].offs));
    }

    return;
}

void
tms_pipeline_apply_global_uniforms(int p,
        struct tms_rstate *state,
        struct tms_program *s)
{
#ifdef TMS_COOL_PIPELINE

    GLuint loc;
    int x = pipelines[p].num_global - 1;

    struct tms_pipeline *pl = tms_get_pipeline(p);

    while (x >= 0) {
        loc = s->p_global[x];
        apply_global_uniform(pl, x, state, loc);
        x--;
    }

#else
    GLuint *locs = s->p_global;
    for (int x=0; x<pipelines[p].num_global; x++) {
        GLuint loc = locs[x];

        if (loc == -1)
            continue;

        struct uniform *u = &pipelines[p].global[x];

        if (u->type == TMS_P) {
            glUniformMatrix4fv(loc, 1, 0, state->projection);
        } else if (u->type >= 128)
            (uniform_mat_fn[u->type - 128])(loc, 1, 0, ((char*)(state->data))+u->offs);
        else {
            (uniform_fn[u->type])(loc, 1, ((char*)(state->data))+u->offs);
        }
    }
#endif
}

void
tms_pipeline_apply_local_uniforms(int p,
        struct tms_rstate *state,
        struct tms_program *s,
        struct tms_entity *e)
{
#ifdef TMS_COOL_PIPELINE
    for (int x=0; x<pipelines[p].num_local; x++) {
        GLuint loc = s->p_local[x];
#else
    GLuint *locs = s->p_local;

    for (int x=0; x<pipelines[p].num_local; x++) {
        GLuint loc = locs[x];
#endif

        if (loc == -1)
            continue;

        struct uniform *u = &pipelines[p].local[x];
        //tms_infof("applying %s", u->name);

        n_local_uniforms ++;

        if (u->type == TMS_MV) {
            glUniformMatrix4fv(loc, 1, 0, state->modelview);
        } else if (u->type == TMS_MVP) {
            float tmp[16];
            tmat4_copy(tmp, state->projection);
            tmat4_multiply(tmp, state->modelview);
            glUniformMatrix4fv(loc, 1, 0, tmp);

        } else if (u->type >= 128) {
#ifdef TMS_COOL_PIPELINE
            if (u->type == TMS_MAT3)
                glUniformMatrix3fv(loc, 1, 0, (const GLfloat *)((char *)(e) + (u->offs)));
            else
                glUniformMatrix4fv(loc, 1, 0, (const GLfloat *)((char *)(e) + (u->offs)));
#else
            (uniform_mat_fn[u->type - 128])(loc, 1, 0, (const GLfloat *)((char *)(e) + (u->offs)));
#endif
        } else
            (uniform_fn[u->type])(loc, 1, (const GLfloat *)((char*)(e))+u->offs);
    }
}

void
tms_pipeline_declare(int pipeline, const char *name, int type, uintptr_t offs)
{
    tms_assertf(pipeline < TMS_NUM_PIPELINES, "%d > TMS_NUM_PIPELINES (%d)", pipeline, TMS_NUM_PIPELINES);

    struct tms_pipeline *p = &pipelines[pipeline];

    p->local[p->num_local].name = name;
    p->local[p->num_local].type = type;
    p->local[p->num_local].offs = offs;

    p->num_local ++;
}

void
tms_pipeline_declare_global(int pipeline, const char *name, int type, uintptr_t offs)
{
    tms_assertf(pipeline < TMS_NUM_PIPELINES, "%d > TMS_NUM_PIPELINES (%d)", pipeline, TMS_NUM_PIPELINES);

    struct tms_pipeline *p = &pipelines[pipeline];

    p->global[p->num_global].name = name;
    p->global[p->num_global].type = type;
    p->global[p->num_global].offs = offs;

    p->num_global ++;
}

void
tms_pipeline_declare_combined(int pipeline, const char *name, int type1, uintptr_t offs1, int type2, uintptr_t offs2)
{
    tms_assertf(pipeline < TMS_NUM_PIPELINES, "%d > TMS_NUM_PIPELINES (%d)", pipeline, TMS_NUM_PIPELINES);

    struct tms_pipeline *p = &pipelines[pipeline];

    p->combined[p->num_combined].name = name;
    p->combined[p->num_combined].type1 = type1;
    p->combined[p->num_combined].offs1 = offs1;
    p->combined[p->num_combined].type2 = type2;
    p->combined[p->num_combined].offs2 = offs2;

    p->num_combined ++;
}

#pragma GCC pop_options
