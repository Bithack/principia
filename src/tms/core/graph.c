#include <tms/core/glob.h>
#include <tms/math/glob.h>
#include "util.h"

#include <tms/backend/opengl.h>

#pragma GCC push_options
#pragma GCC optimize ("O0")

static int enable_blending(struct tms_rstate *state, void *blend);
static int bind_program(struct tms_rstate *state, struct tms_program *program);
static int bind_texture0(struct tms_rstate *state, struct tms_texture *texture);
static int bind_texture1(struct tms_rstate *state, struct tms_texture *texture);
static int bind_texture2(struct tms_rstate *state, struct tms_texture *texture);
static int bind_texture3(struct tms_rstate *state, struct tms_texture *texture);
static int bind_varray(struct tms_rstate *state, struct tms_varray *va);
static int bind_mesh(struct tms_rstate *state, struct tms_gbuffer *m);
static int flat(struct tms_rstate *state, void *);
static int bind_prio(struct tms_rstate *state, void *val);

static int render_entities(struct tms_rstate *state, struct tms_entity **ee, int count);
static int render_branch(struct tms_rstate *s, struct _branch *b, int *sort_v, int depth);
static void branch_remove_entity(struct tms_graph *g, struct _branch *b, struct tms_entity *e);
static inline struct _branch * get_branch(struct tms_graph *g, struct tms_entity *e);

static const int (*sort_fns[])(void*, void*) = {
    (const int (*)(void*, void*)) enable_blending,
    (const int (*)(void*, void*)) bind_program,
    (const int (*)(void*, void*)) bind_texture0,
    (const int (*)(void*, void*)) bind_texture1,
    (const int (*)(void*, void*)) bind_texture2,
    (const int (*)(void*, void*)) bind_texture3,
    (const int (*)(void*, void*)) bind_varray,
    (const int (*)(void*, void*)) bind_mesh,
    (const int (*)(void*, void*)) flat,
    (const int (*)(void*, void*)) bind_prio,
    (const int (*)(void*, void*)) bind_prio,
};

/* 1 or 0 depending on whether the sorting
 * type of the given index sorts depending on true/false.
 * For example, a texture is not a boolean sort, but
 * blending on/off. */
static const int sort_boolean[] = {
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    1,
    0,
    0,
};

struct tms_graph *tms_graph_alloc(void)
{
    struct tms_graph *g = calloc(1, sizeof(struct tms_graph));

    return g;
}

void
tms_graph_init(struct tms_graph *g, struct tms_scene *s, int pipeline)
{
    if (pipeline >= TMS_NUM_PIPELINES)
        tms_fatalf("attempted to create scene graph with invalid pipeline number");

    g->sort_reverse_prio = 0;
    g->post_fn = 0;
    g->scene = s;
    g->p = pipeline;
    g->full_pipeline = 1;

    g->cull_step = 0;
    g->enable_culling = 0;

    /* set up default sorting */
    g->sorting[0] = TMS_SORT_FLAT;
    g->sorting[1] = TMS_SORT_PRIO;
    g->sorting[2] = TMS_SORT_SHADER;
    g->sorting[3] = TMS_SORT_TEXTURE0;
    g->sorting[4] = TMS_SORT_TEXTURE1;
    g->sorting[5] = TMS_SORT_TEXTURE2;
    g->sorting[6] = TMS_SORT_TEXTURE3;
    g->sorting[7] = TMS_SORT_VARRAY;
    g->sorting[8] = TMS_SORT_MESH;
    g->sort_depth = 9;

    memcpy(g->sort_fns, sort_fns, sizeof(sort_fns));
}

void
tms_graph_set_sort_callback(struct tms_graph *g, int sort,
        int (*fun)(struct tms_rstate* rstate, void* value))
{
    g->sort_fns[sort] = (int (*)(void *, void *))fun;
}

int
tms_graph_is_entity_culled(struct tms_graph *g, struct tms_entity *e)
{
    tms_assertf(e->scene == g->scene, "entity(%p) scene(%p) not equal to graph(%p) scene(%p)", e, e->scene, g, g->scene);

    if (!e->graph_loc[g->scene_pos].branch)
        return e->cull_step != g->scene->cull_step;

    return e->graph_loc[g->scene_pos].pos >= e->graph_loc[g->scene_pos].branch->num_unculled || e->graph_loc[g->scene_pos].branch->cull_step != g->cull_step;
}

void
tms_graph_uncull_entity(struct tms_graph *g, struct tms_entity *e)
{
    if (!e->scene || e->scene != g->scene) {
        tms_warnf("cannot uncull entity that is not added");
        return;
    }

    struct _branch *br = e->graph_loc[g->scene_pos].branch;
    int pos = e->graph_loc[g->scene_pos].pos;

    if (!br) {
        goto uncull_children;
    }

    tms_assertf(br->nodes.as_entity[pos] == e, "entity at graph pos does not match entity");

    if (br->cull_step != g->cull_step) {
        br->cull_step = g->cull_step;
        br->num_unculled = 0;
    }

    if (pos < br->num_unculled)
        return;

    if (pos != br->num_unculled) {
        /* swap the entity with the one at the end of the unculled list */
        struct tms_entity *tmp = br->nodes.as_entity[br->num_unculled];
        br->nodes.as_entity[br->num_unculled] = e;
        br->nodes.as_entity[pos] = tmp;

        e->graph_loc[g->scene_pos].pos = br->num_unculled;
        br->nodes.as_entity[pos]->graph_loc[g->scene_pos].pos = pos;
    }

    br->num_unculled ++;

uncull_children:
    for (int x=0; x<e->num_children; x++)
        tms_graph_uncull_entity(g, e->children[x]);
}


/* get the branch that the given entity would be
 * placed in */
static inline struct _branch *
get_branch(struct tms_graph *g, struct tms_entity *e)
{
    void *refval = 0;
    struct _branch *br = &g->root;

    for (int x=0; x<g->sort_depth; x++) {
        if (sort_boolean[g->sorting[x]]) {
            int id = 0;

            switch (g->sorting[x]) {
                //case TMS_SORT_BLENDING: id = (e->material->pipeline[g->p].flags & TMS_MATERIAL_BLEND) ? 1 : 0; break;
                case TMS_SORT_FLAT: id = (e->material->pipeline[g->p].flags & TMS_MATERIAL_FLAT) ? 1 : 0; break;
                default: tms_fatalf("invalid scene sorting %d", x);
            }

            if (br->num_nodes != 2) {
                br->nodes.as_branch = calloc(2, sizeof(struct value));
                br->nodes.as_branch[0].next = calloc(1, sizeof(struct _branch));
                br->nodes.as_branch[0].val = 0;
                br->nodes.as_branch[1].next = calloc(1, sizeof(struct _branch));
                br->nodes.as_branch[1].val = (void *)(intptr_t)1;
                br->fixed = 1;
                br->num_nodes = 2;
            }

            br = br->nodes.as_branch[id].next;
        } else {
            switch (g->sorting[x]) {
                case TMS_SORT_SHADER: refval = e->material->pipeline[g->p].program; break;
                case TMS_SORT_TEXTURE0: refval = e->material->pipeline[g->p].texture[0]; break;
                case TMS_SORT_TEXTURE1: refval = e->material->pipeline[g->p].texture[1]; break;
                case TMS_SORT_TEXTURE2: refval = e->material->pipeline[g->p].texture[2]; break;
                case TMS_SORT_TEXTURE3: refval = e->material->pipeline[g->p].texture[3]; break;
                case TMS_SORT_VARRAY: refval = e->mesh->vertex_array; break;
                case TMS_SORT_MESH: refval = e->mesh->indices; break;
                case TMS_SORT_PRIO: refval = (void *)(intptr_t)e->prio; break;
                case TMS_SORT_PRIO_BIASED: refval = (void *)(intptr_t)e->prio+e->prio_bias; break;
                case TMS_SORT_BLENDING: refval = (void *)(intptr_t)e->material->pipeline[g->p].blend_mode; break;
                default: tms_fatalf("invalid scene sorting");
            }

            struct value *values = br->nodes.as_branch;

            int found = 0;
            for (int y=0; y<br->num_nodes; y ++) {
                if (values[y].val == refval) {
                    br = values[y].next;
                    found = 1;
                    break;
                }
            }

            if (!found) {
                /* not found in this branch, create the branch and continue */
                int i;
                ARRAY_ENSURE_CAPACITY(br->nodes.as_branch, br->num_nodes+1, br->cap_nodes);

                values = br->nodes.as_branch;

                if (g->sorting[x] == TMS_SORT_PRIO) {
                    br->fixed = 1;
                    /* find where in the array to insert this value */
                    int v;
                    for (v=0; v<br->num_nodes; v++) {
                        if (values[v].val < refval)
                            break;
                    }

                    if (v != br->num_nodes)
                        memmove(&values[v+1], &values[v], (br->num_nodes - v) * sizeof(struct value));

                    i = v;
                } else {
                    i = br->num_nodes;
                }

                br->nodes.as_branch[i].next = calloc(1, sizeof(struct _branch));
                br->nodes.as_branch[i].val = refval;
                br->nodes.as_branch[i].next->parent = br;
                br->num_nodes ++;
                br = br->nodes.as_branch[i].next;
            }
        }
    }

    return br;
}

static int
render_entities(struct tms_rstate *state,
                struct tms_entity **ee,
                int count)
{
#if 0
    if (state->p == 0) {
        tms_infof("rendering %d entities", count);
        if (count == 1) {
            return T_OK;
        }
        /*
        if (count == 792)
            return T_OK;*/
    }
#endif

    for (int x=0; x<count; x++, ee++) {
        struct tms_entity *e = *ee;
        struct tms_mesh *m = e->mesh;

        tmat4_copy(state->modelview, state->view);
        tmat4_multiply(state->modelview, e->M);

        tms_pipeline_apply_local_uniforms(state->p, state, state->program, e);
        tms_pipeline_apply_combined_uniforms(state->p, state, state->program, e);

        tms_entity_apply_uniforms(e, state->p);

        if (m->indices) {
            int start = m->i_start;
            int count = m->i_count;

            if (count == -1) {
                count = m->indices->usize / (m->i32?sizeof(int):sizeof(short));
            }

            if (count) glDrawElements(
                m->primitive_type,
                count,
                (m->i32?GL_UNSIGNED_INT:GL_UNSIGNED_SHORT),
                (char*)(start * (m->i32?sizeof(int):sizeof(short)))
            );
        } else {
            glDrawArrays(m->primitive_type, 0, m->vertex_array->gbufs[0].gbuf->usize / m->vertex_array->gbufs[0].vsize);
        }
    }
    //glFlush();

    return T_OK;
}

static
void branch_remove_branch(struct _branch *b, struct _branch *c)
{
    if (b->fixed)
        return;

    for (int x=0; x<b->num_nodes; x++) {
        if (b->nodes.as_branch[x].next == c) {
            if (x != b->num_nodes - 1)
                b->nodes.as_branch[x] = b->nodes.as_branch[b->num_nodes-1];

            b->num_nodes --;
            break;
        }
    }

    free(c->nodes.as_branch);
    free(c);

    if (b->num_nodes == 0)
        if (b->parent) branch_remove_branch(b->parent, b);
}

static void
branch_remove_entity(struct tms_graph *g, struct _branch *b, struct tms_entity *e)
{
    for (int x=0; x<b->num_nodes; x++) {
        if (b->nodes.as_entity[x] == e) {
            /* found it, now swap it with the last element in the array */
            if (x != b->num_nodes - 1) {
                b->nodes.as_entity[x] = b->nodes.as_entity[b->num_nodes-1];

                /* update the branch pos of the entity we moved */
                b->nodes.as_entity[x]->graph_loc[g->scene_pos].pos = x;
            }

            /*if (x <= b->num_unculled) */
            if (b->num_unculled > 0) b->num_unculled --;
            else b->num_unculled = 0;

            b->num_nodes --;
            break;
        }
    }

    if (b->num_nodes == 0) {
        /* no nodes left, remove this branch from its parent */
        if (b->parent) branch_remove_branch(b->parent, b);
    }

    /* XXX: assert error? */
}

static int
render_branch(struct tms_rstate *s,
              struct _branch *b,
              int *sort_v,
              int depth)
{
    if (depth > 0) {
        if (*sort_v == TMS_SORT_PRIO && s->graph->sort_reverse_prio) {
            for (int x=b->num_nodes-1; x>=0; x--) {
                if ((s->sort_fns[*sort_v])(s, b->nodes.as_branch[x].val) == T_OK) {
                    render_branch(s, b->nodes.as_branch[x].next, sort_v+1, depth-1);
                }
            }
        } else {
            for (int x=0; x<b->num_nodes; x++) {
                if ((s->sort_fns[*sort_v])(s, b->nodes.as_branch[x].val) == T_OK) {
                    render_branch(s, b->nodes.as_branch[x].next, sort_v+1, depth-1);
                }
            }
        }
    } else {
        return s->render_entities_fn(s, b->nodes.as_entity,
                s->graph->enable_culling ? (b->cull_step == s->graph->cull_step ? b->num_unculled : 0) : b->num_nodes
                );
    }

    return T_OK;
}

int
tms_graph_remove_entity_with_children(struct tms_graph *g, struct tms_entity *e)
{
    tms_graph_remove_entity(g, e);
    for (int x=0; x<e->num_children; x++)
        tms_graph_remove_entity_with_children(g,e->children[x]);

    return T_OK;
}

int
tms_graph_remove_entity(struct tms_graph *g, struct tms_entity *e)
{
    if (!e->material || !e->material->pipeline[g->p].program)
        return T_OK;

    if (!e->mesh)
        return T_OK;

    struct _branch *br = get_branch(g, e);
    tms_assertf(br != 0, "error: could not find graph branch");

    branch_remove_entity(g, br, e);

    e->graph_loc[g->scene_pos].branch = 0;
    e->graph_loc[g->scene_pos].pos = 0;

    return T_OK;
}

int
tms_graph_add_entity_with_children(struct tms_graph *g, struct tms_entity *e)
{
    tms_graph_add_entity(g, e);
    for (int x=0; x<e->num_children; x++)
        tms_graph_add_entity_with_children(g,e->children[x]);

    return T_OK;
}

int
tms_graph_add_entity(struct tms_graph *g, struct tms_entity *e)
{
    /* only add the entity if it's compatible with this graph's
     * pipeline */
    if (!e->material || !e->material->pipeline[g->p].program) {
        //tms_errorf("material does not have a shader program");
        return T_OK;
    }

    /* skip the entity if it doesnt have a mesh */
    if (!e->mesh)
        return T_OK;

    struct _branch *br = get_branch(g, e);

    tms_assertf(br != 0, "error: could not create graph branch");

    ARRAY_ENSURE_CAPACITY(br->nodes.as_entity, br->num_nodes+1, br->cap_nodes);

    br->nodes.as_entity[br->num_nodes] = e;
    br->num_nodes ++;

    e->graph_loc[g->scene_pos].branch = br;
    e->graph_loc[g->scene_pos].pos = br->num_nodes-1;

    return T_OK;
}

/*
static void
dump_branch(struct tms_scene_branch *b, int *sorting, int depth, int maxdepth)
{
    char indent[depth+1];
    for (int x=0; x<depth; x++)
        indent[x] = ' ';
    indent[depth] = '\0';

    if (depth < maxdepth) {
        for (int x=0; x<b->num_nodes; x++) {
            switch (*sorting) {
                case TMS_SORT_SHADER: tms_infof("%ssort shader: %s", indent, ((struct tms_shader*)b->nodes.as_branch[x].value)->name?:"null"); break;

                case TMS_SORT_TEXTURE0:
                case TMS_SORT_TEXTURE1:
                case TMS_SORT_TEXTURE2:
                    if (!((struct tms_texture*)b->nodes.as_branch[x].value)) {
                        tms_infof("%ssort texture: NO TEXTURE", indent);
                    } else {
                        tms_infof("%ssort texture: %s", indent, ((struct tms_texture*)b->nodes.as_branch[x].value)->filename?:"null");
                    }
                    break;
                case TMS_SORT_VARRAY: tms_infof("%ssort varray: %p", indent, b->nodes.as_branch[x].value); break;
                case TMS_SORT_MESH: tms_infof("%ssort mesh: %p", indent, b->nodes.as_branch[x].value); break;
            }
            dump_branch(b->nodes.as_branch[x].next, sorting+1, depth+1, maxdepth);
        }
    } else {
        for (int x=0; x<b->num_nodes; x++)
            tms_infof("%sentity: %p", indent, b->nodes.as_entity[x]);
    }
}
*/

/*
void
tms_scene_dump_tree(struct tms_scene *s)
{
    dump_branch(&s->root, s->sorting, 0, s->sort_depth);
}
*/

/**
 * @relates tms_graph
 **/
int
tms_graph_render(struct tms_graph *g, struct tms_camera *cam, void *pipeline_data)
{
    struct tms_rstate state;

    state.p = g->p;
    state.active_tex = -1;
    state.graph = g;
    state.data = pipeline_data;
    state.last_va = 0;
    state.last_loc = 0;

    state.sort_fns = g->sort_fns;
    state.render_entities_fn = render_entities;

    tmat4_copy(state.view, cam->view);
    tmat4_copy(state.projection, cam->projection);

    glActiveTexture(GL_TEXTURE0);

    if (g->full_pipeline) {
        tms_pipeline_begin_render(state.p);
    }

    int status = render_branch(&state, &g->root, g->sorting, g->sort_depth);

    if (state.last_va && state.last_loc) {
        tms_varray_unbind_attributes(state.last_va, state.last_loc);
    }

    if (g->post_fn != 0) {
        g->post_fn(&state);
    }

    if (g->full_pipeline) {
        tms_pipeline_end_render(state.p);
    }

    return status;
}

/**  sorting functions **/

static int
flat(struct tms_rstate *state,
     void *f)
{
    if (f) {
        glDisable(GL_CULL_FACE);
    } else
        glEnable(GL_CULL_FACE);

    return T_OK;
}

static int
enable_blending(struct tms_rstate *state,
                void *blend)
{
    if (blend == 0) {
        glDisable(GL_BLEND);
    } else {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    return T_OK;
}

static int
bind_texture0(struct tms_rstate *state,
              struct tms_texture *texture)
{
    if (state->active_tex != 0) {
        glActiveTexture(GL_TEXTURE0);
        state->active_tex = 0;
    }
    if (!texture) {
        glBindTexture(GL_TEXTURE_2D, 0);
        return T_OK;
    }
#if 0
    tms_infof("binding texture 0: %s", texture->filename);
#endif
    return tms_texture_bind(texture);
}

static int
bind_texture1(struct tms_rstate *state,
              struct tms_texture *texture)
{
    if (state->active_tex != 1) {
        glActiveTexture(GL_TEXTURE1);
        state->active_tex = 1;
    }
    if (!texture) {
        glBindTexture(GL_TEXTURE_2D, 0);
        return T_OK;
    }
#if 0
    tms_infof("binding texture 1: %s", texture->filename);
#endif
    return tms_texture_bind(texture);
}

static int
bind_texture2(struct tms_rstate *state,
              struct tms_texture *texture)
{
    if (state->active_tex != 2) {
        glActiveTexture(GL_TEXTURE2);
        state->active_tex = 2;
    }
    if (!texture) {
        glBindTexture(GL_TEXTURE_2D, 0);
        return T_OK;
    }
    return tms_texture_bind(texture);
}

static int
bind_texture3(struct tms_rstate *state,
              struct tms_texture *texture)
{
    if (state->active_tex != 3) {
        glActiveTexture(GL_TEXTURE3);
        state->active_tex = 3;
    }
    if (!texture) {
        glBindTexture(GL_TEXTURE_2D, 0);
        return T_OK;
    }
    return tms_texture_bind(texture);
}

static int
bind_varray(struct tms_rstate *state,
            struct tms_varray *va)
{
    int *locations = tms_program_get_locations(state->program, va);
    state->last_va = va;
    state->last_loc = locations;
    return tms_varray_bind_attributes(va, locations);
}

static int
bind_mesh(struct tms_rstate *state,
          struct tms_gbuffer *m)
{
    if (m) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m->vbo);

        tms_assertf(glIsBuffer(m->vbo), "is not buffer");
    } else
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    return T_OK;
}

static int bind_prio(struct tms_rstate *state, void *val)
{
    return T_OK;
}

static int
bind_program(struct tms_rstate *state,
             struct tms_program *program)
{
    tms_program_bind(program);

#if 0
    static Uint32 ss=0;

    if (state->p == 0) {
        tms_infof("binding program: %s (since last %d)", program->parent->name, SDL_GetTicks() - ss);
        ss = SDL_GetTicks();
    }
#endif

    /* bind all global pipeline uniforms */
    tms_pipeline_apply_global_uniforms(state->p, state, program);

    state->program = program;

    return T_OK;
}

#pragma GCC pop_options
