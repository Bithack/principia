#ifndef _TMS_MODEL__H_
#define _TMS_MODEL__H_

struct tms_mesh;
struct tms_gbuffer;
struct tms_mesh;

/** @relates tms_model @{ */

/**
 * A model loaded from file
 * Essentially a collection of meshes
 **/
struct tms_model {
    struct tms_varray *va;
    struct tms_gbuffer *vertices;
    struct tms_gbuffer *indices;
    struct tms_mesh **meshes;
    int num_meshes;
    int flat;
    int disable_upload;
};

struct tms_model *tms_model_alloc(void);
struct tms_mesh * tms_model_load(struct tms_model *m, const char *filename, int *status);
struct tms_mesh * tms_model_create_mesh(struct tms_model *m);

/* duplicate a mesh and shift the texture coordinates */
struct tms_mesh* tms_model_shift_mesh_uv(struct tms_model *m,
        struct tms_mesh *mesh,
        float dx, float dy);
void tms_model_upload(struct tms_model *m);

#endif
