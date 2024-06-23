#include <stdlib.h>

#include "material.h"

struct tms_material* tms_material_alloc(void)
{
    struct tms_material *m = calloc(1, sizeof(struct tms_material));
    return m;
}

void
tms_material_init(struct tms_material *mat )
{
    /*
    mat->shader = 0;
    memset(mat->texture, 0, sizeof(mat->texture));
    mat->num_textures = 0;
    mat->flags = 0;
    */
}

struct tms_material* tms_material_dup(struct tms_material *m)
{
    struct tms_material *mn = tms_material_alloc();
    if (mn) tms_material_copy(mn, m);
    return 0;
}

void
tms_material_copy(struct tms_material *dest, struct tms_material *src)
{
    for (int x=0; x<TMS_NUM_PIPELINES; x++) {

        struct tms_material_info *d = &dest->pipeline[x];
        struct tms_material_info *s = &src->pipeline[x];

        d->program = s->program;
        d->flags = s->flags;
        for (int x=0; x<TMS_MATERIAL_MAX_TEXTURES; x++)
            d->texture[x] = s->texture[x];
    }
}
