#include "glob.h"

struct vertex {
    tvec3 pos;
    tvec3 nor;
    tvec2 uv;
};

struct tms_model *tms_model_alloc(void)
{
    struct tms_model *m = calloc(1, sizeof(struct tms_model));

    m->va = tms_varray_alloc(3);

    m->vertices = tms_gbuffer_alloc(0);
    m->indices = tms_gbuffer_alloc(0);

    tms_varray_map_attribute(m->va, "position", 3, GL_FLOAT, m->vertices);
    tms_varray_map_attribute(m->va, "normal", 3, GL_FLOAT, m->vertices);
    tms_varray_map_attribute(m->va, "texcoord", 2, GL_FLOAT, m->vertices);

    m->meshes = 0;
    m->num_meshes = 0;
    m->flat = 0;

    return m;
}

struct tms_mesh *
tms_model_create_mesh(struct tms_model *m)
{
    struct tms_mesh *mesh;

    m->meshes = realloc(m->meshes, (m->num_meshes+1)*sizeof(struct tms_mesh*));
    mesh = (m->meshes[m->num_meshes] = tms_mesh_alloc(m->va, m->indices));
    m->num_meshes ++;

    mesh->owner = m;

    return mesh;
}

void
tms_model_upload(struct tms_model *m)
{
    if (m->meshes) {
        tms_gbuffer_upload(m->vertices);
        tms_gbuffer_upload(m->indices);
    }
}

struct tms_mesh*
tms_model_shift_mesh_uv(struct tms_model *m,
        struct tms_mesh *mesh,
        float dx, float dy)
{
    struct tms_mesh *ret = tms_model_create_mesh(m);

    size_t vsz, osz;
    vsz = m->vertices->size;
    osz = m->indices->size;

    uint16_t base = vsz/sizeof(struct vertex);
    uint16_t last_base = mesh->v_start / sizeof(struct vertex);

    //tms_infof("base %d, last base %d", base, last_base);

    tms_gbuffer_realloc(m->indices, m->indices->size + mesh->i_count*sizeof(uint16_t));
    tms_gbuffer_realloc(m->vertices, m->vertices->size + mesh->v_count*sizeof(struct vertex));

    struct vertex *v = (struct vertex *)(m->vertices->buf + mesh->v_start);
    uint16_t *i = (uint16_t *)(m->indices->buf + mesh->i_start * sizeof(uint16_t));

    struct vertex *nv = (struct vertex *)(m->vertices->buf + vsz);
    uint16_t *ni = (uint16_t *)(m->indices->buf + osz);

    for (int x=0; x<mesh->i_count; x++) {
        ni[x] = (i[x] - last_base) + base;
    }

    for (int x=0; x<mesh->v_count; x++) {
        nv[x] = v[x];
        nv[x].uv.x += dx;
        nv[x].uv.y += dy;
    }

    ret->owner = m;
    ret->v_start = vsz;
    ret->v_count = mesh->v_count;

    ret->i_start = osz / 2;
    //tms_infof("i start %d", ret->i_start);
    ret->i_count = mesh->i_count;

    return ret;
}

extern struct tms_mesh * load_3ds_model(struct tms_model *model, SDL_RWops *fp, int *status);

struct tms_mesh *
tms_model_load(struct tms_model *m, const char *filename, int *status)
{
    struct tms_mesh *ret = 0;

    *status = T_ERR;

    SDL_RWops *fp = SDL_RWFromFile(filename,"rb");

    if (!fp)
        tms_fatalf("Could not open model file: %s", filename);

    ret = load_3ds_model(m, fp, status);

    SDL_RWclose(fp);

    return ret;
}
