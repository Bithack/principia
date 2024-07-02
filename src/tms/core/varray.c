#include "varray.h"
#include "gbuffer.h"
#include <tms/core/err.h>

static int last_amax = 0;

struct tms_varray*
tms_varray_alloc(size_t num_attributes)
{
    struct tms_varray *va;
    tms_assertf(num_attributes < 128, "too many vertex attributes");

    if (!(va = malloc(sizeof(struct tms_varray))))
        return 0;

    tms_varray_init(va, num_attributes);

    return va;
}

void
tms_varray_init(struct tms_varray *va, size_t num_attributes)
{
    va->vnum = 0;
    va->num_gbufs = 0;
    va->gbufs = 0;

    va->mappings = calloc(num_attributes, sizeof(struct tms_varray_amapping));
    va->num_mappings = num_attributes;
}

int
tms_varray_map_attribute(struct tms_varray *va, const char *name,
                         int num_components, GLenum component_type,
                         struct tms_gbuffer *gbuf)
{
    tms_assertf(va->vnum < va->num_mappings, "all attribute mappings already set");

    struct tms_varray_amapping *m = &va->mappings[va->vnum];
    struct tms_varray_gbufdata *gdata;

    int gbuf_id = -1;

    for (int x=0; x<va->num_gbufs; x++) {
        if (va->gbufs[x].gbuf == gbuf) {
            gdata = &va->gbufs[x];
            gbuf_id = x;
            break;
        }
    }

    if (gbuf_id == -1) {
        va->gbufs = realloc(va->gbufs, (va->num_gbufs+1)*sizeof(struct tms_varray_gbufdata));

        if (!va->gbufs) {
            tms_fatalf("out of mem (va)");
        }

        gbuf_id = va->num_gbufs;
        gdata = &va->gbufs[va->num_gbufs];
        gdata->gbuf = gbuf;
        gdata->vsize = 0;
        va->num_gbufs ++;
    }

    m->offset = gdata->vsize;
    m->num_components = num_components;
    m->component_type = component_type;
    m->name = strdup(name); /* XXX */
    m->gbuf = gbuf_id;

    int csize = 0;
    switch (component_type) {
        case GL_FLOAT: csize = sizeof(float) * num_components; break;
        case GL_UNSIGNED_INT: csize = sizeof(uint32_t) * num_components; break;
        case GL_INT: csize = sizeof(int32_t) * num_components; break;
        case GL_SHORT: csize = sizeof(int16_t) * num_components; break;
        default: tms_fatalf("unsupported component type"); break;
    }

    gdata->vsize += csize;
    va->vnum ++;

    return T_OK;
}

int
tms_varray_bind_attributes(struct tms_varray *va,
                           int *locations)
{
#ifdef TMS_USE_VAO
    glBindVertexArrayOES(locations);
    return T_OK;
#else
    int last = -1;

    if (va->num_mappings) {
        int ma = 0;

        for (int x=0; x<va->num_mappings; x++) {

            if (locations[x] == -1) {
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

#ifndef TMS_USE_GLES
            if (m->component_type == GL_UNSIGNED_INT || m->component_type == GL_INT)
                glVertexAttribIPointer(locations[x], m->num_components, m->component_type,
                                      bufdata->vsize, (void*)(uintptr_t)m->offset);
            else
#endif
                glVertexAttribPointer(locations[x], m->num_components, m->component_type,
                                      0, bufdata->vsize, (void*)(uintptr_t)m->offset);

            if (locations[x] > last_amax || last_amax == 0)
                glEnableVertexAttribArray(locations[x]);

            if (locations[x] > ma) ma = locations[x];
        }

        for (int x=ma+1; x<=last_amax; x++)
            glDisableVertexAttribArray(x);

        //last_amax = locations[va->num_mappings-1];
        last_amax = ma;
    }

    return T_OK;
#endif
}

int
tms_varray_unbind_attributes(struct tms_varray *va, int *locations)
{
#ifdef TMS_USE_VAO
    glBindVertexArrayOES(0);
#else
    for (int x=0; x<va->num_mappings; x++) {
        if (locations[x] == -1)
            continue;
        glDisableVertexAttribArray(locations[x]);
    }

    last_amax = 0;
#endif

    return T_OK;
}

void
tms_varray_upload_all(struct tms_varray *va)
{
    for (int x=0; x<va->num_gbufs; x++)
        tms_gbuffer_upload(va->gbufs[x].gbuf);
}

