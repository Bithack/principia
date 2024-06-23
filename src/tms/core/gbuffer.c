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

    glGenBuffers(1, &b->vbo);
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
    glBindBuffer(b->target, b->vbo);
    glBufferData(b->target, b->size, b->buf, b->usage);

    b->usize = b->size;

    //if (b->size > 24) {
        //tms_infof("uploading : %d", b->size);
    //}

    return T_OK;
}

int
tms_gbuffer_upload_partial(struct tms_gbuffer *b, size_t size)
{
    glBindBuffer(b->target, b->vbo);
    glBufferData(b->target, size, b->buf, b->usage);

    b->usize = size;

    return T_OK;
}

int
tms_gbuffer_update(struct tms_gbuffer *b, size_t start_offs, size_t num_bytes)
{
    glBindBuffer(b->target, b->vbo);
    glBufferData(b->target, num_bytes, b->buf + start_offs, GL_DYNAMIC_DRAW);

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

