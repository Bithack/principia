#include "gbuffer.h"
#include <tms/core/err.h>
#include <tms/backend/print.h>

struct tms_gbuffer*
tms_gbuffer_alloc(size_t size)
{
    struct tms_gbuffer *b;

    b = malloc(sizeof(struct tms_gbuffer));

    tms_gbuffer_init(b, size);
    return b;
}

void
tms_gbuffer_init(struct tms_gbuffer *b, size_t size)
{
    if (size == 0)
        b->buf = 0;
    else
        b->buf = malloc(size);
    //tms_assertf(b->buf != 0, "out of mem (gb_i)");
    //b->usage = TMS_GBUFFER_DYNAMIC_DRAW;
    b->usage = TMS_GBUFFER_STATIC_DRAW;
    b->size = size;
    b->usize = 0;
    b->target = GL_ARRAY_BUFFER;

#ifdef TMS_BACKEND_WINDOWS
    if (GLEW_VERSION_1_5) {
        glGenBuffers(1, &b->vbo);
    } else {
        glGenBuffersARB(1, &b->vbo);
    }
#else
    glGenBuffers(1, &b->vbo);
#endif
}

struct tms_gbuffer*
tms_gbuffer_alloc_fill(void *data, size_t size)
{
    struct tms_gbuffer *b = tms_gbuffer_alloc(size);
    memcpy(b->buf, data, size);

    return b;
}

int
tms_gbuffer_realloc(struct tms_gbuffer *b, size_t newsize)
{
    if (newsize == 0) newsize = 1;
    (b->buf = realloc(b->buf, newsize)) || tms_fatalf("out of mem (gb_r)");
    b->size = newsize;
    return T_OK;
}

int
tms_gbuffer_set_usage(struct tms_gbuffer *b, int usage)
{
    b->usage = usage;
    return T_OK;
}

int
tms_gbuffer_upload(struct tms_gbuffer *b)
{
#ifdef TMS_BACKEND_WINDOWS
    if (GLEW_VERSION_1_5) {
        glBindBuffer(b->target, b->vbo);
        glBufferData(b->target, b->size, b->buf, b->usage);
    } else {
        glBindBufferARB(b->target, b->vbo);
        glBufferDataARB(b->target, b->size, b->buf, b->usage);
    }
#else
     glBindBuffer(b->target, b->vbo);
     glBufferData(b->target, b->size, b->buf, b->usage);
#endif

     b->usize = b->size;

     //if (b->size > 24) {
         //tms_infof("uploading : %d", b->size);
     //}

     return T_OK;
}

int
tms_gbuffer_upload_partial(struct tms_gbuffer *b, size_t size)
{
#ifdef TMS_BACKEND_WINDOWS
    if (GLEW_VERSION_1_5) {
         glBindBuffer(b->target, b->vbo);
         glBufferData(b->target, size, b->buf, b->usage);
    } else {
        glBindBufferARB(b->target, b->vbo);
        glBufferDataARB(b->target, size, b->buf, b->usage);
    }
#else
     glBindBuffer(b->target, b->vbo);
     glBufferData(b->target, size, b->buf, b->usage);
#endif

     b->usize = size;

     return T_OK;
}

int
tms_gbuffer_update(struct tms_gbuffer *b, size_t start_offs, size_t num_bytes)
{
#ifdef TMS_BACKEND_WINDOWS
    if (GLEW_VERSION_1_5) {
        glBindBuffer(b->target, b->vbo);
        glBufferData(b->target, start_offs, num_bytes, b->buf+start_offs);
    } else {
        glBindBufferARB(b->target, b->vbo);
        glBufferDataARB(b->target, start_offs, num_bytes, b->buf+start_offs);
    }
#else
     glBindBuffer(b->target, b->vbo);
     glBufferData(b->target, start_offs, num_bytes, b->buf+start_offs);
#endif

     return T_OK;
}

void
tms_gbuffer_free(struct tms_gbuffer *b)
{
    glDeleteBuffers(1, &b->vbo);
    free(b->buf);
    free(b);
}

void*
tms_gbuffer_get_buffer(struct tms_gbuffer *b)
{
    return b->buf;
}

