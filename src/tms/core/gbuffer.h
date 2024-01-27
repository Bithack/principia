#ifndef _TMS_GBUFFER__H_
#define _TMS_GBUFFER__H_

#include <stdlib.h>
#include <tms/backend/opengl.h>

#define TMS_GBUFFER_STATIC_DRAW GL_STATIC_DRAW
#define TMS_GBUFFER_STATIC_COPY GL_STATIC_COPY
#define TMS_GBUFFER_STATIC_READ GL_STATIC_READ

#define TMS_GBUFFER_DYNAMIC_DRAW GL_DYNAMIC_DRAW
#define TMS_GBUFFER_DYNAMIC_COPY GL_DYNAMIC_COPY
#define TMS_GBUFFER_DYNAMIC_READ GL_DYNAMIC_READ

#define TMS_GBUFFER_STREAM_DRAW GL_STREAM_DRAW
#define TMS_GBUFFER_STREAM_COPY GL_STREAM_COPY
#define TMS_GBUFFER_STREAM_READ GL_STREAM_READ

/**
 * Manage buffers on the graphics card
 **/
struct tms_gbuffer {
    GLuint vbo;
    char *buf;
    size_t size;
    size_t usize;
    int target;
    int usage;
};

struct tms_gbuffer *tms_gbuffer_alloc(size_t size);
void tms_gbuffer_init(struct tms_gbuffer *b, size_t size);
struct tms_gbuffer *tms_gbuffer_alloc_fill(void *data, size_t size);
void* tms_gbuffer_get_buffer(struct tms_gbuffer *b);
int tms_gbuffer_realloc(struct tms_gbuffer *b, size_t new_size);
int tms_gbuffer_upload(struct tms_gbuffer *b);
int tms_gbuffer_upload_partial(struct tms_gbuffer *b, size_t size);
int tms_gbuffer_update(struct tms_gbuffer *b, size_t start_offs, size_t num_bytes);
int tms_gbuffer_set_usage(struct tms_gbuffer *b, int usage);
void tms_gbuffer_free(struct tms_gbuffer *b);

#endif
