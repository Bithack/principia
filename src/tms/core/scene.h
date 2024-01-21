#ifndef _TMS_SCENE__H_
#define _TMS_SCENE__H_

#include <tms/math/vector.h>

/** @relates tms_scene @{ */

struct tms_entity;
struct tms_graph;

/**
 * The scene is a sorted tree of entities in the world. The structure
 * of the tree will be static, but before each frame culling will be done.
 * The culling will sort entities in the tree and reorder them depending
 * on whether they are visible or not.
 **/
struct tms_scene {
    struct tms_entity **entities; /**< array of all the entities currently added to the scene */
    int num_entities;
    int cap_entities;

    int cull_step;
    struct tms_graph **graphs;
    int num_graphs;
};

struct tms_scene* tms_scene_alloc(void);
int tms_scene_init(struct tms_scene *scene);
int tms_scene_add_entity(struct tms_scene *s, struct tms_entity *e);
int tms_scene_remove_entity(struct tms_scene *s, struct tms_entity *e);
struct tms_graph *tms_scene_create_graph(struct tms_scene *s, int pipeline);
void tms_scene_clear_entities(struct tms_scene *s);

int  tms_scene_is_entity_culled(struct tms_scene *s, struct tms_entity *e);
void tms_scene_uncull_entity(struct tms_scene *s, struct tms_entity *e);
void tms_scene_cull_all(struct tms_scene *s);

void tms_scene_clear_graphs(struct tms_scene *s);
void tms_scene_fill_graphs(struct tms_scene *s);

#endif
