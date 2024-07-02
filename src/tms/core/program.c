#include <tms/core/glob.h>
#include "hash.h"

void
tms_program_init(struct tms_program *p)
{
    p->locs_tbl = thash_create_ptrdata_table(32);
}

void
tms_program_free(struct tms_program *p)
{
    thash_free(p->locs_tbl);
    if (p->id) glDeleteProgram(p->id);
    free(p);
}

GLuint
tms_program_get_uniform(struct tms_program *p, const char *name)
{
    tms_assertf(name[0] != '~', "oops! tried to fetch a gamma-corrected uniform");

    return glGetUniformLocation(p->id, name);
}

void
tms_program_load_uniforms(struct tms_program *p)
{
    char name[256], *s;
    int x;
    GLint size;
    GLenum type;

    p->num_uniforms = 0;

    glUseProgram(p->id);
    glGetProgramiv(p->id, GL_ACTIVE_UNIFORMS, (GLint*)&p->num_uniforms);
    //tms_assertf(glGetError() == 0, "vafan");

    (p->uniforms = malloc(p->num_uniforms*sizeof(struct tms_uniform)))
        || tms_fatalf("out of mem (p_lu)");

    for (x=0; x<p->num_uniforms; x++) {
        glGetActiveUniform(p->id, x, 255, 0, &size, &type, name);

        p->uniforms[x].name = strdup(name);
        p->uniforms[x].loc = glGetUniformLocation(p->id, name);

        /* XXX */
        if (type == GL_FLOAT_VEC4) {
            type = TMS_VEC4;
        }
        if (type == GL_FLOAT)
            type = TMS_FLOAT;
        p->uniforms[x].type = type;

        if (size != 1) {
            tms_infof("ARRAY %s:%d", name, size);
        }

        switch (type) {
#ifndef TMS_USE_GLES
            case GL_SAMPLER_1D:
            case GL_SAMPLER_3D:
            case GL_SAMPLER_1D_SHADOW:
            case GL_SAMPLER_2D_SHADOW:
#endif

            case GL_SAMPLER_2D:
            case GL_SAMPLER_CUBE:
                /* bind samplers with names like tex_N to texture unit N,
                 * alternative to glsl version 420 layout(binding = N) */
                if ((s = strrchr(name, '_'))) {
                    s++;
                    if (*s) {
                        int binding = atoi(s);
                        //tms_infof("assigning sampler binding: %s %d", p->uniforms[x].name, binding);
                        glUniform1i(p->uniforms[x].loc, binding);
                    }
                } else {
                    glUniform1i(p->uniforms[x].loc, 0);
                }
                break;
        }
    }

    if (p->pipeline != TMS_NO_PIPELINE) {
        struct tms_pipeline *pipe = tms_get_pipeline(p->pipeline);

        if (pipe->num_local) {
            p->p_local = malloc(sizeof(GLuint)*(pipe->num_local+1));

            for (x=0; x<pipe->num_local; x++)
                p->p_local[x] = glGetUniformLocation(p->id, pipe->local[x].name);

            p->p_local[x] = -1;
        }

        if (pipe->num_global) {
            p->p_global = malloc(sizeof(GLuint)*(pipe->num_global+1));

            for (x=0; x<pipe->num_global; x++)
                p->p_global[x] = glGetUniformLocation(p->id, pipe->global[x].name);

            p->p_global[x] = -1;
        }

        if (pipe->num_combined) {
            p->p_combined = malloc(sizeof(GLuint)*(pipe->num_combined+1));

            for (x=0; x<pipe->num_combined; x++)
                p->p_combined[x] = glGetUniformLocation(p->id, pipe->combined[x].name);

            p->p_combined[x] = -1;
        }

        if (pipe->num_constants) {
            int loc;
            for (x=0; x<pipe->num_constants; x++) {

                loc = glGetUniformLocation(p->id, pipe->constants[x].name);

                if (loc == -1)
                    continue;

                switch (pipe->constants[x].type) {
                    case TMS_INT:
                        glUniform1i(loc, pipe->constants[x].val.i);
                        tms_infof("applying constant %s value %d", pipe->constants[x].name, pipe->constants[x].val.i);
                        break;
                }
            }
        }
    }
}

void
tms_program_load_attributes(struct tms_program *p)
{
#if 0
    int x, n;
    GLint size;
    GLenum type;
    char name[256];

    glGetProgramiv(p->id, GL_ACTIVE_ATTRIBUTES, (GLint*)&p->num_attributes);

    if (n = p->num_attributes) {
        (s->attributes = malloc(sizeof(char*)*s->num_attributes))
            || tms_fatalf("out of mem (p_la)");

        for (x=0; x<n; x++) {
            glGetActiveAttrib(p->id, x, 255, 0, &size, &type, name);

            if (strncmp(name, "gl_", 3) == 0) {
                p->num_attributes --;
                continue;
            }

            char *nm = strdup(name);
            int loc =
        }
    }
#endif
}

int*
tms_program_get_locations(struct tms_program *s, struct tms_varray *va)
{
    int *locs;

#ifdef TMS_USE_VAO
    int _locs[va->num_mappings];
#endif

    if (va == s->last_va)
        return s->last_locations;

    locs = thash_get(s->locs_tbl, va);

    if (!locs) {
        /* we haven't paired this shader with the vertex array yet,
         * pair them and add them to the hash table */

#ifdef TMS_USE_VAO
        int last = -1;
        glGenVertexArraysOES(1, &locs);
        glBindVertexArrayOES(locs);

        for (int x=0; x<va->num_mappings; x++)
            _locs[x] = glGetAttribLocation(s->id, va->mappings[x].name);

        if (va->num_mappings) {
            int ma = 0;

            for (int x=0; x<va->num_mappings; x++) {

                if (_locs[x] == -1) {
                    continue;
                }

                struct tms_varray_amapping *m = &va->mappings[x];
                struct tms_varray_gbufdata *bufdata = &va->gbufs[m->gbuf];

                if (last != m->gbuf) {
                    //tms_gbuffer_bind(va->mappings[x].gbuf, GL_ARRAY_BUFFER);
#ifdef TMS_USE_GLEW
                    if (GLEW_VERSION_1_5) {
                        glBindBuffer(GL_ARRAY_BUFFER, bufdata->gbuf->vbo);
                    } else {
                        glBindBufferARB(GL_ARRAY_BUFFER, bufdata->gbuf->vbo);
                    }
#else
                    glBindBuffer(GL_ARRAY_BUFFER, bufdata->gbuf->vbo);
#endif
                    last = m->gbuf;
                }

                if (bufdata->vsize == 4) {
                    ;
                }

                glVertexAttribPointer(_locs[x], m->num_components, m->component_type,
                                      0, bufdata->vsize, (void*)m->offset);

                //if (_locs[x] > last_amax || last_amax == 0)
                    glEnableVertexAttribArray(_locs[x]);

                //if (_locs[x] > ma) ma = _locs[x];
            }

            //for (int x=ma+1; x<=last_amax; x++)
                //glDisableVertexAttribArray(x);

            //last_amax = _locs[va->num_mappings-1];
            //last_amax = ma;
        }

        glBindVertexArrayOES(0);
#else
        (locs = malloc(va->num_mappings * sizeof(int))) || tms_fatalf("out of mem (p_gl)");

        for (int x=0; x<va->num_mappings; x++)
            locs[x] = glGetAttribLocation(s->id, va->mappings[x].name);
#endif

        thash_add(s->locs_tbl, va, locs);
    }

    s->last_va = va;
    s->last_locations = locs;

    return locs;
}

