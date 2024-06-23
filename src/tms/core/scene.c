#include <stdlib.h>
#include <unistd.h>

#include <tms/math/glob.h>
#include <tms/core/glob.h>
#include "util.h"

struct tms_scene*
tms_scene_alloc(void)
{
    struct tms_scene *scene;
    (scene = malloc(sizeof(struct tms_scene))) || tms_fatalf("out of mem (s_a)");

    tms_scene_init(scene);

    return scene;
}

int tms_scene_is_entity_culled(struct tms_scene *s, struct tms_entity *e){return e->cull_step != s->cull_step;};
void tms_scene_uncull_entity(struct tms_scene *s, struct tms_entity *e){e->cull_step = s->cull_step;};
void tms_scene_cull_all(struct tms_scene *s){s->cull_step ++;};

void
tms_scene_clear_graphs(struct tms_scene *s)
{
    for (int x=0; x<s->num_entities; x++) {
        for (int y=0; y<s->num_graphs; y++) {
            tms_graph_remove_entity(s->graphs[y], s->entities[x]);
        }
    }
}

void
tms_scene_fill_graphs(struct tms_scene *s)
{
    tms_infof("fill graphs: %d", s->num_entities);
    for (int x=0; x<s->num_entities; x++) {
        for (int y=0; y<s->num_graphs; y++) {
            tms_graph_add_entity(s->graphs[y], s->entities[x]);
        }
    }
}

int
tms_scene_init(struct tms_scene *scene)
{
    scene->entities = 0;
    scene->num_entities = 0;
    scene->cap_entities = 0;

    scene->graphs = 0;
    scene->num_graphs = 0;

    scene->cull_step = 0; /* XXX */

    /*
    scene->light.cam->near = 10.f;
    scene->light.cam->far = 30.f;
    scene->light.cam->width = tms.window_width/80;
    scene->light.cam->height = tms.window_height/50;
    tms_scene_set_light_pos(scene, 0, 50.f, 0);
    */

    return T_OK;
}

/*
void tms_scene_set_light_pos(struct tms_scene *s, float x, float y, float z)
{
    s->light.pos = (tvec3){x,  y, z};
    s->light.normal = (tvec3){x,  y, z};
    s->light.cam->up = (tvec3){-.2f,1,0};
    tvec3_normalize(&s->light.cam->up);
    tvec3_normalize(&s->light.normal);
    tms_camera_enable(s->light.cam, TMS_CAMERA_LOOKAT);
    tms_camera_set_position(s->light.cam, TVEC3_INLINE(s->light.pos));
    tms_camera_set_direction(s->light.cam, TVEC3_INLINE_N(s->light.normal));
    tms_camera_calculate(s->light.cam);
}
*/

/**
 * @relates tms_scene
 **/
int tms_scene_remove_entity(struct tms_scene *s, struct tms_entity *e)
{
    if (e->scene != s) {
        tms_errorf("entity not added to scene");
    } else {
        //tms_assertf(e->scene == s, "attempt to remove entity from a scene it is not added to");

        for (int x=0; x<s->num_graphs; x++)
            tms_graph_remove_entity(s->graphs[x], e);

        e->scene = 0;
        //e->scene_branch = 0;
        //

        /* XXX */
        for (int x=0; x<s->num_entities; x++) {
            if (s->entities[x] == e) {
                if (x != s->num_entities - 1)
                    s->entities[x] = s->entities[s->num_entities-1];
                s->num_entities --;
                break;
            }
        }
    }

    /* remove all of this entity's children as well */
    for (int x=0; x<e->num_children; x++)
        tms_scene_remove_entity(s, e->children[x]);

    return T_OK;
}

void
tms_scene_clear_entities(struct tms_scene *s)
{
    for (int x=0; x<s->num_entities; x++) {
        for (int y=0; y<s->num_graphs; y++) {
            tms_graph_remove_entity(s->graphs[y], s->entities[x]);
        }

        s->entities[x]->scene = 0;
    }

    s->num_entities = 0;
}

int
tms_scene_add_entity(struct tms_scene *s, struct tms_entity *e)
{
    if (e->scene != 0)
        return 1;

    for (int x=0; x<s->num_graphs; x++)
        tms_graph_add_entity(s->graphs[x], e);

    e->scene = s;
//    e->scene_branch = br;

    ARRAY_ENSURE_CAPACITY(s->entities, s->num_entities+1, s->cap_entities);
    s->entities[s->num_entities] = e;
    s->num_entities ++;

    /* add all children to the scene */
    for (int x=0; x<e->num_children; x++) {
        tms_scene_add_entity(s, e->children[x]);
    }

    return T_OK;
}

struct tms_graph *
tms_scene_create_graph(struct tms_scene *s, int pipeline)
{
    s->graphs = realloc(s->graphs, (s->num_graphs+1)*sizeof(struct tms_graph*));
    s->num_graphs ++;

    s->graphs[s->num_graphs-1] = tms_graph_alloc();
    tms_graph_init(s->graphs[s->num_graphs-1], s, pipeline);
    s->graphs[s->num_graphs-1]->scene_pos = s->num_graphs-1;

    return s->graphs[s->num_graphs-1];
}

