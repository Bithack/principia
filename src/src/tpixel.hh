#pragma once

#include "ud2.hh"

enum {
    TPIXEL_MATERIAL_GRASS,      // 0
    TPIXEL_MATERIAL_DIRT,       // 1
    TPIXEL_MATERIAL_HEAVY_DIRT, // 2
    TPIXEL_MATERIAL_ROCK,       // 3

    TPIXEL_MATERIAL_DIAMOND_ORE,
    TPIXEL_MATERIAL_RUBY_ORE,
    TPIXEL_MATERIAL_SAPPHIRE_ORE,
    TPIXEL_MATERIAL_EMERALD_ORE,
    TPIXEL_MATERIAL_TOPAZ_ORE,
    TPIXEL_MATERIAL_COPPER_ORE,
    TPIXEL_MATERIAL_IRON_ORE,
    TPIXEL_MATERIAL_ALUMINIUM_ORE,

    NUM_TPIXEL_MATERIALS,
};

struct tpixel_desc_1_5 : public ud2_info {
    /* static data */
    uint8_t size;
    uint8_t pos;
    uint8_t material;
    uint8_t r;

    /* runtime data */
    float oil;
    float hp;
    uint8_t grass;
};

struct tpixel_desc : public ud2_info {
    /* static data */
    uint8_t size;
    uint8_t pos;
    uint8_t material;
    uint8_t r;

    /* runtime data */
    float oil;
    float hp;
    uint8_t grass;


    struct tpixel_desc_1_5_1 {
        uint8_t half; /* 0 = full pixel, 1..4 = corner */
    } desc_1_5_1;

    tpixel_desc() : ud2_info(UD2_TPIXEL_DESC) { desc_1_5_1.half = 0; }

    inline int get_local_x(){return pos & 15;}
    inline int get_local_y(){return pos >> 4;}
    inline void reset()
    {
        switch (this->material) {
            case TPIXEL_MATERIAL_DIRT:
            case TPIXEL_MATERIAL_GRASS:
                this->hp = 2.f;
                break;

            case TPIXEL_MATERIAL_HEAVY_DIRT:
                this->hp = 4.f;
                break;

            case TPIXEL_MATERIAL_ROCK:
                this->hp = 6.f;
                break;

            default:
                this->hp = 12.f;
                break;
        }

        this->oil = 100.f * (material*3) * (size*3);
        this->grass = 0;
        this->desc_1_5_1.half = 0;
    }
};

#ifndef _NO_TMS

#include "basepixel.hh"
#include "resource.hh"

#define TPIXEL_DYN_BUF 3

extern struct tpixel_material {
    const char *name;
    tms::material *material;
    tvec4 color;
    float drops[NUM_RESOURCES];
} tpixel_materials[NUM_TPIXEL_MATERIALS];

class tpixel : public basepixel
{
  private:
    struct tms_entity ore;

  public:
    bool optimized_render;
    float oil;

    struct tpixel_desc desc;

    tpixel();

    static void initialize();
    static struct tms_entity *get_entity(int x);
    static void reset_counter();
    static void upload_buffers();

    const char *get_name(void)
    {
        if (this->body) {
            return tpixel_materials[this->properties[1].v.i8].name;
        }
        return "Block";
    };

    void prepare_fadeout();
    void update();

    void set_block_type(int mat);

    void recreate_shape(bool skip_search=false, bool dynamic=false);
    void update_appearance();
    void setup();
    void construct();
    void on_load(bool created, bool has_state);

    void drop_loot(int num=1);

    float get_slider_snap(int s);
    float get_slider_value(int s);
    const char *get_slider_label(int s){if (s==0)return basepixel::get_slider_label(s);else return "Material";};
    void on_slider_change(int s, float value);
};

#endif
