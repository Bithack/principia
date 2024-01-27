#ifndef _GRAPH__H_
#define _GRAPH__H_

#include <tms/core/pipeline.h>

enum {
    TMS_SORT_BLENDING = 0,
    TMS_SORT_SHADER   = 1,
    TMS_SORT_TEXTURE0 = 2,
    TMS_SORT_TEXTURE1 = 3,
    TMS_SORT_TEXTURE2 = 4,
    TMS_SORT_TEXTURE3 = 5,
    TMS_SORT_VARRAY   = 6,
    TMS_SORT_MESH     = 7,
    TMS_SORT_FLAT     = 8,
    TMS_SORT_PRIO     = 9,
    TMS_SORT_PRIO_BIASED = 10,

    TMS_SORT__MAX
};

struct tms_camera;
struct tms_scene;
struct value;
struct tms_rstate;

struct _branch {
    union {
        struct value *as_branch;
        struct tms_entity **as_entity;
    } nodes;

    int cull_step;
    int num_unculled;
    int num_nodes;
    int cap_nodes;
    struct _branch *parent;
    int fixed;
};

struct value {
    struct _branch *next;
    void *val;
};

/**
 * A graph is a "path" through a scene using a
 * specified rendering pipeline.
 **/
struct tms_graph {
    struct tms_scene *scene;
    int (*sort_fns[TMS_SORT__MAX])(void *, void*);

    void (*post_fn)(struct tms_rstate*);

    int scene_pos;
    int p; /* pipeline */
    int full_pipeline; /* if set to 1, pipeline begin and end functions are called */
    int cull_step;
    int enable_culling;

    int sorting[TMS_SORT__MAX];
    int sort_depth;
    int sort_reverse_prio;

    void *data;

    struct _branch root;
};

struct tms_graph *tms_graph_alloc(void);
void tms_graph_init(struct tms_graph *g, struct tms_scene *s, int pipeline);
int tms_graph_remove_entity(struct tms_graph *g, struct tms_entity *e);
int tms_graph_add_entity(struct tms_graph *g, struct tms_entity *e);
int tms_graph_render(struct tms_graph *g, struct tms_camera *cam, void *data);
void tms_graph_uncull_entity(struct tms_graph *g, struct tms_entity *e);

static inline void
tms_graph_enable_culling(struct tms_graph *g, int enable)
{
    g->enable_culling = enable;
}

static inline void
tms_graph_cull_all(struct tms_graph *g)
{
    g->cull_step ++;
}

int tms_graph_is_entity_culled(struct tms_graph *g, struct tms_entity *e);
void tms_graph_set_sort_callback(struct tms_graph *g, int sort, int (*fun)(struct tms_rstate* rstate, void* value));

int tms_graph_add_entity_with_children(struct tms_graph *g, struct tms_entity *e);
int tms_graph_remove_entity_with_children(struct tms_graph *g, struct tms_entity *e);

#endif
