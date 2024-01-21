#ifndef _TMS_ENTITY__H_
#define _TMS_ENTITY__H_

/** @relates tms_entity @{ **/

#include <tms/core/tms.h>
#include <tms/math/vector.h>

#define TMS_MAX_GRAPHS_PER_SCENE 4

struct tms_material;
struct tms_mesh;
struct tms_scene;
struct tms_uniform_val;

struct tms_entity_graph_loc {
    struct _branch *branch;
    int             pos;
};

/**
 * An entity is any object added to a tms_scene.
 **/
struct tms_entity {
    int       type;
    uint32_t  id;
    uintptr_t prio;
    int       prio_bias;
    struct tms_material *material;/**< must never be set directly but through tms_entity_set_material */
    struct tms_mesh *mesh; /**< must never be set directly but through tms_entity_set_mesh */
    int cull_step;

    unsigned num_children;
    struct tms_entity **children;
    struct tms_entity  *parent;

    float M[16];
    float N[9];

    struct tms_uniform_val *uniforms;
    int num_uniforms;

    void (*update)(struct tms_entity *e);

    /* scene info, if this entity is added to a scene this will be filled */
    struct tms_scene *scene;

    struct tms_entity_graph_loc graph_loc[TMS_MAX_GRAPHS_PER_SCENE];

    void *data;
};

struct tms_entity* tms_entity_alloc(void);
int tms_entity_init(struct tms_entity *e);
int tms_entity_uninit(struct tms_entity *e);
struct tms_entity* tms_entity_dup(struct tms_entity *e);
void tms_entity_copy(struct tms_entity *dest, struct tms_entity *source);
int tms_entity_set_material(struct tms_entity *e, struct tms_material *m);
int tms_entity_set_mesh(struct tms_entity *e, struct tms_mesh *m);
void tms_entity_apply_uniforms(struct tms_entity *e, int pipeline);
int tms_entity_add_child(struct tms_entity *e, struct tms_entity *child);
int tms_entity_remove_child(struct tms_entity *e, struct tms_entity *child);
int tms_entity_set_uniform4f(struct tms_entity *e, const char *name, float r, float g, float b, float a);
int tms_entity_set_uniform2f(struct tms_entity *e, const char *name, float r, float g);
int tms_entity_set_uniform1f(struct tms_entity *e, const char *name, float r);
int tms_entity_set_uniform_fn(struct tms_entity *e, const char *name, void *fn);
void tms_entity_readd(struct tms_entity *e);

void tms_entity_set_prio_all(struct tms_entity *e, uintptr_t prio);

static inline void tms_entity_update(struct tms_entity *e)
{
    if (e->update)
        e->update(e);
}

static inline void tms_entity_update_with_children(struct tms_entity *e)
{
    if (e->update)
        e->update(e);

    for (unsigned x=0; x<e->num_children; ++x)
        tms_entity_update_with_children(e->children[x]);
}

static inline struct tms_scene* tms_entity_get_scene(struct tms_entity *e)
{
    return e->scene;
}

static inline void tms_entity_set_update_fn(struct tms_entity *e, void *fn)
{
    e->update = (void(*)(struct tms_entity*))fn;
}

static inline void tms_entity_set_type(struct tms_entity *e, int type) {
    e->type = type;
}

static inline int tms_entity_get_type(struct tms_entity *e)
{
    return e->type;
}

#endif
