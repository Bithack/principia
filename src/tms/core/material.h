#ifndef _TMS_MATERIAL__H_
#define _TMS_MATERIAL__H_

#include <stdint.h>
#include <tms/core/tms.h>

#define TMS_MATERIAL_MAX_TEXTURES 8

/** @relates tms_material @{ **/

#define TMS_BLENDMODE_OFF 0
#define TMS_BLENDMODE__SRC_ALPHA__ONE_MINUS_SRC_ALPHA 1
#define TMS_BLENDMODE__SRC_ALPHA__ONE 2
#define TMS_BLENDMODE__ONE_MINUS_DST_COLOR__ONE_MINUS_SRC_ALPHA 3

enum {
    TMS_MATERIAL_TEXTURE = 0,
    TMS_MATERIAL_COLOR   = (1 << 1),
    TMS_MATERIAL_LIGHT   = (1 << 2),
    //TMS_MATERIAL_BLEND   = (1 << 4),
    TMS_MATERIAL_FLAT    = (1 << 5),
};

struct tms_shader;
struct tms_texture;

struct tms_material_info {
    struct tms_program *program;
    struct tms_texture *texture[TMS_MATERIAL_MAX_TEXTURES];
    unsigned num_textures;
    uint32_t flags;
    int blend_mode;
};

struct tms_material {
    struct tms_material_info pipeline[TMS_NUM_PIPELINES];
};

struct tms_material* tms_material_alloc(void);
void tms_material_init(struct tms_material *mat);
struct tms_material* tms_material_dup(struct tms_material *m);
void tms_material_copy(struct tms_material *dest, struct tms_material *src);

#endif
