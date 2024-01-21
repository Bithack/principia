#ifndef _TMS_PROGRAM__H_
#define _TMS_PROGRAM__H_

#include <tms/backend/opengl.h>

struct tms_entity;

/**
 * A compiled shader, possibly associated with a pipeline
 **/
struct tms_program {
    struct tms_shader *parent;

    GLuint id;
    int pipeline;

    int num_attributes;

    struct tms_uniform *uniforms;
    unsigned num_uniforms;

    GLuint *p_local;
    GLuint *p_global;
    GLuint *p_combined;

    struct thash *locs_tbl;
    struct tms_varray *last_va;
    int *last_locations;
};

struct tms_uniform {
    char *name;
    int type;
    GLint loc;
};

struct tms_uniform_val {
    char *name;
    GLint loc[TMS_NUM_PIPELINES];
    int type;
    union {
        tvec4 vec4;
        tvec2 vec2;
        float vec1;
        void (*fn)(struct tms_entity *e, int location);
    } val;
};

void tms_program_init(struct tms_program *p);
void tms_program_free(struct tms_program *p);
GLuint tms_program_get_uniform(struct tms_program *p, const char *name);
void tms_program_load_uniforms(struct tms_program *p);
int* tms_program_get_locations(struct tms_program *s, struct tms_varray *va);
void tms_program_load_attributes(struct tms_program *p);

static inline void
tms_program_bind(struct tms_program *p)
{
    glUseProgram(p->id);
}

#endif
