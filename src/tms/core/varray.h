#ifndef _TMS_VARRAY__H_
#define _TMS_VARRAY__H_

#include <tms/backend/opengl.h>
#include <stdint.h>
#include <stdlib.h>

struct tms_gbuffer;

struct tms_varray_amapping {
    const char *name;
    GLenum component_type;
    int num_components;
    int offset;
    int gbuf;
};

struct tms_varray_gbufdata {
    struct tms_gbuffer *gbuf;
    size_t vsize;
};

/**
 * Vertex array
 **/
struct tms_varray {
    int num_mappings;
    int vnum;
    int num_gbufs;
    struct tms_varray_gbufdata *gbufs;
    struct tms_varray_amapping *mappings;
};

struct tms_varray *tms_varray_alloc(size_t num_attributes);
void tms_varray_init(struct tms_varray *va, size_t num_attributes);
int tms_varray_set_buffer_stride(struct tms_varray *va, struct tms_gbuffer *gbuf, size_t offset);
int tms_varray_map_attribute(struct tms_varray *va, const char *name, int num_components, GLenum component_type, struct tms_gbuffer *gbuf);
int tms_varray_bind_attributes(struct tms_varray *va, int *locations);
int tms_varray_unbind_attributes(struct tms_varray *va, int *locations);
void tms_varray_upload_all(struct tms_varray *va);

#endif
