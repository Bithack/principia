#include <string.h>
#include <stdlib.h>

#include "err.h"
#include "entity.h"
#include "scene.h"
#include "program.h"
#include "mesh.h"
#include "material.h"
#include "pipeline.h"
#include "../math/matrix.h"

static int remove_child(struct tms_entity *e, struct tms_entity *child);
int find_uniform(struct tms_entity *e, const char *name);
int add_uniform(struct tms_entity *e, const char *name, int type);

/**
 * Allocate space for an entity.
 **/
struct tms_entity* tms_entity_alloc(void)
{
    struct tms_entity *e;
    ((e = malloc(sizeof(struct tms_entity)))
     && tms_entity_init(e));

    return e;
}

/**
 * Initialize an already allocated entity.
 * If tms_entity_alloc was used to create the entity,
 * this function should not be called.
 **/
int tms_entity_init(struct tms_entity *e)
{
    tmat4_load_identity(e->M);
    e->prio = 0;
    e->prio_bias = 0;
    e->update = 0;
    e->scene = 0;

    e->cull_step = -1; /* XXX */

    memset(e->graph_loc, 0, sizeof(e->graph_loc));

    e->num_children = 0;
    e->children = 0;
    e->parent = 0;
    e->material = 0;
    e->mesh = 0;
    e->num_uniforms = 0;
    e->uniforms = 0;

    return T_OK;
}

int tms_entity_uninit(struct tms_entity *e)
{
    tms_assertf(e, "Entity uninit called on a null-pointer.");

    if (e->uniforms) {
        free(e->uniforms);
    }

    return T_OK;
}

/**
 * Duplicate the entity.
 *
 * @relates tms_entity
 **/
struct tms_entity* tms_entity_dup(struct tms_entity *e)
{
    struct tms_entity *n;

    if ((n = tms_entity_alloc()))
        tms_entity_copy(n, e);

    return n;
}

static void set_prio_all(struct tms_entity *e, uintptr_t prio)
{
    int x;

    struct tms_scene *scene = e->scene;
    if (scene)
        tms_scene_remove_entity(scene, e);

    e->prio = prio;

    if (scene)
        tms_scene_add_entity(scene, e);

    for (x=0; x<e->num_children; x++)
        set_prio_all(e->children[x], prio);
}

void tms_entity_set_prio_all(struct tms_entity *e, uintptr_t prio)
{
    set_prio_all(e, prio);
}

/**
 * Copy the entity `source` to `dest`.
 *
 * @relates tms_entity
 **/
void tms_entity_copy(struct tms_entity *dest, struct tms_entity *source)
{
    tms_entity_set_mesh(dest, source->mesh);
    tms_entity_set_material(dest, source->material);
}

int tms_entity_remove_child(struct tms_entity *e, struct tms_entity *p)
{
    return remove_child(e, p);
}

/**
 * Apply the entities uniform variables.
 * Called internally by tms_scene before the entity
 * is rendered.
 **/
void
tms_entity_apply_uniforms(struct tms_entity *e, int pipeline)
{
    int ierr;
    //tms_assertf((ierr = glGetError()) == 0, "gl error %d before entity apply uniforms", ierr);
    for (int x=0; x<e->num_uniforms; x++) {
        if (e->uniforms[x].loc[pipeline] == -1)
            continue;
        switch (e->uniforms[x].type) {
            case TMS_VEC2: glUniform2f(e->uniforms[x].loc[pipeline], e->uniforms[x].val.vec2.x, e->uniforms[x].val.vec2.y); break;
            case TMS_VEC4:
                //tms_assertf((ierr = glGetError()) == 0, "gl error %d before tvec4 uniform", ierr);
                //tms_infof("loc: %d, vec4: %f, %f, %f, %f", e->uniforms[x].loc[pipeline], TVEC4_INLINE(e->uniforms[x].val.vec4));
                glUniform4f(e->uniforms[x].loc[pipeline], TVEC4_INLINE(e->uniforms[x].val.vec4));
                /* XXX FIXME TODO: this fails when the input vector is 0,0,0,0.
                 * not sure if the error is lethal, for now we just skip the error */
                //tms_assertf((ierr = glGetError()) == 0, "gl error %d after tvec4 uniform", ierr);
                GLenum err;
                do {
                    err = glGetError();
                } while (err != GL_NO_ERROR);
                break;
            case TMS_FLOAT: glUniform1f(e->uniforms[x].loc[pipeline], e->uniforms[x].val.vec1); break;

            case TMS_UNIFORM_CUSTOM:
                if (e->uniforms[x].val.fn)
                    e->uniforms[x].val.fn(e, e->uniforms[x].loc[pipeline]);
                break;
        }
        //tms_assertf((ierr = glGetError()) == 0, "gl error %d after %d(%d) entity apply uniforms", ierr, x, e->uniforms[x].type);
    }
}

/**
 * Set this entity's mesh
 **/
int
tms_entity_set_mesh(struct tms_entity *e, struct tms_mesh *m)
{
    struct tms_scene *scene = e->scene;
    if (scene)
        tms_scene_remove_entity(scene, e);

    e->mesh = m;

    if (scene)
        tms_scene_add_entity(scene, e);

    return T_OK;
}

int tms_entity_set_material(struct tms_entity *e, struct tms_material *m)
{
    struct tms_scene *scene = e->scene;

    if (scene)
        tms_scene_remove_entity(scene, e);

    e->material = m;

    /* XXX : */
    /* this will run pretty slow, because realloc() will be called once per uniform,
     * which is pretty bad */
    /* TODO: realloc one big chunk */

    for (int y=0; y<TMS_NUM_PIPELINES; y++) {
        struct tms_material_info *mi = &m->pipeline[y];
        if (mi->program) {
            for (int x=0; x<mi->program->num_uniforms; x++) {
                int offs = -1;
                struct tms_uniform_val *v;
                struct tms_uniform *u = &mi->program->uniforms[x];

                if ((offs = find_uniform(e, u->name)) == -1)
                    offs = add_uniform(e, u->name, u->type);

                e->uniforms[offs].loc[y] = u->loc;
            }
        }
    }

    if (scene)
        tms_scene_add_entity(scene, e);

    return T_OK;
}

int
find_uniform(struct tms_entity *e, const char *name)
{
    for (int x=0; x<e->num_uniforms; x++)
        if (strcmp(name, e->uniforms[x].name) == 0)
            return x;
    return -1;
}

int
add_uniform(struct tms_entity *e,
            const char *name, int type)
{
    (e->uniforms = realloc(e->uniforms, (e->num_uniforms+1)*sizeof(struct tms_uniform_val)))
        || tms_fatalf("out of mem (e_au)");
    e->num_uniforms++;
    e->uniforms[e->num_uniforms-1].type = type;
    e->uniforms[e->num_uniforms-1].name = (char*)name;
    memset(&e->uniforms[e->num_uniforms-1].val, 0, sizeof(e->uniforms[e->num_uniforms-1].val));

    if (*name == '_') {
        e->uniforms[e->num_uniforms-1].type = TMS_UNIFORM_CUSTOM;
    }

    for (int x=0; x<TMS_NUM_PIPELINES; x++) {
        e->uniforms[e->num_uniforms-1].loc[x] = -1;
    }

    return e->num_uniforms - 1;
}

/**
 * Set a tvec4 uniform value of this entity.
 * When this entity is rendered, the shader program's
 * uniform variable with the same name will have the given
 * value applied to it.
 *
 * @relates tms_entity
 **/
int tms_entity_set_uniform4f(struct tms_entity *e, const char *name,
                             float r, float g, float b, float a)
{
    int gamma_correct = 0;

    if (name[0] == '~') {
        name++; /* turn "~color" into "color" */
        if (tms.gamma_correct) {
            gamma_correct = 1;
        }
    }

    int offs = find_uniform(e, name);
    if (offs == -1)
        offs = add_uniform(e, name, TMS_VEC4);

    if (gamma_correct) {
        e->uniforms[offs].val.vec4 = (tvec4){powf(r, tms.gamma),powf(g, tms.gamma),powf(b, tms.gamma),a};
    } else {
        e->uniforms[offs].val.vec4 = (tvec4){r,g,b,a};
    }

    return T_OK;
}

/* set a custom uniform applier function, only supported for
 * uniforms beginning with a '_' in their name */
int
tms_entity_set_uniform_fn(struct tms_entity *e, const char *name, void *fn)
{
    if (*name != '_')
        return 1;

    int offs = find_uniform(e, name);
    if (offs == -1)
        offs = add_uniform(e, name, TMS_UNIFORM_CUSTOM);

    e->uniforms[offs].val.fn = fn;

    return T_OK;
}

/**
 * Set a tvec2 uniform value of this entity.
 *
 * @relates tms_entity
 **/
int tms_entity_set_uniform2f(struct tms_entity *e, const char *name,
                             float r, float g)
{
    int offs = find_uniform(e, name);
    if (offs == -1)
        offs = add_uniform(e, name, TMS_VEC2);

    e->uniforms[offs].val.vec2 = (tvec2){r,g};

    return T_OK;
}

/**
 * Set a float uniform value of this entity.
 *
 * @relates tms_entity
 **/
int tms_entity_set_uniform1f(struct tms_entity *e, const char *name,
                             float r)
{
    int offs = find_uniform(e, name);
    if (offs == -1)
        offs = add_uniform(e, name, TMS_FLOAT);

    e->uniforms[offs].val.vec1 = r;

    return T_OK;
}

int
tms_entity_add_child(struct tms_entity *e, struct tms_entity *child)
{
    /* TODO: if the child is already added to a scene, readd it? */
    tms_assertf(child->parent == 0, "child is already added to an entity");

    e->children = realloc(e->children, (e->num_children+1)*sizeof(struct tms_entity*));

    /* XXX handle children realloc fail */
    e->children[e->num_children] = child;
    e->num_children ++;

    child->parent = e;

    if (e->scene) tms_scene_add_entity(e->scene, child);

    return T_OK;
}

static int remove_child(struct tms_entity *e, struct tms_entity *child)
{
    for (int x=0; x<e->num_children; x++) {
        if (e->children[x] == child) {
            if (x != e->num_children-1) {
                //memmove(&e->children[x], &e->children[x+1], (e->num_children-x)*sizeof(void*));
                e->children[x] = e->children[e->num_children-1];
            }

            if (e->num_children == 1) {
                free(e->children);
                e->children = 0;

            } else {
                e->children = realloc(e->children, (e->num_children-1)*sizeof(struct tms_entity*));
                tms_assertf(e->children != 0, "num children is 0");
            }
            e->num_children --;
            break;
        }
    }

    child->parent = 0;

    return T_OK;
}

