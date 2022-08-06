#ifndef _PIPELINE__H_
#define _PIPELINE__H_

#include <stdarg.h>
#include <tms/core/tms.h>
#include <tms/core/program.h>

#define TMS_NO_PIPELINE -1
#define MAX_UNIFORMS 128

#define TMS_VEC4  1
#define TMS_VEC3  2
#define TMS_TEX2D 3
#define TMS_VEC2  4
#define TMS_FLOAT 5
#define TMS_INT   6
#define TMS_UNIFORM_CUSTOM   7

#define TMS_MAT4  128
#define TMS_MAT3  129
#define TMS_MV    130
#define TMS_MVP   131
#define TMS_P     132

struct tms_graph;
struct tms_program;
struct tms_entity;

struct tms_rstate {
    int p;

    struct tms_graph *graph;
    struct tms_program *program; /* current shader */

    /* these two will be static throughout the render process,
     * taken from camera */
    float view[16];
    float projection[16];

    /* updated per render call */
    float modelview[16];

    void *data;

    int active_tex;

    struct tms_varray *last_va;
    int               *last_loc;

    int (*render_entities_fn)(struct tms_rstate *state, struct tms_entity **ee, int count);
    int (**sort_fns)(void*, void*);
};

struct uniform {
    const char *name;
    int         type;
    uintptr_t   offs;
};

struct uniform_constant {
    const char *name;
    int type;
    union {
        tvec3 vec3;
        tvec2 vec2;
        float vec1;
        int i;
    } val;
};

struct uniform_combined {
    const char *name;
    int         type1;
    uintptr_t   offs1;
    int         type2;
    uintptr_t   offs2;

    int out_type;
};

static struct tms_pipeline {
    struct uniform local[MAX_UNIFORMS];
    struct uniform global[MAX_UNIFORMS];
    struct uniform_combined combined[MAX_UNIFORMS];
    struct uniform_constant constants[MAX_UNIFORMS];

    struct tms_fb *fb;

    void (*begin_fn)(void);
    void (*end_fn)(void);

    int num_local, num_global, num_combined, num_constants;
} pipelines[TMS_NUM_PIPELINES];

struct tms_pipeline *tms_get_pipeline(int num);
void tms_pipeline_init();
void tms_pipeline_set_begin_fn(int p, void (*fn)(void));
void tms_pipeline_set_end_fn(int p, void (*fn)(void));
void tms_pipeline_declare(int pipeline, const char *name, int type, uintptr_t offs);
void tms_pipeline_declare_global(int pipeline, const char *name, int type, uintptr_t offs);
void tms_pipeline_declare_combined(int pipeline, const char *name, int type, uintptr_t offs, int type2, uintptr_t offs2);

void tms_pipeline_apply_global_uniforms(int p, struct tms_rstate *state, struct tms_program *s);
void tms_pipeline_apply_local_uniforms(int p, struct tms_rstate *state, struct tms_program *s, struct tms_entity *e);
void tms_pipeline_apply_combined_uniforms(int p, struct tms_rstate *state, struct tms_program *s, struct tms_entity *e);

void tms_pipeline_begin_render(int p);
void tms_pipeline_end_render(int p);

void tms_pipeline_set_begin_fn(int p, void (*fn)(void));
void tms_pipeline_set_end_fn(int p, void (*fn)(void));

void tms_pipeline_set_uniform1i(int pipeline, const char *name, int val);

void tms_pipeline_set_framebuffer(int p, struct tms_fb *fb);
struct tms_fb* tms_pipeline_get_framebuffer(int p);

#endif
