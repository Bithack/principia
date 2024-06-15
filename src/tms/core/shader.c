#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <tms/math/matrix.h>
#include <tms/core/err.h>
#include <tms/core/shader.h>
#include <tms/core/program.h>
#include <tms/core/pipeline.h>

#include <tms/backend/opengl.h>

#define INFOLOG_SIZE 2048

static const char *unknown = "<unknown>";
static char infolog[INFOLOG_SIZE];
static GLsizei infolog_length;

/* dummy shader used for global defines */
struct tms_shader _tms_global_shader = {0};

struct tms_shader*
tms_shader_alloc(void)
{
    struct tms_shader *s;
    (s = calloc(1, sizeof(struct tms_shader))) || tms_fatalf("out of mem (sh_a)");

    if (s) tms_shader_init(s);

    return s;
}

void
tms_shader_init(struct tms_shader *s)
{
    memset(s, 0, sizeof(struct tms_shader));
    s->name = (char *)unknown;
    /*
    s->vertex = 0;
    s->fragment = 0;
    s->tess_control = 0;
    s->tess_eval = 0;
    s->geometry = 0;

    s->num_defines = 0;
    s->defines = 0;
    s->num_fs_defines = 0;
    s->fs_defines = 0;
    s->num_vs_defines = 0;
    s->vs_defines = 0;
    */
}

void
tms_shader_uninit(struct tms_shader *s)
{
    //if (s->name != unknown)
        //free(s->name);

    for (int x=0; x<s->num_defines; x++) {
        free(s->defines[x]);
    }
    for (int x=0; x<s->num_fs_defines; x++) {
        free(s->fs_defines[x]);
    }
    for (int x=0; x<s->num_vs_defines; x++) {
        free(s->vs_defines[x]);
    }

    if (s->defines) free(s->defines);
    if (s->fs_defines) free(s->fs_defines);
    if (s->vs_defines) free(s->vs_defines);

    if (s->program) tms_program_free(s->program);
    for (int x=0; x<TMS_NUM_PIPELINES; x++) {
        if (s->pipeline_program[x]) tms_program_free(s->pipeline_program[x]);
    }

    if (s->vertex) glDeleteShader(s->vertex);
    if (s->fragment) glDeleteShader(s->fragment);
}

void
tms_shader_free(struct tms_shader *s)
{
    tms_shader_uninit(s);
    free(s);
}

void
tms_shader_define_vs(struct tms_shader *s, const char *name, const char *value)
{
    s->vs_defines = realloc(s->vs_defines, (s->num_vs_defines+1)*sizeof(char*));
    s->vs_defines[s->num_vs_defines] = malloc(strlen(name)+strlen(value)+strlen("#define  \n")+1);
    sprintf(s->vs_defines[s->num_vs_defines], "#define %s %s\n", name, value);

    s->num_vs_defines++;
}

void
tms_shader_define_fs(struct tms_shader *s, const char *name, const char *value)
{
    s->fs_defines = realloc(s->fs_defines, (s->num_fs_defines+1)*sizeof(char*));
    s->fs_defines[s->num_fs_defines] = malloc(strlen(name)+strlen(value)+strlen("#define  \n")+1);
    sprintf(s->fs_defines[s->num_fs_defines], "#define %s %s\n", name, value);

    s->num_fs_defines++;
}

void
tms_shader_define(struct tms_shader *s, const char *name, const char *value)
{
    s->defines = realloc(s->defines, (s->num_defines+1)*sizeof(char*));
    s->defines[s->num_defines] = malloc(strlen(name)+strlen(value)+strlen("#define   \n")+1);
    sprintf(s->defines[s->num_defines], "#define %s %s\n", name, value);

    s->num_defines++;
}

static GLint compile(struct tms_shader *sh, GLenum st, const char *src)
{
    int ierr = glGetError();
        tms_assertf(ierr == 0, "vafan compile -1 tjena %d", ierr);
    GLint s = glCreateShader(st);
    GLint success;
    const char *type = st == GL_VERTEX_SHADER ? "vertex shader"
#ifndef TMS_USE_GLES
        : st == GL_TESS_CONTROL_SHADER ? "tessellation control shader"
        : st == GL_TESS_EVALUATION_SHADER ? "tessellation evaluation shader"
        : st == GL_GEOMETRY_SHADER ? "geometry shader"
#endif
        : "fragment shader";

    int num_src = sh->num_defines + _tms_global_shader.num_defines + 1;

    if (st == GL_VERTEX_SHADER)
        num_src += sh->num_vs_defines + _tms_global_shader.num_vs_defines;
    else if (st == GL_FRAGMENT_SHADER)
        num_src += sh->num_fs_defines + _tms_global_shader.num_fs_defines;

    char *sources[num_src+1];
    sources[0] = TMS_GLSL_HEADER;

    for (int x=0; x<sh->num_defines; x++)
        sources[1+x] = sh->defines[x];

    for (int x=0; x<_tms_global_shader.num_defines; x++)
        sources[1+x+sh->num_defines] = _tms_global_shader.defines[x];

    if (st == GL_VERTEX_SHADER) {
        for (int x=0; x<sh->num_vs_defines; x++)
            sources[1+sh->num_defines+_tms_global_shader.num_defines+x] = sh->vs_defines[x];

        for (int x=0; x<_tms_global_shader.num_vs_defines; x++)
            sources[1+sh->num_defines+sh->num_vs_defines+_tms_global_shader.num_defines+x] = _tms_global_shader.vs_defines[x];

    } else if (st == GL_FRAGMENT_SHADER) {
        for (int x=0; x<sh->num_fs_defines; x++)
            sources[1+sh->num_defines+_tms_global_shader.num_defines+x] = sh->fs_defines[x];

        for (int x=0; x<_tms_global_shader.num_fs_defines; x++)
            sources[1+sh->num_defines+sh->num_fs_defines+_tms_global_shader.num_defines+x] = _tms_global_shader.fs_defines[x];
    }

    sources[num_src] = (char *)src;

    /*
    tms_infof("-- shader compile (%s)", type);
    for (int x=0; x<num_src+1; x++)
        tms_infof("%s", sources[x]);
    tms_infof("--");
    */

    tms_assertf(s != -1, "glCreateShader() failed");

    glShaderSource(s, num_src+1, (const GLchar *const *)sources, 0);
    glCompileShader(s);

    glGetShaderiv(s, GL_COMPILE_STATUS, &success);
    if (!success) {
        /* save the infolog, dev can fetch it using tms_shader_get_error_string() */
        glGetShaderInfoLog(s, INFOLOG_SIZE-1, &infolog_length, infolog);
        glDeleteShader(s);
        s = -1;
        tms_errorf("error compiling %s: %s", type, infolog);
    }

    tms_assertf(glGetError() == 0, "vafan compile 1 tjena");

    return s;
}

/**
 * Link the shaders into a program and return a
 * handle. Set pipeline to TMS_NO_PIPELINE to not
 * associate the shader with any specific pipeline.
 * If the shader is not associated with a pipeline,
 * all uniforms have to manually be set. Otherwise,
 * the uniforms will be set by the pipeline provided
 * that they have been declared in the pipeline.
 *
 * If the shader will be used with multiple pipelines,
 * the shader should be compiled multiple times.
 *
 * If the shader has been linked before, the old program
 * is returned.
 **/
struct tms_program *
tms_shader_get_program(struct tms_shader *s, int pipeline)
{
    GLint status;
    struct tms_program *p;

        tms_assertf(glGetError() == 0, "vafan -1 tjena");

    if (pipeline == TMS_NO_PIPELINE) {
        if (s->program)
            return s->program;
    } else {
        if (s->pipeline_program[pipeline])
            return s->pipeline_program[pipeline];
    }

    if (!s->vertex || !s->fragment)
        tms_fatalf("shader %s needs a minimum of 1 vertex shader and 1 fragment shader", s->name);

    p = calloc(1, sizeof(struct tms_program));

    tms_program_init(p);

    p->parent = s;
    p->id = glCreateProgram();
    p->pipeline = pipeline;

    tms_assertf(p->id != -1, "glCreateProgram() failed");

        tms_assertf(glGetError() == 0, "vafan 1 tjena");
    glAttachShader(p->id, s->vertex);
        tms_assertf(glGetError() == 0, "vafan 2 ");
    glAttachShader(p->id, s->fragment);
#ifndef TMS_USE_GLES
    if (s->tess_control) glAttachShader(p->id, s->tess_control);
    if (s->tess_eval) glAttachShader(p->id, s->tess_eval);
    if (s->geometry) glAttachShader(p->id, s->geometry);
#endif

    glLinkProgram(p->id);
        tms_assertf(glGetError() == 0, "vafan 3");

    glGetProgramiv(p->id, GL_LINK_STATUS, &status);
    if (!status) {
        glGetProgramInfoLog(p->id, INFOLOG_SIZE-1, &infolog_length, infolog);
        tms_errorf("error linking program for shader %s: %s", s->name, infolog);
        glDeleteProgram(p->id);
        free(p);
        return 0;
    }

    tms_program_load_attributes(p);
    tms_program_load_uniforms(p);

    if (pipeline == TMS_NO_PIPELINE) {
        s->program = p;
    } else
        s->pipeline_program[pipeline] = p;

    return p;
}

int
tms_shader_compile(struct tms_shader *s,
                   GLenum shader_type,
                   const char *source)
{
    GLuint shader = compile(s, shader_type, source);

    if (shader == -1) {
        tms_errorf("Error compiling shader[%s]: %s", s->name, source);
        tms_errorf("Defines (normal/fs/vs) (%d/%d/%d)", s->num_defines, s->num_fs_defines, s->num_vs_defines);
        for (int x=0; x<s->num_defines; x++) {
            tms_errorf("define[%d/%d]: '%s'", x+1, s->num_defines, s->defines[x]);
        }
        for (int x=0; x<s->num_fs_defines; x++) {
            tms_errorf("fs define[%d/%d]: '%s'", x+1, s->num_fs_defines, s->fs_defines[x]);
        }
        for (int x=0; x<s->num_vs_defines; x++) {
            tms_errorf("vs define[%d/%d]: '%s'", x+1, s->num_vs_defines, s->vs_defines[x]);
        }
        return T_ERR;
    }

    switch (shader_type) {
        case GL_VERTEX_SHADER: s->vertex = shader; break;
        case GL_FRAGMENT_SHADER: s->fragment = shader; break;
#ifndef TMS_USE_GLES
        case GL_TESS_CONTROL_SHADER: s->tess_control = shader; break;
        case GL_TESS_EVALUATION_SHADER: s->tess_eval = shader; break;
        case GL_GEOMETRY_SHADER: s->geometry = shader; break;
#endif
        default: tms_fatalf("unknown shader type %d, recognized by OpenGL but not TMS :(", shader_type); break;
    }

    return T_OK;
}
