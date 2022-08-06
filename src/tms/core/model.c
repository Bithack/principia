#include "glob.h"
#include "../util/hash.h"

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
#if 0
void
tms_model_plc_translate(struct tms_model *m,
        struct tms_mesh *mesh,
        float dx, float dy,float dz, struct tms_mesh *ret,
        size_t *v_offs, size_t *i_offs
        )
{
    //struct tms_mesh *ret = tms_model_create_mesh(m);

    size_t vsz, osz;
    //vsz = m->vertices->size;
    //osz = m->indices->size;
    vsz = *v_offs;
    osz = *i_offs;

    uint16_t base = vsz/sizeof(struct vertex);
    uint16_t last_base = mesh->v_start / sizeof(struct vertex);

    tms_infof("base %d, last base %d", base, last_base);

    //tms_gbuffer_realloc(m->indices, m->indices->size + mesh->i_count*sizeof(uint16_t));
    //tms_gbuffer_realloc(m->vertices, m->vertices->size + mesh->v_count*sizeof(struct vertex));
    //
    (*v_offs) += mesh->v_count*sizeof(struct vertex);
    (*i_offs) += mesh->i_count*sizeof(uint16_t);

    struct vertex *v = m->vertices->buf+mesh->v_start;
    uint16_t *i = m->indices->buf+mesh->i_start*sizeof(uint16_t);

    struct vertex *nv = m->vertices->buf+vsz;
    uint16_t *ni = m->indices->buf+osz;

    for (int x=0; x<mesh->i_count; x++)
        ni[x] = (i[x] - last_base) + base;

    for (int x=0; x<mesh->v_count; x++) {
        nv[x] = v[x];
        nv[x].pos.x += dx;
        nv[x].pos.y += dy;
        nv[x].pos.z += dz;
    }

    ret->owner = m;
    ret->v_start = vsz;
    ret->v_count = mesh->v_count;

    ret->i_start = osz / 2;
    ret->i_count = mesh->i_count;

    //return ret;
}
#endif

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

    struct vertex *v = m->vertices->buf+mesh->v_start;
    uint16_t *i = m->indices->buf+mesh->i_start*sizeof(uint16_t);

    struct vertex *nv = m->vertices->buf+vsz;
    uint16_t *ni = m->indices->buf+osz;

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

struct tms_mesh *
tms_model_load(struct tms_model *m, const char *filename, int *status)
{
    const char *ext = strrchr(filename, '.')+1;
    struct tms_mesh *ret = 0;

    *status = T_ERR;

    if (ext != 1) {
        struct tms_mesh * (*loader)(struct tms_model *, SDL_RWops *, int *)
            = thash_get(tms.model_loaders, ext, strlen(ext));

        if (!loader) {
            tms_errorf("unsupported model format: %s", ext);
            *status = T_UNSUPPORTED_FILE_FORMAT;
            return 0;
        }

        SDL_RWops *fp = SDL_RWFromFile(filename,"rb");

        if (!fp) {
            tms_errorf("could not open model file: %s", filename);
            *status = T_COULD_NOT_OPEN;
            return 0;
        }

        ret = loader(m, fp, status);

        SDL_RWclose(fp);
    }

    return ret;
}
