#include "pipeline.h"
#include "framebuffer.h"
#include <tms/backend/opengl.h>

static struct tms_pipeline pipelines[TMS_NUM_PIPELINES] = {0};
static int n_local_uniforms = 0;

typedef void (*TMS_UNIFORM_FN)(GLint, GLsizei, const GLfloat*);
typedef void (*TMS_UNIFORM_MAT_FN)(GLint, GLsizei, GLboolean, const GLfloat*);

TMS_UNIFORM_FN uniform_fn[7];
TMS_UNIFORM_MAT_FN uniform_mat_fn[5];

struct tms_pipeline *tms_get_pipeline(int num)
{
    tms_assertf(num < TMS_NUM_PIPELINES, "%d > TMS_NUM_PIPELINES (%d)", num, TMS_NUM_PIPELINES);

    return &pipelines[num];
}

void
tms_pipeline_init()
{
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

    if (pipelines[p].fb)
        tms_fb_bind(pipelines[p].fb);

    if (pipelines[p].begin_fn != 0)
        pipelines[p].begin_fn();
}


void
tms_pipeline_end_render(int p)
{
    if (pipelines[p].end_fn != 0)
        pipelines[p].end_fn();

    if (pipelines[p].fb)
        tms_fb_unbind(pipelines[p].fb);
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

        struct uniform_combined *u = &pipelines[p].combined[x];

        if (u->type1 == TMS_MAT4 && u->type2 == TMS_MAT4) {
            float *m1 = (float*)(((char*)(state->data)) + u->offs1);
            float *m2 = (float*)(((char*)(e)) + u->offs2);

            tmat4_copy(mat, m1);
            tmat4_multiply(mat, m2);

            glUniformMatrix4fv(loc, 1, 0, mat);
        }
    }
}

void apply_global_uniform(struct tms_pipeline *p, int x, struct tms_rstate *state, GLuint loc)
{
    if (loc == -1) {
        return;
    }

    struct uniform *u = &p->global[x];

    if (u->type == TMS_P)
        glUniformMatrix4fv(loc, 1, 0, state->projection);
    else if (u->type >= 128)
        (uniform_mat_fn[u->type - 128])(loc, 1, 0, (const GLfloat *)(((char*)(state->data))+u->offs));
    else
        (uniform_fn[u->type])(loc, 1, (const GLfloat *)(((char*)(state->data))+u->offs));
}

void
tms_pipeline_apply_global_uniforms(int p,
        struct tms_rstate *state,
        struct tms_program *s)
{
    struct tms_pipeline *pl = tms_get_pipeline(p);
    GLuint *locs = s->p_global;
    for (int x=0; x<pipelines[p].num_global; x++) {
        GLuint loc = locs[x];

        apply_global_uniform(pl, x, state, loc);
    }
}

void
tms_pipeline_apply_local_uniforms(int p,
        struct tms_rstate *state,
        struct tms_program *s,
        struct tms_entity *e)
{
    GLuint *locs = s->p_local;

    for (int x=0; x<pipelines[p].num_local; x++) {
        GLuint loc = locs[x];

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

        } else if (u->type >= 128)
            (uniform_mat_fn[u->type - 128])(loc, 1, 0, (const GLfloat *)((char *)(e) + (u->offs)));
        else
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
