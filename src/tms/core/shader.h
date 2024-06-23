#ifndef _TMS_SHADER__H_
#define _TMS_SHADER__H_

/** @relates tms_shader @{ */

#include <tms/backend/opengl.h>
#include <tms/math/vector.h>
#include <tms/core/tms.h>

#ifdef TMS_USE_GLES
#define TMS_GLSL_HEADER\
    "#version 100\n"\
    "precision mediump float;"\
    "precision mediump int;"\
"\n"
#else
#define TMS_GLSL_HEADER "#version 110\n"
#endif

struct tms_mesh_attribute;

struct tms_shader_pairing_val {
#if defined(__clang__) || defined(_MSC_VER)
    int dummy_124;
#endif
};

/**
 * Pairing with a mesh
 **/
struct tms_shader_pairing {
    struct tms_shader_pairing_val attributes;
};

/**
 * A shader object
 **/
struct tms_shader {
    char *name;

    int num_defines;
    char **defines;

    int num_fs_defines;
    char **fs_defines;

    int num_vs_defines;
    char **vs_defines;

    GLuint vertex;
    GLuint fragment;
    GLuint tess_control;
    GLuint tess_eval;
    GLuint geometry;

    struct tms_program *program;
    struct tms_program *pipeline_program[TMS_NUM_PIPELINES];
};

extern struct tms_shader _tms_global_shader;

struct tms_shader *tms_shader_alloc(void);
void tms_shader_init(struct tms_shader *s);
void tms_shader_define(struct tms_shader *s, const char *name, const char *value);
void tms_shader_define_fs(struct tms_shader *s, const char *name, const char *value);
void tms_shader_define_vs(struct tms_shader *s, const char *name, const char *value);
int tms_shader_compile(struct tms_shader *s, GLenum type, const char *source);
struct tms_program *tms_shader_get_program(struct tms_shader *s, int pipeline);
void tms_shader_free(struct tms_shader *s);
void tms_shader_uninit(struct tms_shader *s);

static inline void
tms_shader_global_clear_defines(void)
{
    _tms_global_shader.num_defines = 0;
    _tms_global_shader.num_vs_defines = 0;
    _tms_global_shader.num_fs_defines = 0;
    if (_tms_global_shader.defines) {free(_tms_global_shader.defines);_tms_global_shader.defines = 0;}
    if (_tms_global_shader.vs_defines) {free(_tms_global_shader.vs_defines);_tms_global_shader.vs_defines = 0;}
    if (_tms_global_shader.fs_defines) {free(_tms_global_shader.fs_defines);_tms_global_shader.fs_defines = 0;}
}

static inline void
tms_shader_global_define_vs(const char *name, const char *value)
{
    tms_shader_define_vs(&_tms_global_shader,name,value);
}

static inline void
tms_shader_global_define_fs(const char *name, const char *value)
{
    tms_shader_define_fs(&_tms_global_shader,name,value);
}

static inline void
tms_shader_global_define(const char *name, const char *value)
{
    tms_shader_define(&_tms_global_shader,name,value);
}
#endif

