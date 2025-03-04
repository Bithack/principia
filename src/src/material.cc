#include "material.hh"
#include "sticky.hh"
#include "settings.hh"
#include "robot.hh"
#include "ui.hh"
#include "simplebg.hh"
#include "gui.hh"
#include "misc.hh"

#include <locale.h>

#ifndef GLSL
#define GLSL(...) #__VA_ARGS__
#endif

#define SN(x) x->name = const_cast<char*>(#x)

struct tms_program *menu_bg_program;
GLuint              menu_bg_color_loc;
tms::shader *shader_colored;
tms::shader *shader_gem;
tms::shader *shader_breadboard;
tms::shader *shader_pv_colored;
tms::shader *shader_bark;
tms::shader *shader_pv_rgba;
tms::shader *shader_rubberband;
tms::shader *shader_pv_colored_m;
tms::shader *shader_edev;
tms::shader *shader_edev_m;
tms::shader *shader_edev_dark;
tms::shader *shader_edev_dark_m;
tms::shader *shader_red;
tms::shader *shader_white;
tms::shader *shader_blue;
tms::shader *shader_red_m;
tms::shader *shader_white_m;
tms::shader *shader_blue_m;
tms::shader *shader_grass;
tms::shader *shader_pv_textured;
tms::shader *shader_pv_textured_ao;
tms::shader *shader_textured_ao;
tms::shader *shader_pv_textured_ao_tinted;
tms::shader *shader_pv_textured_m;
tms::shader *shader_shiny;
tms::shader *shader_pv_sticky;
tms::shader *shader_textured;
tms::shader *shader_gi;
tms::shader *shader_gi_col;
tms::shader *shader_gi_tex;
tms::shader *shader_ao;
tms::shader *shader_ao_norot;
tms::shader *shader_ao_clear;
tms::shader *shader_ao_bias;
tms::shader *shader_ledbuf;
tms::shader *shader_digbuf;
tms::shader *shader_spritebuf;
tms::shader *shader_spritebuf_light;
tms::shader *shader_charbuf;
tms::shader *shader_charbuf2;
tms::shader *shader_fluidbuf;
tms::shader *shader_linebuf;
tms::shader *shader_colorbuf;
tms::shader *shader_cable;
tms::shader *shader_wheel;
tms::shader *shader_field;
tms::shader *shader_interactive;
tms::shader *shader_interactive_m;

tms::shader *shader_border;
tms::shader *shader_border_ao;
tms::shader *shader_bg;
tms::shader *shader_bg_fixed;
tms::shader *shader_bg_colored;

tms::shader *shader_grid;

int material_factory::background_id = 0;

m m_colored;
m m_gem;
m m_cavemask;
m m_cable;
m m_pixel;
m m_magnet;
m m_factory;
m m_iomisc;
m m_pv_colored;
m m_interactive;
m m_pv_rgba;
m m_wood;
m m_tpixel;
m m_grass;
m m_weight;
m m_metal;
m m_angulardamper;
m m_iron;
m m_rail;
m m_rocket;
m m_plastic;
m m_pellet;
m m_bullet;
m m_rope;
m m_gen;
m m_battery;
m m_motor;
m m_wmotor;
m m_gear;
m m_gear_ao;
m m_rackhouse;
m m_rack;
m m_sticky;
m m_breadboard;
m m_ladder;
m m_room;
m m_wheel;
m m_cup;
m m_i2o1;
m m_i1o1;
m m_edev;
m m_edev_dark;
m m_spikes;
m m_red;
m m_heavyedev;
m m_cable_red;
m m_cable_black;
m m_cable_blue;
m m_conn;
m m_conn_no_ao;
m m_ledbuf;
m m_digbuf;
m m_charbuf;
m m_charbuf2;
m m_fluidbuf;
m m_spritebuf;
m m_spritebuf2;
m m_linebuf;
m m_linebuf2;
m m_bigpanel;
m m_mpanel;
m m_smallpanel;
m m_misc;
m m_robot;
m m_robot_skeleton;
m m_animal;
m m_robot_tinted;
m m_leaves;
m m_robot_head;
m m_robot_arm;
m m_robot_leg;
m m_robot_foot;
m m_weapon;
m m_weapon_nospecular;
m m_border;
m m_bg;
m m_bg2;
m m_bg_fixed;
m m_grid;
m m_bg_colored;
m m_rubber;
m m_bedrock;
m m_bark;
m m_field;
m m_conveyor;
m m_cpad;
m m_rubberband;
m m_item;
m m_item_shiny;
m m_chest;
m m_chest_shiny;
m m_repairstation;
m m_robot2;
m m_stone;
m m_decoration;
m m_robot_tinted_light;
m m_robot_armor;

static tms::texture *tex_wood = 0;
static tms::texture *tex_animal = 0;
static tms::texture *tex_tpixel = 0;
static tms::texture *tex_grass = 0;
static tms::texture *tex_rubber = 0;
static tms::texture *tex_bark = 0;
static tms::texture *tex_reflection = 0;
static tms::texture *tex_robot = 0;
static tms::texture *tex_weapons = 0;
static tms::texture *tex_leaves = 0;
static tms::texture *tex_magnet = 0;
static tms::texture *tex_factory = 0;
static tms::texture *tex_iomisc = 0;
static tms::texture *tex_motor = 0;
static tms::texture *tex_i2o1 = 0;
static tms::texture *tex_i1o1 = 0;
static tms::texture *tex_gear = 0;
static tms::texture *tex_bigpanel = 0;
static tms::texture *tex_mpanel = 0;
static tms::texture *tex_smallpanel = 0;
static tms::texture *tex_rackhouse = 0;
static tms::texture *tex_rack = 0;
static tms::texture *tex_metal = 0;
static tms::texture *tex_rope = 0;
static tms::texture *tex_gen = 0;
static tms::texture *tex_battery = 0;
static tms::texture *tex_wmotor = 0;
static tms::texture *tex_breadboard = 0;
static tms::texture *tex_wheel = 0;
static tms::texture *tex_cup_ao = 0;
static tms::texture *tex_misc = 0;
static tms::texture *tex_border = 0;
static tms::texture *tex_cpad = 0;
static tms::texture *tex_grid = 0;
static tms::texture *tex_items = 0;
static tms::texture *tex_chests = 0;
static tms::texture *tex_repairstation = 0;
static tms::texture *tex_robot2 = 0;
static tms::texture *tex_decoration = 0;
static tms::texture *tex_robot_armor = 0;

static tms::texture *tex_sprites = 0;
static tms::texture *tex_line = 0;

tms::texture *tex_bg = 0;
tms::texture *tex_bedrock = 0;

const char *available_bgs[] = {
    "Wood 1",
    "Wood 2",
    "Concrete",
    "Space",
    "Wood 3",
    "Outdoor",
    "Colored",
    "Colored space",
    "Wood 4",
    "Wood 5",
    "Wood 6",
    "Bricks 1",
    "Bricks 2",
    "Concrete 2",
    "Concrete 3",
};
const int num_bgs = sizeof(available_bgs)/sizeof(void*);

const int colored_bgs[] = {
    6, /* Colored */
    7, /* Colored space */
    -1
};

static const char *menu_bgsources[] = {
    "attribute vec2 position;"
    "attribute vec2 texcoord;"
    "varying lowp vec2 FS_texcoord;"

    "void main(void) {"
        "FS_texcoord = texcoord * SCALE;"
        "gl_Position = vec4(position, 0, 1.);"
    "}",

    "uniform sampler2D tex_0;"
    "uniform vec4      color;"
    "varying lowp vec2 FS_texcoord;"

    "void main(void) {"
        "gl_FragColor = texture2D(tex_0, FS_texcoord)*color;"
    "}"
};

static void
read_shader(struct shader_load_data *sld, GLenum type, uint32_t global_flags, char **out)
{
    if (!(global_flags & GF_ENABLE_GI) && (sld->flags & SL_REQUIRE_GI)) {
        *out = 0;
        return;
    }

    char path[1024];

    snprintf(path, 1023, "data/shaders/%s.%s",
            sld->name, type == GL_VERTEX_SHADER ? "vp" : "fp");

    FILE_IN_ASSET(1);

    _FILE *fh = _fopen(path, "rb");
    if (fh) {
        _fseek(fh, 0, SEEK_END);
        long size = _ftell(fh);
        _fseek(fh, 0, SEEK_SET);

        *out = (char*)malloc(size+1);

        _fread(*out, 1, size, fh);

        (*out)[size] = '\0';

        _fclose(fh);
    } else {
        *out = 0;
        tms_errorf("Error reading shader at %s!", path);
    }
}

struct shader_load_data shaders[] = {
    { SL_SHARED, "border_ao",               &shader_border_ao },
    { SL_SHARED, "border",                  &shader_border },
    { SL_SHARED, "colored",                 &shader_colored },
    { SL_SHARED, "gem",                     &shader_gem },
    { SL_SHARED, "cable",                   &shader_cable },
    { SL_SHARED, "colorbuf",                &shader_colorbuf }, // TODO: We can precompute the shaded color too
    { SL_SHARED, "pv_colored",              &shader_pv_colored },
    { SL_SHARED, "bark",                    &shader_bark },
    { SL_SHARED, "rubberband",              &shader_rubberband },
    { SL_SHARED, "breadboard",              &shader_breadboard },
    { SL_SHARED, "pv_sticky",               &shader_pv_sticky },
    { SL_SHARED, "pv_textured",             &shader_pv_textured },
    { SL_SHARED, "grass",                   &shader_grass },
    { SL_SHARED, "wheel",                   &shader_wheel },
    { SL_SHARED, "gi",                      &shader_gi },
    {
        SL_SHARED | SL_REQUIRE_GI,
        "gi_tex",
        &shader_gi_tex,
        &shader_gi
    },
    {
        SL_SHARED | SL_REQUIRE_GI,
        "gi_col",
        &shader_gi_col,
        &shader_gi
    },
    { SL_SHARED, "ao",                      &shader_ao },
    { SL_SHARED, "ao_norot",                &shader_ao_norot },
    { SL_SHARED, "ao_clear",                &shader_ao_clear },
    { SL_SHARED, "ao_bias",                 &shader_ao_bias },
    { SL_SHARED, "pv_textured_ao",          &shader_pv_textured_ao },
    { SL_SHARED, "textured_ao",             &shader_textured_ao },
    { SL_SHARED, "pv_textured_ao_tinted",   &shader_pv_textured_ao_tinted },
    { SL_SHARED, "textured",                &shader_textured },
    { SL_SHARED, "ledbuf",                  &shader_ledbuf },
    { SL_SHARED, "digbuf",                  &shader_digbuf },
    { SL_SHARED, "linebuf",                 &shader_linebuf },
    { SL_SHARED, "fluidbuf",                &shader_fluidbuf },
    { SL_SHARED, "spritebuf",               &shader_spritebuf },
    { SL_SHARED, "spritebuf_light",         &shader_spritebuf_light },
    { SL_SHARED, "charbuf",                 &shader_charbuf },
    { SL_SHARED, "charbuf2",                &shader_charbuf2 },
    { SL_SHARED, "grid",                    &shader_grid },
    { SL_SHARED, "bg_fixed",                &shader_bg_fixed },
    { SL_SHARED, "bg_colored",              &shader_bg_colored },
    { SL_SHARED, "bg",                      &shader_bg },
    { SL_SHARED, "shiny",                   &shader_shiny },
    { SL_SHARED, "colored_field",           &shader_field },

    /* menu shaders */
    { SL_SHARED, "pv_colored_m",            &shader_pv_colored_m },
    { SL_SHARED, "pv_textured_m",           &shader_pv_textured_m },
};

static int num_shaders = sizeof(shaders) / sizeof(shaders[0]);

static const char *src_constcolored_m[] = {
GLSL(
    attribute vec3 position;
    attribute vec3 normal;
    attribute vec2 texcoord;

    varying mediump float FS_diffuse;
    varying mediump vec2 FS_texcoord;

    uniform mat4 MVP;

    void main(void)
    {
        FS_diffuse = clamp(dot(vec3(0,0,1), normal), 0., .8)
            /*+ min(position.z-1., 0.)*.5*/;
        gl_Position = MVP*vec4(position, 1.);
    }
),
GLSL(
    varying mediump float FS_diffuse;

    void main(void)
    {
        float ambient =  AMBIENT_M;
        vec4 color = COLOR;
        gl_FragColor = vec4(color.rgb * FS_diffuse + color.rgb * ambient, color.a);
    }
)
};

static const char *src_constcolored[] = {
GLSL(
    attribute vec3 position;
    attribute vec3 normal;

    uniform mat4 MVP;
    uniform mat3 N;
    UNIFORMS

    varying lowp vec2 FS_diffuse;
    VARYINGS

    void main(void)
    {
        vec3 nor = N*normal;

        vec4 pos = MVP*vec4(position, 1.);
        SET_SHADOW
        SET_AMBIENT_OCCL
        SET_GI
        FS_diffuse = vec2(clamp(dot(LIGHT, nor)*_DIFFUSE, 0., 1.), .05*nor.z);
        gl_Position = pos;
    }
),
GLSL(
    UNIFORMS

    varying lowp vec2 FS_diffuse;
    VARYINGS

    GI_FUN

    void main(void)
    {
        gl_FragColor = SHADOW * COLOR * FS_diffuse.x
        + COLOR * (_AMBIENT + FS_diffuse.y) * AMBIENT_OCCL GI;
    }
)
};
/*
static const char *src_wheel[] = {
GLSL(
    attribute vec3 position;
    attribute vec3 normal;
    attribute vec2 texcoord;

    varying lowp vec2 FS_diffuse;
    varying lowp vec2 FS_texcoord;
    VARYINGS

    uniform mat4 MVP;
    uniform mat4 MV;
    uniform mat3 N;
    UNIFORMS

    varying lowp vec3 FS_normal;
    varying lowp vec3 FS_eye;

    void main(void)
    {
        vec3 nor = N*normal;
        vec4 pos = MVP*vec4(position, 1.);

        SET_SHADOW
        SET_AMBIENT_OCCL

        FS_texcoord = texcoord;
        FS_diffuse = vec2(clamp(dot(LIGHT, nor)*_DIFFUSE, 0., 1.), .05*nor.z);

        FS_normal = nor;
        FS_eye = (MV*vec4(position, 1.)).xyz;

        gl_Position = pos;
    }
),
GLSL(
    uniform sampler2D tex_0;
    UNIFORMS

    varying lowp vec2 FS_diffuse;
    varying lowp vec2 FS_texcoord;
    varying lowp vec3 FS_normal;
    varying lowp vec3 FS_eye;
    VARYINGS

    void main(void)
    {
        vec4 color = texture2D(tex_0, FS_texcoord);
        vec3 n = normalize(FS_normal);
        vec3 e = normalize(FS_eye);
        vec3 R = normalize(reflect(LIGHT, n));
        float specular = pow(clamp(dot(R, e), .0, 1.), 6.);
        gl_FragColor = SHADOW * (color + color*specular) * FS_diffuse.x + color.a * color * (_AMBIENT + FS_diffuse.y)*AMBIENT_OCCL
                        ;
    }
)
};
*/

void
material_factory::upload_all()
{
    //tex_wood->upload();
    //tex_metal->upload();
}

void
material_factory::free_shaders()
{
    tms_infof("Freeing shaders...");
    int ierr;
    //tms_assertf((ierr = glGetError()) == 0, "gl error %d at beginning of free shaders", ierr);

    tms_infof("FREEING SHADER_EDEV: %p", shader_edev);
    delete shader_edev;
    delete shader_edev_dark;
    delete shader_edev_m;
    delete shader_edev_dark_m;
    delete shader_bg;
    delete shader_grid;
    delete shader_bg_fixed;
    delete shader_bg_colored;
    delete shader_field;
    delete shader_border;
    delete shader_border_ao;
    delete shader_gem;
    delete shader_breadboard;
    delete shader_pv_colored;
    delete shader_bark;
    delete shader_rubberband;
    delete shader_pv_colored_m;
    delete shader_pv_textured;
    delete shader_wheel;
    delete shader_grass;
    delete shader_pv_textured_ao;
    delete shader_textured_ao;
    delete shader_pv_textured_ao_tinted;
    delete shader_pv_textured_m;
    delete shader_shiny;
    delete shader_pv_sticky;
    delete shader_textured;
    if (shader_gi_tex == shader_gi) {
        shader_gi_tex = 0;
    } else {
        delete shader_gi_tex;
    }
    if (shader_gi_col == shader_gi) {
        shader_gi_col = 0;
    } else {
        delete shader_gi_col;
    }
    delete shader_gi;
    delete shader_ao;
    delete shader_ao_norot;
    delete shader_ao_clear;
    delete shader_ao_bias;
    delete shader_ledbuf;
    delete shader_digbuf;
    delete shader_spritebuf;
    delete shader_spritebuf_light;
    delete shader_charbuf;
    delete shader_charbuf2;
    delete shader_fluidbuf;
    delete shader_linebuf;
    delete shader_cable;
    delete shader_colorbuf;

    tms_infof("Done freeing shaders...");

    //tms_assertf((ierr = glGetError()) == 0, "gl error %d at end of free shaders", ierr);
}
static int last_loaded = -1;

void
material_factory::load_bg_texture(bool soft)
{
    tms_debugf("Load BG Texture...");
    char bgname[256];

    if (material_factory::background_id >= num_bgs || material_factory::background_id < 0)
        material_factory::background_id = 0;

    // color background does not have a texture
    if (material_factory::background_id == BG_COLORED || material_factory::background_id == BG_COLORED_SPACE) {
        return;
    }

    if (!tex_bg) tex_bg = new tms::texture();
    else {
        if (soft && last_loaded == material_factory::background_id) {
            return;
        }
    }
    last_loaded = material_factory::background_id;

    /* special case for bg 5, always jpg file */
    switch (material_factory::background_id) {
        case BG_OUTDOOR:
            {
                tex_bg->load("data/bg/5.jpg");

                tex_bg->format = GL_RGB;
                tex_bg->wrap = GL_CLAMP_TO_EDGE;
                tms_texture_set_filtering(tex_bg, GL_LINEAR);
            }
            break;

        default:
            {
                sprintf(bgname, "data/bg/%d.jpg", material_factory::background_id);

                if (tex_bg->load(bgname) != T_OK)
                    tex_bg->load("data/bg/0.jpg");

                tex_bg->format = GL_RGB;

                tex_bg->wrap = GL_REPEAT;
                tms_texture_set_filtering(tex_bg, GL_LINEAR);
                tex_bg->gamma_correction = settings["gamma_correct"]->v.b;
            }
            break;
    }

    tex_bg->upload();
    tms_texture_free_buffer(tex_bg);

    tms_debugf("Done");
}

#define TEX_LAZYLOAD_FN(name, body) \
    static void lz_##name(struct tms_texture *_tex_##name)\
    { tms::texture *tex_##name = static_cast<tms::texture*>(_tex_##name); \
      body }

TEX_LAZYLOAD_FN(tpixel,
    tms_texture_load(tex_tpixel,"data/textures/tpixel.jpg");
    tex_tpixel->format = GL_RGB;
    tex_tpixel->gamma_correction = settings["gamma_correct"]->v.b;
    tms_texture_set_filtering(tex_tpixel, GL_LINEAR);
    tms_texture_upload(tex_tpixel);
    tms_texture_free_buffer(tex_tpixel);
)

TEX_LAZYLOAD_FN(decoration,
    tms_texture_load(tex_decoration,"data/textures/decorations.jpg");
    tex_decoration->format = GL_RGB;
    tex_decoration->gamma_correction = settings["gamma_correct"]->v.b;
    tms_texture_set_filtering(tex_decoration, GL_LINEAR);
    tms_texture_upload(tex_decoration);
    tms_texture_free_buffer(tex_decoration);
)

TEX_LAZYLOAD_FN(grass,
    tms_texture_load(tex_grass,"data/textures/grass.png");
    tex_grass->format = GL_RGBA;
    tex_grass->gamma_correction = settings["gamma_correct"]->v.b;
    tms_texture_set_filtering(tex_grass, GL_LINEAR);
    tms_texture_upload(tex_grass);
    tms_texture_free_buffer(tex_grass);
)

TEX_LAZYLOAD_FN(grid,
    tms_texture_load(tex_grid,"data/textures/grid.png");
    tex_grid->format = GL_RGBA;
    tex_grid->gamma_correction = settings["gamma_correct"]->v.b;
    tms_texture_set_filtering(tex_grid, TMS_MIPMAP);
    tms_texture_upload(tex_grid);
    tms_texture_free_buffer(tex_grid);
)

TEX_LAZYLOAD_FN(animal,
    tex_animal->load("data/textures/animal.png");
    tex_animal->format = GL_RGB;
    tms_texture_set_filtering(tex_animal, TMS_MIPMAP);
    tex_animal->gamma_correction = settings["gamma_correct"]->v.b;
    tex_animal->upload();
    tms_texture_free_buffer(tex_animal);
)

TEX_LAZYLOAD_FN(wood,
    tex_wood->load("data/textures/wood.jpg");
    tex_wood->format = GL_RGB;
    tms_texture_set_filtering(tex_wood, TMS_MIPMAP);
    tex_wood->gamma_correction = settings["gamma_correct"]->v.b;
    tex_wood->upload();
    tms_texture_free_buffer(tex_wood);
)

TEX_LAZYLOAD_FN(bark,
    tex_bark->load("data/textures/bark-2.jpg");
    tex_bark->format = GL_RGB;
    tms_texture_set_filtering(tex_bark, TMS_MIPMAP);
    tex_bark->gamma_correction = settings["gamma_correct"]->v.b;
    tex_bark->upload();
    tms_texture_free_buffer(tex_bark);
)

TEX_LAZYLOAD_FN(rubber,
    tex_rubber->load("data/textures/rubber.jpg");
    tex_rubber->format = GL_RGB;
    tms_texture_set_filtering(tex_rubber, TMS_MIPMAP);
    tex_rubber->gamma_correction = settings["gamma_correct"]->v.b;
    tex_rubber->upload();
    tms_texture_free_buffer(tex_rubber);
)

TEX_LAZYLOAD_FN(bedrock,
    tex_bedrock->load("data/textures/bedrock.jpg");
    tex_bedrock->format = GL_RGB;
    tms_texture_set_filtering(tex_bedrock, TMS_MIPMAP);
    tex_bedrock->gamma_correction = settings["gamma_correct"]->v.b;
    tex_bedrock->upload();
    tms_texture_free_buffer(tex_bedrock);
)

TEX_LAZYLOAD_FN(reflection,
    tex_reflection->load("data/textures/reflection.jpg");
    tex_reflection->format = GL_RGB;
    tms_texture_set_filtering(tex_reflection, GL_LINEAR);
    tex_reflection->gamma_correction = settings["gamma_correct"]->v.b;
    tex_reflection->upload();
    tms_texture_free_buffer(tex_reflection);
)

TEX_LAZYLOAD_FN(magnet,
    tex_magnet->gamma_correction = settings["gamma_correct"]->v.b;
    tex_magnet->format = GL_RGBA;
    tex_magnet->load("data/textures/magnet.png");
    tex_magnet->upload();
    tms_texture_free_buffer(tex_magnet);
)

TEX_LAZYLOAD_FN(factory,
    tex_factory->gamma_correction = settings["gamma_correct"]->v.b;
    tex_factory->format = GL_RGBA;
    tex_factory->load("data/textures/factory.png");
    tex_factory->upload();
    tms_texture_free_buffer(tex_factory);
)

TEX_LAZYLOAD_FN(iomisc,
    tex_iomisc->gamma_correction = settings["gamma_correct"]->v.b;
    tex_iomisc->format = GL_RGBA;
    tex_iomisc->load("data/textures/iomisc.png");
    tex_iomisc->upload();
    tms_texture_free_buffer(tex_iomisc);
)

TEX_LAZYLOAD_FN(leaves,
    tex_leaves->gamma_correction = settings["gamma_correct"]->v.b;
    tex_leaves->format = GL_RGBA;
    tex_leaves->load("data/textures/leaves.png");
    tex_leaves->upload();
    tms_texture_free_buffer(tex_leaves);
)

TEX_LAZYLOAD_FN(robot,
    tex_robot->gamma_correction = settings["gamma_correct"]->v.b;
    tex_robot->format = GL_RGBA;
    tex_robot->load("data/textures/robot.png");
    tex_robot->upload();
    tms_texture_free_buffer(tex_robot);
)

TEX_LAZYLOAD_FN(robot2,
    tex_robot2->gamma_correction = settings["gamma_correct"]->v.b;
    tex_robot2->format = GL_RGBA;
    tex_robot2->load("data/textures/robot2.png");
    tex_robot2->upload();
    tms_texture_free_buffer(tex_robot2);
)

TEX_LAZYLOAD_FN(robot_armor,
    tex_robot_armor->gamma_correction = settings["gamma_correct"]->v.b;
    tex_robot_armor->format = GL_RGBA;
    tex_robot_armor->load("data/textures/robot_armor.png");
    tex_robot_armor->upload();
    tms_texture_free_buffer(tex_robot_armor);
)

TEX_LAZYLOAD_FN(weapons,
    tex_weapons->gamma_correction = settings["gamma_correct"]->v.b;
    tex_weapons->format = GL_RGBA;
    tex_weapons->load("data/textures/weapons.png");
    tex_weapons->upload();
    tms_texture_free_buffer(tex_weapons);
)

TEX_LAZYLOAD_FN(gen,
    tex_gen->gamma_correction = settings["gamma_correct"]->v.b;
    tex_gen->format = GL_RGBA;
    tex_gen->load("data/textures/generator.png");
    tms_texture_set_filtering(tex_gen, TMS_MIPMAP);
    tex_gen->upload();
    tms_texture_free_buffer(tex_gen);
)

TEX_LAZYLOAD_FN(battery,
    tex_battery->gamma_correction = settings["gamma_correct"]->v.b;
    tex_battery->format = GL_RGBA;
    tex_battery->load("data/textures/battery_aa.png");
    tms_texture_set_filtering(tex_battery, TMS_MIPMAP);
    tex_battery->upload();
    tms_texture_free_buffer(tex_battery);
)

TEX_LAZYLOAD_FN(motor,
    tex_motor->gamma_correction = settings["gamma_correct"]->v.b;
    tex_motor->format = GL_RGBA;
    tex_motor->load("data/textures/motor.png");
    tex_motor->upload();
    tms_texture_free_buffer(tex_motor);
)

TEX_LAZYLOAD_FN(misc,
    tex_misc->gamma_correction = settings["gamma_correct"]->v.b;
    tex_misc->format = GL_RGBA;
    tex_misc->load("data/textures/misc.png");
    //tms_texture_set_filtering(tex_misc, TMS_MIPMAP);
    tex_misc->upload();
    tms_texture_free_buffer(tex_misc);
)

TEX_LAZYLOAD_FN(wmotor,
    tex_wmotor->gamma_correction = settings["gamma_correct"]->v.b;
    tex_wmotor->format = GL_RGB;
    tex_wmotor->load("data/textures/wmotor.png");
    tex_wmotor->upload();
    tms_texture_free_buffer(tex_wmotor);
)

TEX_LAZYLOAD_FN(metal,
    tex_metal->load("data/textures/metal.jpg");
    tex_metal->format = GL_RGB;
    tms_texture_set_filtering(tex_metal, TMS_MIPMAP);
    tex_metal->gamma_correction = settings["gamma_correct"]->v.b;
    tex_metal->upload();
    tms_texture_free_buffer(tex_metal);
)

TEX_LAZYLOAD_FN(i2o1,
    tex_i2o1->gamma_correction = settings["gamma_correct"]->v.b;
    tex_i2o1->format = GL_RGBA;
    tex_i2o1->load("data/textures/i2o1.png");
    tms_texture_set_filtering(tex_i2o1, TMS_MIPMAP);
    tex_i2o1->upload();
    tms_texture_free_buffer(tex_i2o1);
)

TEX_LAZYLOAD_FN(i1o1,
    tex_i1o1->gamma_correction = settings["gamma_correct"]->v.b;
    tex_i1o1->format = GL_RGBA;
    tex_i1o1->load("data/textures/i1o1.png");
    tms_texture_set_filtering(tex_i1o1, TMS_MIPMAP);
    tex_i1o1->upload();
    //tex_i1o1->set_filtering(TMS_MIPMAP);
    tms_texture_free_buffer(tex_i1o1);
)

TEX_LAZYLOAD_FN(gear,
    tex_gear->gamma_correction = settings["gamma_correct"]->v.b;
    tex_gear->format = GL_RGBA;
    tex_gear->load("data/textures/rust.png");
    tex_gear->upload();
    tms_texture_free_buffer(tex_gear);
)

TEX_LAZYLOAD_FN(bigpanel,
    tex_bigpanel->gamma_correction = settings["gamma_correct"]->v.b;
    tex_bigpanel->format = GL_RGBA;
    tex_bigpanel->load("data/textures/bigpanel.png");
    tex_bigpanel->upload();
    tms_texture_free_buffer(tex_bigpanel);
)

TEX_LAZYLOAD_FN(mpanel,
    tex_mpanel->gamma_correction = settings["gamma_correct"]->v.b;
    tex_mpanel->format = GL_RGBA;
    tex_mpanel->load("data/textures/mpanel.png");
    tex_mpanel->upload();
    tms_texture_free_buffer(tex_mpanel);
)

TEX_LAZYLOAD_FN(smallpanel,
    tex_smallpanel->gamma_correction = settings["gamma_correct"]->v.b;
    tex_smallpanel->format = GL_RGBA;
    tex_smallpanel->load("data/textures/smallpanel.png");
    tex_smallpanel->upload();
    tms_texture_free_buffer(tex_smallpanel);
)

TEX_LAZYLOAD_FN(rackhouse,
    tex_rackhouse->load("data/textures/rackhouse.jpg");
    tex_rackhouse->format = GL_RGB;
    tms_texture_set_filtering(tex_rackhouse, TMS_MIPMAP);
    tex_rackhouse->gamma_correction = settings["gamma_correct"]->v.b;
    tex_rackhouse->upload();
    tms_texture_free_buffer(tex_rackhouse);
)

TEX_LAZYLOAD_FN(rack,
    tex_rack->gamma_correction = settings["gamma_correct"]->v.b;
    tex_rack->format = GL_RGBA;
    tex_rack->load("data/textures/rack.png");
    tex_rack->upload();
    tms_texture_free_buffer(tex_rack);
)

TEX_LAZYLOAD_FN(rope,
    tex_rope->gamma_correction = settings["gamma_correct"]->v.b;
    tex_rope->format = GL_RGBA;
    tex_rope->load("data/textures/rope.png");
    tex_rope->upload();
    tms_texture_free_buffer(tex_rope);
)

TEX_LAZYLOAD_FN(wheel,
    tex_wheel->gamma_correction = settings["gamma_correct"]->v.b;
    tex_wheel->format = GL_RGBA;
    tex_wheel->load("data/textures/wheel.png");
    tex_wheel->upload();
    tms_texture_free_buffer(tex_wheel);
)

TEX_LAZYLOAD_FN(cpad,
    tex_cpad->gamma_correction = settings["gamma_correct"]->v.b;
    tex_cpad->format = GL_RGBA;
    tex_cpad->load("data/textures/command.png");
    tex_cpad->upload();
    tms_texture_free_buffer(tex_cpad);
)

TEX_LAZYLOAD_FN(breadboard,
    tex_breadboard->gamma_correction = settings["gamma_correct"]->v.b;
    tex_breadboard->format = GL_RGB;
    tex_breadboard->load("data/textures/breadboard.jpg");
    tex_breadboard->upload();
    tms_texture_free_buffer(tex_breadboard);
)

TEX_LAZYLOAD_FN(cup_ao,
    tex_cup_ao->gamma_correction = settings["gamma_correct"]->v.b;
    tex_cup_ao->format = GL_RGBA;
    tex_cup_ao->load("data/textures/cup_ao.png");
    tex_cup_ao->upload();
    tms_texture_free_buffer(tex_cup_ao);
)

TEX_LAZYLOAD_FN(border,
    tex_border->load("data/textures/border.jpg");
    tex_border->format = GL_RGB;
    tms_texture_set_filtering(tex_border, TMS_MIPMAP);
    tex_border->gamma_correction = settings["gamma_correct"]->v.b;
    tex_border->upload();
    tms_texture_free_buffer(tex_border);
)

TEX_LAZYLOAD_FN(sprites,
    tex_sprites->gamma_correction = settings["gamma_correct"]->v.b;
    tex_sprites->format = GL_RGBA;
    tex_sprites->load("data/textures/sprites.png");
    tex_sprites->upload();
    tms_texture_free_buffer(tex_sprites);
)

TEX_LAZYLOAD_FN(line,
    tex_line->gamma_correction = settings["gamma_correct"]->v.b;
    tex_line->format = GL_RGBA;
    tex_line->load("data/textures/line.png");
    tex_line->upload();
    tms_texture_free_buffer(tex_line);
)

TEX_LAZYLOAD_FN(items,
    tex_items->gamma_correction = settings["gamma_correct"]->v.b;
    tex_items->format = GL_RGBA;
    tex_items->load("data/textures/items.png");
    tms_texture_set_filtering(tex_items, TMS_MIPMAP);
    tex_items->upload();
    tms_texture_free_buffer(tex_items);
)

TEX_LAZYLOAD_FN(chests,
    tex_chests->gamma_correction = settings["gamma_correct"]->v.b;
    tex_chests->format = GL_RGBA;
    tex_chests->load("data/textures/chests.png");
    tms_texture_set_filtering(tex_chests, TMS_MIPMAP);
    tex_chests->upload();
    tms_texture_free_buffer(tex_chests);
)

TEX_LAZYLOAD_FN(repairstation,
    tex_repairstation->gamma_correction = settings["gamma_correct"]->v.b;
    tex_repairstation->format = GL_RGBA;
    tex_repairstation->load("data/textures/repairstation.png");
    tms_texture_set_filtering(tex_repairstation, TMS_MIPMAP);
    tex_repairstation->upload();
    tms_texture_free_buffer(tex_repairstation);
)

void
material_factory::init()
{
    setlocale(LC_ALL, "C");
    setlocale(LC_NUMERIC, "C");

    material_factory::background_id = 0;
    int ierr;
    bool enable_gi = false; // settings["enable_gi"]->v.b

    /* XXX: a gl error occurs when a gtk dialog is shown */
    tms_assertf((ierr = glGetError()) == 0, "gl error %d at material factory init", ierr);

    tms_infof("Initializing material factor...");

    material_factory::init_shaders();

    /* TEXTURES BEGIN */
    tms_infof("Initializing textures... ");

#define TEX_INIT_LAZYLOAD(x) {tex_##x = new tms::texture(); tms_texture_set_buffer_fn(tex_##x, lz_##x);}
    TEX_INIT_LAZYLOAD(weapons);
    TEX_INIT_LAZYLOAD(tpixel);
    TEX_INIT_LAZYLOAD(grass);
    TEX_INIT_LAZYLOAD(grid);
    TEX_INIT_LAZYLOAD(wood);
    TEX_INIT_LAZYLOAD(chests);
    TEX_INIT_LAZYLOAD(repairstation);
    TEX_INIT_LAZYLOAD(animal);
    TEX_INIT_LAZYLOAD(rubber);
    TEX_INIT_LAZYLOAD(bark);
    TEX_INIT_LAZYLOAD(bedrock);
    TEX_INIT_LAZYLOAD(reflection);
    TEX_INIT_LAZYLOAD(magnet);
    TEX_INIT_LAZYLOAD(factory);
    TEX_INIT_LAZYLOAD(iomisc);
    TEX_INIT_LAZYLOAD(robot);
    TEX_INIT_LAZYLOAD(robot2);
    TEX_INIT_LAZYLOAD(gen);
    TEX_INIT_LAZYLOAD(battery);
    TEX_INIT_LAZYLOAD(motor);
    TEX_INIT_LAZYLOAD(misc);
    TEX_INIT_LAZYLOAD(wmotor);
    TEX_INIT_LAZYLOAD(metal);
    TEX_INIT_LAZYLOAD(i2o1);
    TEX_INIT_LAZYLOAD(i1o1);
    TEX_INIT_LAZYLOAD(gear);
    TEX_INIT_LAZYLOAD(bigpanel);
    TEX_INIT_LAZYLOAD(mpanel);
    TEX_INIT_LAZYLOAD(smallpanel);
    TEX_INIT_LAZYLOAD(rackhouse);
    TEX_INIT_LAZYLOAD(rack);
    TEX_INIT_LAZYLOAD(rope);
    TEX_INIT_LAZYLOAD(wheel);
    TEX_INIT_LAZYLOAD(cpad);
    TEX_INIT_LAZYLOAD(breadboard);
    TEX_INIT_LAZYLOAD(cup_ao);
    TEX_INIT_LAZYLOAD(border);
    TEX_INIT_LAZYLOAD(sprites);
    TEX_INIT_LAZYLOAD(line);
    TEX_INIT_LAZYLOAD(leaves);
    TEX_INIT_LAZYLOAD(items);
    TEX_INIT_LAZYLOAD(decoration);
    TEX_INIT_LAZYLOAD(robot_armor);
#undef TEX_INIT_LAZYLOAD

    material_factory::load_bg_texture();

    /* TEXTURES END */

    material_factory::init_materials();
}

void
material_factory::init_shaders()
{
    setlocale(LC_ALL, "C");
    setlocale(LC_NUMERIC, "C");

    tms_infof("Defining shader globals...");

    bool dynamic_lighting = false;
    _tms.gamma_correct = (int)settings["gamma_correct"]->v.b;

    /* Default ambient/diffuse values */
    if (_tms.gamma_correct) {
        //this->default_ambient = .1f;
        //this->default_diffuse = .95f;
        P.default_ambient = .225f;
        P.default_diffuse = 2.8f;
    } else {
        P.default_ambient = .55f;
        P.default_diffuse = 1.1f;
    }

    int ierr;
    char tmp[512];
    bool enable_gi = false; // settings["enable_gi"]->v.b

    tms_shader_global_clear_defines();

#ifndef TMS_BACKEND_ANDROID
    tms_shader_global_define_vs("lowp", "");
    tms_shader_global_define_fs("lowp", "");
    tms_shader_global_define_vs("mediump", "");
    tms_shader_global_define_fs("mediump", "");
    tms_shader_global_define_vs("highp", "");
    tms_shader_global_define_fs("highp", "");
#endif

    if (settings["shadow_map_precision"]->v.i == 0 && !settings["shadow_map_depth_texture"]->is_true()) {
        tms_shader_global_define("SHADOW_BIAS", ".15");
    } else {
        tms_shader_global_define("SHADOW_BIAS", ".005");
    }

    tvec3 light = P.get_light_normal();
    sprintf(tmp, "vec3(%f,%f,%f)", light.x, light.y, light.z);
    tms_shader_global_define("LIGHT", tmp);

    if (settings["enable_shadows"]->v.b) {
        switch (settings["shadow_quality"]->v.u8) {
            default: case 0: case 2:
                tms_shader_global_define_vs("SET_SHADOW",
                        "vec4 shadow = SMVP*pos;"
                        "FS_shadow_z = shadow.z;"
                        "FS_shadow = shadow.xy;");
                break;
            case 1:
            {
                sprintf(tmp,
                        "vec4 shadow = SMVP*pos; FS_shadow_z = shadow.z; FS_shadow = shadow.xy; FS_shadow_dither = shadow.xy + vec2(%f, %f);",
                        //1.f / settings["shadow_map_resx"]->v.i,
                        //1.f / settings["shadow_map_resy"]->v.i
                        1.f / _tms.window_width,
                        1.f / _tms.window_height
                        );
                tms_shader_global_define_vs("SET_SHADOW", tmp);
                break;
            }
        }
    } else {
        tms_shader_global_define_vs("SET_SHADOW", "");
    }

    if (settings["enable_ao"]->v.b) {
        if (!settings["shadow_ao_combine"]->v.b) {
            tms_shader_global_define_vs("SET_AMBIENT_OCCL", "FS_ao = (AOMVP * pos).xy;");
            tms_shader_global_define_vs("SET_AMBIENT_OCCL2", "FS_ao = (AOMVP * pos).xy;");
        } else {
            tms_shader_global_define_vs("SET_AMBIENT_OCCL", "FS_ao = (SMVP * (pos - position.z*MVP[2])).xy;");
            tms_shader_global_define_vs("SET_AMBIENT_OCCL2", "FS_ao = (SMVP * (pos - (position.z+1.0)*MVP[2])).xy;");
        }
    } else {
        tms_shader_global_define_vs("SET_AMBIENT_OCCL", "");
        tms_shader_global_define_vs("SET_AMBIENT_OCCL2", "");
    }

    if (enable_gi) {
        tms_shader_global_define_vs("SET_GI", "FS_gi = (SMVP * (pos + vec4(0.,-2.25,0.,0.))).xy;");
        tms_shader_global_define("ENABLE_GI","1");
        tms_shader_global_define_fs("GI","+ get_gi()");
    } else {
        tms_shader_global_define_vs("SET_GI", "");
        tms_shader_global_define_fs("GI","");
    }

    if (settings["enable_shadows"]->v.b || settings["enable_ao"]->v.b) {
        if (settings["shadow_ao_combine"]->v.b) {
            if (dynamic_lighting) {
                tms_debugf("dl=1 ");
                tms_shader_global_define_vs("UNIFORMS", "uniform mat4 SMVP;uniform lowp vec2 _AMBIENTDIFFUSE;");
            } else {
                tms_debugf("dl=0 ");
                tms_shader_global_define_vs("UNIFORMS", "uniform mat4 SMVP;");
            }
        } else {
            char tmp[1024];
            tmp[0]='\0';

            if (settings["enable_shadows"]->v.b) {
                strcat(tmp, "uniform mat4 SMVP;");
            }
            if (settings["enable_ao"]->v.b) {
                strcat(tmp, "uniform mat4 AOMVP;");
            }
            if (dynamic_lighting) {
                strcat(tmp, "uniform lowp vec2 _AMBIENTDIFFUSE;");
            }

            tms_shader_global_define_vs("UNIFORMS", tmp);
        }
    } else {
        tms_debugf("sao=0 ");
        if (dynamic_lighting) {
            tms_debugf("dl=1 ");
            tms_shader_global_define_vs("UNIFORMS", "uniform lowp vec2 _AMBIENTDIFFUSE;");
        } else {
            tms_shader_global_define_vs("UNIFORMS", "");
        }
    }

    sprintf(tmp, "%s%s%s%s",
            settings["enable_ao"]->v.b ? "uniform lowp vec3 ao_mask;" : "",
            settings["enable_shadows"]->v.b ? "uniform lowp sampler2D tex_3;" : "",
            settings["enable_ao"]->v.b ? "uniform lowp sampler2D tex_4;" : "",
            dynamic_lighting ? "uniform lowp vec2 _AMBIENTDIFFUSE;" : ""
            );
    tms_shader_global_define_fs("UNIFORMS", tmp);

    tms_shader_global_define_fs("GI_FUN",
            enable_gi ?
            "vec4 get_gi(){"
            "vec4 r = vec4(0.,0.,0.,0.);"
            "vec2 offs[16];"
            "float xx = .004;"
            "offs[0] = vec2(0.0, 0.0);"
            "offs[1] = vec2(xx, 0.0);"
            "offs[2] = vec2(xx, xx);"
            "offs[3] = vec2(0., xx);"
            "offs[4] = vec2(-xx, xx);"
            "offs[5] = vec2(-xx, 0.);"
            "offs[6] = vec2(-xx, -xx);"
            "offs[7] = vec2(0., -xx);"
            "offs[8] = vec2(0.0, 0.0);"
            "offs[9] = vec2(xx*2., 0.0);"
            "offs[10] = vec2(xx*2., xx*2.);"
            "offs[11] = vec2(0., xx*2.);"
            "offs[12] = vec2(-xx*2., xx*2.);"
            "offs[13] = vec2(-xx*2., 0.);"
            "offs[14] = vec2(-xx*2., -xx*2.);"
            "offs[15] = vec2(0., -xx*2.);"
            "for (int x=0; x<16; x++){"
            "vec4 s = texture2D(tex_3, FS_gi + offs[x]);"
            "float d = FS_shadow_z-.1 - s.g;"
            "r+=vec4(s.rba*.2, 0.)*float(d > 0. && d < .2);"
            "}return clamp(r / 16., 0, .2);}" : "");

    sprintf(tmp, "%s%s%s%s%s",
            settings["enable_shadows"]->v.b ? "varying lowp float FS_shadow_z;" : "",
            settings["enable_shadows"]->v.b ? "varying lowp vec2 FS_shadow;" : "",
            settings["shadow_quality"]->v.u8 == 1 ? "varying lowp vec2 FS_shadow_dither;" : "",
            settings["enable_ao"]->v.b ? "varying lowp vec2 FS_ao;" : "",
            enable_gi ? "varying lowp vec2 FS_gi;" : ""
            );
    tms_shader_global_define("VARYINGS", tmp);

#define COOL_THING ".005"

    if (settings["enable_shadows"]->v.b)
        switch (settings["shadow_quality"]->v.u8) {
            default: case 0:
                if (settings["shadow_map_depth_texture"]->is_true()) {
                    tms_shader_global_define_fs("SHADOW", "float(texture2D(tex_3, FS_shadow).g > FS_shadow_z- " COOL_THING ")");
                } else {
                    tms_shader_global_define_fs("SHADOW", "float(texture2D(tex_3, FS_shadow).g > FS_shadow_z)");
                }
                break;
            case 1:
                if (settings["shadow_map_depth_texture"]->is_true()) {
                    tms_shader_global_define_fs("SHADOW", "((float(texture2D(tex_3, FS_shadow).g > FS_shadow_z-" COOL_THING ") + float(texture2D(tex_3, FS_shadow_dither).g > FS_shadow_z-" COOL_THING "))*.5)");
                } else {
                    tms_shader_global_define_fs("SHADOW", "((float(texture2D(tex_3, FS_shadow).g > FS_shadow_z) + float(texture2D(tex_3, FS_shadow_dither).g > FS_shadow_z))*.5)");
                }
                break;

            case 2:
                tms_shader_global_define_fs("SHADOW", "(1. - dot(vec3(lessThan(texture2D(tex_3, FS_shadow).rgb, vec3(FS_shadow_z))), vec3(.3333333,.3333333,.3333333)))");
                break;
        }
    else
        tms_shader_global_define_fs("SHADOW", "1.0");

    if (settings["enable_shadows"]->v.b) {
        if (settings["gamma_correct"]->v.b)
            tms_shader_global_define_fs("AMBIENT_OCCL_FACTOR", ".9");
        else
            tms_shader_global_define_fs("AMBIENT_OCCL_FACTOR", ".5");
    } else /* boost AO factor if shadows off */
        tms_shader_global_define_fs("AMBIENT_OCCL_FACTOR", ".7");

    if (settings["enable_ao"]->v.b) {
        tms_shader_global_define_fs("AMBIENT_OCCL", "(1. - AMBIENT_OCCL_FACTOR*dot(texture2D(tex_4, FS_ao).xyz, ao_mask))");
        tms_shader_global_define_fs("AMBIENT_OCCL2", "(1. - AMBIENT_OCCL_FACTOR*dot(texture2D(tex_4, FS_ao).xyz, ao_mask2.xyz))");
        tms_shader_global_define("ENABLE_AO", "1");
    } else {
        tms_shader_global_define_fs("AMBIENT_OCCL", "1.0");
        tms_shader_global_define_fs("AMBIENT_OCCL2", "1.0");
        //tms_shader_global_define("ENABLE_AO", "0");
    }

    if (settings["shadow_ao_combine"]->v.b) {
        tms_shader_global_define("SHADOW_AO_COMBINE", "1");
    }

    if (!dynamic_lighting) {
        char _tmp[32];
        setlocale(LC_ALL, "C");
        setlocale(LC_NUMERIC, "C");
        sprintf(_tmp, "%f", P.default_ambient);
        tms_shader_global_define("_AMBIENT", _tmp);
        setlocale(LC_ALL, "C");
        setlocale(LC_NUMERIC, "C");
        sprintf(_tmp, "%f", P.default_diffuse);
        tms_shader_global_define("_DIFFUSE", _tmp);
    } else {
        tms_shader_global_define("_AMBIENT", "_AMBIENTDIFFUSE.x");
        tms_shader_global_define("_DIFFUSE", "_AMBIENTDIFFUSE.y");
    }

    tms_shader_global_define("AMBIENT_M", ".75");

    tms_material_init(static_cast<tms_material*>(&m_colored));

    tms_assertf((ierr = glGetError()) == 0, "gl error %d before shader compile", ierr);

    tms_infof("Compiling shaders");

    uint32_t global_flags = 0;

    if (enable_gi) {
        global_flags |= GF_ENABLE_GI;
    }

    for (int x=0; x<num_shaders; ++x) {
        struct shader_load_data *sld = &shaders[x];

        char *buf;
        int r;

        tms_debugf("Reading %s vertex shader...", sld->name);
        read_shader(sld, GL_VERTEX_SHADER, global_flags, &buf);
        if (!buf) {
            tms_infof("Falling back, failed to read!");
            *sld->shader = (sld->fallback ? *sld->fallback : 0);
            continue;
        }

        tms_infof("Compiling %s vertex shader...", sld->name);
        //tms_infof("Data: '%s'", buf);
        tms::shader *sh = new tms::shader(sld->name);
        r = sh->compile(GL_VERTEX_SHADER, buf);
        free(buf);
        buf = 0;

        tms_debugf("Reading %s fragment shader...", sld->name);
        read_shader(sld, GL_FRAGMENT_SHADER, global_flags, &buf);
        if (!buf) {
            tms_infof("Falling back, failed to read!");
            *sld->shader = (sld->fallback ? *sld->fallback : 0);
            continue;
        }

        tms_infof("Compiling %s fragment shader...", sld->name);
        //tms_infof("Data: '%s'", buf);
        r = sh->compile(GL_FRAGMENT_SHADER, buf);
        free(buf);
        buf = 0;

        *sld->shader = sh;
    }

    tms::shader *sh;

    sh = new tms::shader("edev");

    if (_tms.gamma_correct) {
        tms_shader_define((struct tms_shader*)sh, "COLOR", "vec4(.07074,.07074,.07074,1.)");
    } else {
        tms_shader_define((struct tms_shader*)sh, "COLOR", "vec4(.3,.3,.3,1.)");
    }
    sh->compile(GL_VERTEX_SHADER, src_constcolored[0]);
    sh->compile(GL_FRAGMENT_SHADER, src_constcolored[1]);
    shader_edev = sh;

    sh = new tms::shader("edev dark");
    if (_tms.gamma_correct) {
        tms_shader_define((struct tms_shader*)sh, "COLOR", "vec4(.02899,.02899,.02899,1.)");
    } else {
        tms_shader_define((struct tms_shader*)sh, "COLOR", "vec4(.2,.2,.2,1.)");
    }
    sh->compile(GL_VERTEX_SHADER, src_constcolored[0]);
    sh->compile(GL_FRAGMENT_SHADER, src_constcolored[1]);
    shader_edev_dark = sh;

    sh = new tms::shader("red");
    if (_tms.gamma_correct) {
        tms_shader_define((struct tms_shader*)sh, "COLOR", "vec4(.45626,.0993,.0993,1.)");
    } else {
        tms_shader_define((struct tms_shader*)sh, "COLOR", "vec4(.7,.35,.35,1.)");
    }
    sh->compile(GL_VERTEX_SHADER, src_constcolored[0]);
    sh->compile(GL_FRAGMENT_SHADER, src_constcolored[1]);
    shader_red = sh;

    sh = new tms::shader("blue");
    if (_tms.gamma_correct) {
        tms_shader_define((struct tms_shader*)sh, "COLOR", "vec4(.1726,.1726,.612065,1.)");
    } else {
        tms_shader_define((struct tms_shader*)sh, "COLOR", "vec4(.45,.45,.8,1.)");
    }
    sh->compile(GL_VERTEX_SHADER, src_constcolored[0]);
    sh->compile(GL_FRAGMENT_SHADER, src_constcolored[1]);
    shader_blue = sh;

    sh = new tms::shader("white");
    if (_tms.gamma_correct) {
        tms_shader_define((struct tms_shader*)sh, "COLOR", "vec4(.612065,.612065,.612065,1.)");
    } else {
        tms_shader_define((struct tms_shader*)sh, "COLOR", "vec4(.8,.8,.8,1.)");
    }
    sh->compile(GL_VERTEX_SHADER, src_constcolored[0]);
    sh->compile(GL_FRAGMENT_SHADER, src_constcolored[1]);
    shader_white = sh;

    sh = new tms::shader("interactive");
    if (_tms.gamma_correct) {
        tms_shader_define((struct tms_shader*)sh, "COLOR", "vec4(.325,.325,.612065,1.)");
    } else {
        tms_shader_define((struct tms_shader*)sh, "COLOR", "vec4(.6,.6,.8,1.)");
    }
    sh->compile(GL_VERTEX_SHADER, src_constcolored[0]);
    sh->compile(GL_FRAGMENT_SHADER, src_constcolored[1]);
    shader_interactive = sh;

    /* menu shaders */

    sh = new tms::shader("edev menu");
    if (_tms.gamma_correct) {
        tms_shader_define((struct tms_shader*)sh, "COLOR", "vec4(.07074,.07074,.07074,1.)");
    } else {
        tms_shader_define((struct tms_shader*)sh, "COLOR", "vec4(.3,.3,.3,1.)");
    }
    sh->compile(GL_VERTEX_SHADER, src_constcolored_m[0]);
    sh->compile(GL_FRAGMENT_SHADER, src_constcolored_m[1]);
    shader_edev_m = sh;

    sh = new tms::shader("edev dark menu");
    if (_tms.gamma_correct) {
        tms_shader_define((struct tms_shader*)sh, "COLOR", "vec4(.02899,.02899,.02899,1.)");
    } else {
        tms_shader_define((struct tms_shader*)sh, "COLOR", "vec4(.2,.2,.2,1.)");
    }
    sh->compile(GL_VERTEX_SHADER, src_constcolored_m[0]);
    sh->compile(GL_FRAGMENT_SHADER, src_constcolored_m[1]);
    shader_edev_dark_m = sh;

    sh = new tms::shader("red menu");
    if (_tms.gamma_correct) {
        tms_shader_define((struct tms_shader*)sh, "COLOR", "vec4(.45626,.0993,.0993,1.)");
    } else {
        tms_shader_define((struct tms_shader*)sh, "COLOR", "vec4(.7,.35,.35,1.)");
    }
    sh->compile(GL_VERTEX_SHADER, src_constcolored_m[0]);
    sh->compile(GL_FRAGMENT_SHADER, src_constcolored_m[1]);
    shader_red_m = sh;

    sh = new tms::shader("blue menu");
    if (_tms.gamma_correct) {
        tms_shader_define((struct tms_shader*)sh, "COLOR", "vec4(.1726,.1726,.612065,1.)");
    } else {
        tms_shader_define((struct tms_shader*)sh, "COLOR", "vec4(.45,.45,.8,1.)");
    }
    sh->compile(GL_VERTEX_SHADER, src_constcolored_m[0]);
    sh->compile(GL_FRAGMENT_SHADER, src_constcolored_m[1]);
    shader_blue_m = sh;

    sh = new tms::shader("white menu");
    if (_tms.gamma_correct) {
        tms_shader_define((struct tms_shader*)sh, "COLOR", "vec4(.612065,.612065,.612065,1.)");
    } else {
        tms_shader_define((struct tms_shader*)sh, "COLOR", "vec4(.8,.8,.8,1.)");
    }
    sh->compile(GL_VERTEX_SHADER, src_constcolored_m[0]);
    sh->compile(GL_FRAGMENT_SHADER, src_constcolored_m[1]);
    shader_white_m = sh;

    sh = new tms::shader("interactive menu");
    if (_tms.gamma_correct) {
        tms_shader_define((struct tms_shader*)sh, "COLOR", "vec4(.325,.325,.612065,1.)");
    } else {
        tms_shader_define((struct tms_shader*)sh, "COLOR", "vec4(.6,.6,.8,1.)");
    }
    sh->compile(GL_VERTEX_SHADER, src_constcolored_m[0]);
    sh->compile(GL_FRAGMENT_SHADER, src_constcolored_m[1]);
    shader_interactive_m = sh;

    setlocale(LC_ALL, "C");
    setlocale(LC_NUMERIC, "C");

    sh = new tms::shader("Menu BG");
    {char tmp[32];
    sprintf(tmp, "vec2(%f,%f)", _tms.window_width / 512.f, _tms.window_height / 512.f);
    tms_shader_define_vs((struct tms_shader*)sh, "SCALE", tmp);}
    sh->compile(GL_VERTEX_SHADER, menu_bgsources[0]);
    sh->compile(GL_FRAGMENT_SHADER, menu_bgsources[1]);
    menu_bg_program = sh->get_program(TMS_NO_PIPELINE);
    menu_bg_color_loc = tms_program_get_uniform(menu_bg_program, "color");

    SN(shader_shiny);
    SN(shader_edev);
    SN(shader_edev_m);
    SN(shader_edev_dark);
    SN(shader_edev_dark_m);
    SN(shader_interactive);
    SN(shader_interactive_m);
    SN(shader_red);
    SN(shader_white);
    SN(shader_blue);
    SN(shader_red_m);
    SN(shader_white_m);
    SN(shader_blue_m);

    tms_infof("Done with shaders!\n");
}

void
material_factory::init_materials()
{
    tms_infof("Initializing materials");

    _tms.gamma_correct = settings["gamma_correct"]->v.b;
    bool shadow_ao_combine = settings["shadow_ao_combine"]->v.b;
    bool enable_gi = false; // settings["enable_gi"]->v.b

    m_bg.pipeline[0].program = shader_bg->get_program(0);
    m_bg.pipeline[1].program = 0;
    m_bg.pipeline[0].texture[0] = tex_bg;

    m_bg2.pipeline[0].program = shader_bg->get_program(0);
    m_bg2.pipeline[1].program = 0;
    m_bg2.pipeline[0].texture[0] = tex_wood;

    m_bg_fixed.pipeline[0].program = shader_bg_fixed->get_program(0);
    m_bg_fixed.pipeline[1].program = 0;
    m_bg_fixed.pipeline[0].texture[0] = tex_bg;

    m_bg_colored.pipeline[0].program = shader_bg_colored->get_program(0);
    m_bg_colored.pipeline[1].program = 0;
    m_bg_colored.pipeline[2].program = 0;
    m_bg_colored.pipeline[3].program = 0;

    m_grid.pipeline[0].program = shader_grid->get_program(0);
    //m_grid.pipeline[0].blend_mode = TMS_BLENDMODE__SRC_ALPHA__ONE;
    //m_grid.pipeline[0].blend_mode = TMS_BLENDMODE__SRC_ALPHA__ONE_MINUS_SRC_ALPHA;
    m_grid.pipeline[0].blend_mode = TMS_BLENDMODE__ONE_MINUS_DST_COLOR__ONE_MINUS_SRC_ALPHA;
    m_grid.pipeline[1].program = 0;
    m_grid.pipeline[0].texture[0] = tex_grid;

    m_border.pipeline[0].program = shader_border->get_program(0);
    m_border.pipeline[1].program = shader_gi->get_program(1);
    m_border.pipeline[2].program = 0;
    if (shadow_ao_combine) {
        m_border.pipeline[3].program = shader_border_ao->get_program(3);
    } else {
        m_border.pipeline[3].program = shader_ao_norot->get_program(3);
    }
    m_border.pipeline[0].texture[0] = tex_border;
    m_border.pipeline[2].texture[0] = tex_border;
    m_border.type = TYPE_WOOD2;

    m_colored.pipeline[0].program = shader_colored->get_program(0);
    m_colored.pipeline[1].program = shader_gi->get_program(1);
    m_colored.pipeline[2].program = shader_pv_colored_m->get_program(2);
    if (shadow_ao_combine) {
        m_colored.pipeline[3].program = shader_ao->get_program(3);
    } else {
        m_colored.pipeline[3].program = shader_ao_norot->get_program(3);
    }
    m_colored.friction = .6f;
    m_colored.density = .5f*M_DENSITY;
    m_colored.restitution = .3f;
    m_colored.type = TYPE_PLASTIC;

    m_gem.pipeline[0].program = shader_gem->get_program(0);
    m_gem.pipeline[1].program = shader_gi->get_program(1);
    m_gem.pipeline[2].program = shader_pv_colored_m->get_program(2);
    if (shadow_ao_combine) {
        m_gem.pipeline[3].program = shader_ao->get_program(3);
    } else {
        m_gem.pipeline[3].program = shader_ao_norot->get_program(3);
    }
    m_gem.friction = .6f;
    m_gem.density = 1.f*M_DENSITY;
    m_gem.restitution = .3f;
    m_gem.type = TYPE_METAL;

    m_rubberband.pipeline[0].program = shader_rubberband->get_program(0);
    m_rubberband.pipeline[1].program = shader_gi->get_program(1);
    m_rubberband.pipeline[2].program = 0;
    m_rubberband.pipeline[3].program = 0;

    m_cable.pipeline[0].program = shader_cable->get_program(0);

    if ((float)settings["shadow_map_resx"]->v.i / (float)_tms.window_width < .7f) {
        /* disable cable shadows if the resolution is too low, it just looks ugly */
        m_cable.pipeline[1].program = 0;
    } else {
        m_cable.pipeline[1].program = shader_gi->get_program(1);
    }

    m_cable.pipeline[2].program = shader_cable->get_program(2);
    m_cable.pipeline[3].program = 0;

    m_pixel.pipeline[0].program = shader_colorbuf->get_program(0);
    m_pixel.pipeline[1].program = shader_gi->get_program(1);
    m_pixel.pipeline[2].program = 0;
    if (shadow_ao_combine) {
        m_pixel.pipeline[3].program = shader_ao->get_program(3);
    } else {
        m_pixel.pipeline[3].program = shader_ao_norot->get_program(3);
    }
    m_pixel.friction = .6f;
    m_pixel.density = .5f*M_DENSITY;
    m_pixel.restitution = .3f;
    m_pixel.type = TYPE_PLASTIC;

    m_cavemask.pipeline[0].program = shader_edev_dark->get_program(0);
    m_cavemask.pipeline[1].program = 0;
    m_cavemask.pipeline[2].program = 0;
    m_cavemask.pipeline[3].program = shader_ao_clear->get_program(3);

    m_pv_colored.pipeline[0].program = shader_pv_colored->get_program(0);
    m_pv_colored.pipeline[1].program = shader_gi_col->get_program(1);
    m_pv_colored.pipeline[2].program = shader_pv_colored_m->get_program(2);
    if (shadow_ao_combine) {
        m_pv_colored.pipeline[3].program = shader_ao->get_program(3);
    } else {
        m_pv_colored.pipeline[3].program = shader_ao_norot->get_program(3);
    }
    m_pv_colored.friction = .6f;
    m_pv_colored.density = .5f*M_DENSITY;
    m_pv_colored.restitution = .3f;
    m_pv_colored.type = TYPE_PLASTIC;

    m_interactive.pipeline[0].program = shader_interactive->get_program(0);
    m_interactive.pipeline[1].program = shader_gi->get_program(1);
    m_interactive.pipeline[2].program = shader_interactive_m->get_program(2);
    if (shadow_ao_combine) {
        m_interactive.pipeline[3].program = shader_ao->get_program(3);
    } else {
        m_interactive.pipeline[3].program = shader_ao_norot->get_program(3);
    }
    m_interactive.friction = .8f;
    m_interactive.density = 1.25f*M_DENSITY;
    m_interactive.restitution = .3f;
    m_interactive.type = TYPE_PLASTIC;

    m_pv_rgba.pipeline[0].program = shader_pv_colored->get_program(0);
    m_pv_rgba.pipeline[0].blend_mode = TMS_BLENDMODE__SRC_ALPHA__ONE_MINUS_SRC_ALPHA;
    //m_pv_rgba.pipeline[1].program = shader_gi->get_program(1);
    m_pv_rgba.pipeline[2].program = shader_pv_colored_m->get_program(2);
    //m_pv_rgba.pipeline[3].program = shader_ao->get_program(3);
    m_pv_rgba.friction = .6f;
    m_pv_rgba.density = .5f*M_DENSITY;
    m_pv_rgba.restitution = .3f;
    m_pv_rgba.type = TYPE_PLASTIC;

    m_rope.pipeline[0].program = shader_pv_textured->get_program(0);
    m_rope.pipeline[1].program = shader_gi->get_program(1);
    m_rope.pipeline[2].program = shader_pv_textured_m->get_program(2);
    m_rope.pipeline[0].texture[0] = static_cast<tms_texture*>(tex_rope);
    m_rope.pipeline[2].texture[0] = static_cast<tms_texture*>(tex_rope);
    m_rope.pipeline[3].program = 0;//shader_ao->get_program(3);

    m_gen.pipeline[0].program = shader_pv_textured_ao->get_program(0);
    m_gen.pipeline[1].program = shader_gi->get_program(1);
    m_gen.pipeline[2].program = shader_pv_textured_m->get_program(2);
    m_gen.pipeline[0].texture[0] = static_cast<tms_texture*>(tex_gen);
    m_gen.pipeline[2].texture[0] = static_cast<tms_texture*>(tex_gen);
    if (shadow_ao_combine) {
        m_gen.pipeline[3].program = shader_ao->get_program(3);
    } else {
        m_gen.pipeline[3].program = shader_ao_norot->get_program(3);
    }
    m_gen.friction = .5f;
    m_gen.density = 2.0f*M_DENSITY;
    m_gen.restitution = .1f;
    m_gen.type = TYPE_METAL;

    m_battery.pipeline[0].program = shader_pv_textured_ao->get_program(0);
    m_battery.pipeline[1].program = shader_gi->get_program(1);
    m_battery.pipeline[2].program = shader_pv_textured_m->get_program(2);
    m_battery.pipeline[0].texture[0] = static_cast<tms_texture*>(tex_battery);
    m_battery.pipeline[2].texture[0] = static_cast<tms_texture*>(tex_battery);
    if (shadow_ao_combine) {
        m_battery.pipeline[3].program = shader_ao->get_program(3);
    } else {
        m_battery.pipeline[3].program = shader_ao_norot->get_program(3);
    }
    m_battery.friction = .5f;
    m_battery.density = 2.0f*M_DENSITY;
    m_battery.restitution = .1f;
    m_battery.type = TYPE_METAL;

    m_motor.pipeline[0].program = shader_pv_textured_ao->get_program(0);
    m_motor.pipeline[1].program = shader_gi->get_program(1);
    m_motor.pipeline[2].program = shader_pv_textured_m->get_program(2);
    m_motor.pipeline[0].texture[0] = static_cast<tms_texture*>(tex_motor);
    m_motor.pipeline[2].texture[0] = static_cast<tms_texture*>(tex_motor);
    if (shadow_ao_combine) {
        m_motor.pipeline[3].program = shader_ao->get_program(3);
    } else {
        m_motor.pipeline[3].program = shader_ao_norot->get_program(3);
    }
    m_motor.friction = .5f;
    m_motor.density = 2.0f*M_DENSITY;
    m_motor.restitution = .1f;
    m_motor.type = TYPE_METAL;

    m_room.pipeline[0].program = shader_breadboard->get_program(0);
    m_room.pipeline[1].program = shader_gi->get_program(1);
    m_room.pipeline[2].program = shader_pv_textured_m->get_program(2);
    m_room.pipeline[0].texture[0] = static_cast<tms_texture*>(tex_breadboard);
    m_room.pipeline[2].texture[0] = static_cast<tms_texture*>(tex_breadboard);
    m_room.pipeline[3].program = 0;//shader_ao->get_program(3);
    m_room.friction = .9f;
    m_room.density = .025f*M_DENSITY;
    m_room.restitution = .05f;
    m_room.type = TYPE_WOOD2;

    m_breadboard.pipeline[0].program = shader_breadboard->get_program(0);
    m_breadboard.pipeline[1].program = shader_gi->get_program(1);
    m_breadboard.pipeline[2].program = shader_pv_textured_m->get_program(2);
    m_breadboard.pipeline[0].texture[0] = static_cast<tms_texture*>(tex_breadboard);
    m_breadboard.pipeline[2].texture[0] = static_cast<tms_texture*>(tex_breadboard);
    m_breadboard.pipeline[3].program = 0;//shader_ao->get_program(3);
    m_breadboard.friction = .7f;
    m_breadboard.density = .1f*M_DENSITY;
    m_breadboard.restitution = .4f;
    m_breadboard.type = TYPE_WOOD2;

    m_ladder.pipeline[0].program = shader_pv_textured->get_program(0);
    m_ladder.pipeline[1].program = shader_gi->get_program(1);
    m_ladder.pipeline[2].program = shader_pv_textured_m->get_program(2);
    m_ladder.pipeline[0].texture[0] = static_cast<tms_texture*>(tex_breadboard);
    m_ladder.pipeline[2].texture[0] = static_cast<tms_texture*>(tex_breadboard);
    if (shadow_ao_combine) {
        m_ladder.pipeline[3].program = shader_ao->get_program(3);
    } else {
        m_ladder.pipeline[3].program = shader_ao_norot->get_program(3);
    }
    m_ladder.friction = .7f;
    m_ladder.density = .8f;
    m_ladder.restitution = .1f;
    m_ladder.type = TYPE_WOOD;

    m_wood.pipeline[0].program = shader_pv_textured->get_program(0);
    m_wood.pipeline[1].program = shader_gi->get_program(1);
    m_wood.pipeline[2].program = shader_pv_textured_m->get_program(2);
    m_wood.pipeline[0].texture[0] = static_cast<tms_texture*>(tex_wood);
    if (enable_gi) m_wood.pipeline[1].texture[0] = static_cast<tms_texture*>(tex_wood);
    m_wood.pipeline[2].texture[0] = static_cast<tms_texture*>(tex_wood);
    if (shadow_ao_combine) {
        m_wood.pipeline[3].program = shader_ao->get_program(3);
    } else {
        m_wood.pipeline[3].program = shader_ao_norot->get_program(3);
    }
    m_wood.friction = .6f;
    m_wood.density = .5f*M_DENSITY;
    m_wood.restitution = .3f;
    m_wood.type = TYPE_WOOD;

    m_tpixel.pipeline[0].program = shader_pv_textured->get_program(0);
    m_tpixel.pipeline[1].program = shader_gi_tex->get_program(1);
    m_tpixel.pipeline[2].program = shader_pv_textured_m->get_program(2);
    m_tpixel.pipeline[0].texture[0] = static_cast<tms_texture*>(tex_tpixel);
    if (enable_gi) m_tpixel.pipeline[1].texture[0] = static_cast<tms_texture*>(tex_tpixel);
    m_tpixel.pipeline[2].texture[0] = static_cast<tms_texture*>(tex_tpixel);
    if (shadow_ao_combine) {
        m_tpixel.pipeline[3].program = shader_ao->get_program(3);
    } else {
        m_tpixel.pipeline[3].program = shader_ao_norot->get_program(3);
    }
    m_tpixel.friction = .6f;
    m_tpixel.density = .5f*M_DENSITY;
    m_tpixel.restitution = .3f;
    m_tpixel.type = TYPE_PLASTIC;

    m_grass.pipeline[0].program = shader_grass->get_program(0);
    m_grass.pipeline[1].program = 0;
    m_grass.pipeline[2].program = 0;
    m_grass.pipeline[0].texture[0] = static_cast<tms_texture*>(tex_grass);
    m_grass.pipeline[3].program = 0;
    m_grass.pipeline[0].blend_mode = TMS_BLENDMODE__SRC_ALPHA__ONE_MINUS_SRC_ALPHA;

    m_weight.pipeline[0].program = shader_pv_colored->get_program(0);
    m_weight.pipeline[1].program = shader_gi_col->get_program(1);
    m_weight.pipeline[2].program = shader_pv_colored_m->get_program(2);
    if (shadow_ao_combine) {
        m_weight.pipeline[3].program = shader_ao->get_program(3);
    } else {
        m_weight.pipeline[3].program = shader_ao_norot->get_program(3);
    }
    m_weight.friction = .6f;
    m_weight.density = 25.0f*M_DENSITY; /* XXX: Should this density really be used? */
    m_weight.restitution = .3f;
    m_weight.type = TYPE_METAL2;

    m_bedrock.pipeline[0].program = shader_pv_textured->get_program(0);
    m_bedrock.pipeline[1].program = 0;
    m_bedrock.pipeline[2].program = 0;
    m_bedrock.pipeline[0].texture[0] = static_cast<tms_texture*>(tex_bedrock);
    if (shadow_ao_combine) {
        m_bedrock.pipeline[3].program = shader_border_ao->get_program(3);
    } else {
        m_bedrock.pipeline[3].program = shader_ao_norot->get_program(3);
    }
    m_bedrock.friction = 1.75f;
    m_bedrock.density = .75f*M_DENSITY;
    m_bedrock.type = TYPE_RUBBER;

    m_bark.pipeline[0].program = shader_pv_textured->get_program(0);
    m_bark.pipeline[1].program = shader_gi->get_program(1);
    m_bark.pipeline[2].program = shader_pv_textured_m->get_program(2);
    m_bark.pipeline[0].texture[0] = static_cast<tms_texture*>(tex_bark);
    m_bark.pipeline[2].texture[0] = static_cast<tms_texture*>(tex_bark);
    if (shadow_ao_combine) {
        m_bark.pipeline[3].program = shader_ao->get_program(3);
    } else {
        m_bark.pipeline[3].program = shader_ao_norot->get_program(3);
    }
    m_bark.friction = .9f;
    m_bark.density = .75f*M_DENSITY;
    m_bark.restitution = .1f;

    m_rubber.pipeline[0].program = shader_pv_textured->get_program(0);
    m_rubber.pipeline[1].program = shader_gi->get_program(1);
    m_rubber.pipeline[2].program = shader_pv_textured_m->get_program(2);
    m_rubber.pipeline[0].texture[0] = static_cast<tms_texture*>(tex_rubber);
    m_rubber.pipeline[2].texture[0] = static_cast<tms_texture*>(tex_rubber);
    if (shadow_ao_combine) {
        m_rubber.pipeline[3].program = shader_ao->get_program(3);
    } else {
        m_rubber.pipeline[3].program = shader_ao_norot->get_program(3);
    }
    m_rubber.friction = 1.75f;
    m_rubber.density = .75f*M_DENSITY;
    m_rubber.restitution = .5f;
    m_rubber.type = TYPE_RUBBER;

    m_metal.pipeline[0].program = shader_pv_textured->get_program(0);/* XXX */
    m_metal.pipeline[1].program = shader_gi->get_program(1);
    m_metal.pipeline[2].program = shader_pv_textured_m->get_program(2);
    m_metal.pipeline[0].texture[0] = static_cast<tms_texture*>(tex_metal);
    m_metal.pipeline[2].texture[0] = static_cast<tms_texture*>(tex_metal);
    if (shadow_ao_combine) {
        m_metal.pipeline[3].program = shader_ao->get_program(3);
    } else {
        m_metal.pipeline[3].program = shader_ao_norot->get_program(3);
    }
    m_metal.friction = .2f;
    m_metal.density = 1.0f*M_DENSITY;
    m_metal.restitution = .4f;
    m_metal.type = TYPE_METAL;

    m_angulardamper.pipeline[0].program = shader_pv_textured->get_program(0);/* XXX */
    m_angulardamper.pipeline[1].program = shader_gi->get_program(1);
    m_angulardamper.pipeline[2].program = shader_pv_textured_m->get_program(2);
    m_angulardamper.pipeline[0].texture[0] = static_cast<tms_texture*>(tex_metal);
    m_angulardamper.pipeline[2].texture[0] = static_cast<tms_texture*>(tex_metal);
    if (shadow_ao_combine) {
        m_angulardamper.pipeline[3].program = shader_ao->get_program(3);
    } else {
        m_angulardamper.pipeline[3].program = shader_ao_norot->get_program(3);
    }
    m_angulardamper.friction = .2f;
    m_angulardamper.density = 1.0f*M_DENSITY;
    m_angulardamper.restitution = 1.0f;

    m_iron.pipeline[0].program = shader_textured->get_program(0);
    m_iron.pipeline[1].program = shader_gi->get_program(1);
    m_iron.pipeline[2].program = shader_pv_textured_m->get_program(2);
    m_iron.pipeline[0].texture[0] = static_cast<tms_texture*>(tex_metal);
    m_iron.pipeline[2].texture[0] = static_cast<tms_texture*>(tex_metal);
    if (shadow_ao_combine) {
        m_iron.pipeline[3].program = shader_ao->get_program(3);
    } else {
        m_iron.pipeline[3].program = shader_ao_norot->get_program(3);
    }
    m_iron.friction = 0.5f;
    m_iron.density = 4.f*M_DENSITY;
    m_iron.restitution = .6f; /* TODO: previous: .9f */
    m_iron.type = TYPE_METAL;

    m_rail.pipeline[0].program = shader_textured->get_program(0);
    m_rail.pipeline[1].program = shader_gi->get_program(1);
    m_rail.pipeline[2].program = shader_pv_textured_m->get_program(2);
    m_rail.pipeline[0].texture[0] = static_cast<tms_texture*>(tex_metal);
    m_rail.pipeline[2].texture[0] = static_cast<tms_texture*>(tex_metal);
    if (shadow_ao_combine) {
        m_rail.pipeline[3].program = shader_ao->get_program(3);
    } else {
        m_rail.pipeline[3].program = shader_ao_norot->get_program(3);
    }
    m_rail.friction = 0.1f;
    m_rail.density = 1.f*M_DENSITY;
    m_rail.restitution = .0f;
    m_rail.type = TYPE_METAL2;

    m_cpad.pipeline[0].program = shader_pv_textured->get_program(0);/* XXX */
    m_cpad.pipeline[1].program = shader_gi->get_program(1);
    m_cpad.pipeline[2].program = shader_pv_textured_m->get_program(2);
    m_cpad.pipeline[0].texture[0] = static_cast<tms_texture*>(tex_cpad); /* XXX */
    m_cpad.pipeline[2].texture[0] = static_cast<tms_texture*>(tex_cpad); /* XXX */
    if (shadow_ao_combine) {
        m_cpad.pipeline[3].program = shader_ao->get_program(3);
    } else {
        m_cpad.pipeline[3].program = shader_ao_norot->get_program(3);
    }
    m_cpad.friction = .8f;
    m_cpad.density = 1.0f*M_DENSITY;
    m_cpad.restitution = .1f;
    m_cpad.type = TYPE_PLASTIC;

    m_rocket.pipeline[0].program = shader_pv_textured->get_program(0);/* XXX */
    m_rocket.pipeline[1].program = shader_gi->get_program(1);
    m_rocket.pipeline[2].program = shader_pv_textured_m->get_program(2);
    m_rocket.pipeline[0].texture[0] = static_cast<tms_texture*>(tex_metal); /* XXX */
    m_rocket.pipeline[2].texture[0] = static_cast<tms_texture*>(tex_metal); /* XXX */
    if (shadow_ao_combine) {
        m_rocket.pipeline[3].program = shader_ao->get_program(3);
    } else {
        m_rocket.pipeline[3].program = shader_ao_norot->get_program(3);
    }
    m_rocket.friction = .2f;
    m_rocket.density = 1.0f*M_DENSITY;
    m_rocket.restitution = .005f;
    m_rocket.type = TYPE_METAL;

    m_plastic.pipeline[0].program = shader_pv_colored->get_program(0);
    m_plastic.pipeline[1].program = shader_gi_col->get_program(1);
    m_plastic.pipeline[2].program = shader_pv_colored_m->get_program(2);
    if (shadow_ao_combine) {
        m_plastic.pipeline[3].program = shader_ao->get_program(3);
    } else {
        m_plastic.pipeline[3].program = shader_ao_norot->get_program(3);
    }

    m_plastic.friction = .4f;
    m_plastic.density = 1.35f*M_DENSITY;
    m_plastic.restitution = .2f;
    m_plastic.type = TYPE_PLASTIC;

    m_bullet.pipeline[0].program = shader_edev_dark->get_program(0);
    m_bullet.pipeline[1].program = shader_gi->get_program(1);
    m_bullet.pipeline[2].program = 0;
    if (shadow_ao_combine) {
        m_bullet.pipeline[3].program = shader_ao->get_program(3);
    } else {
        m_bullet.pipeline[3].program = shader_ao_norot->get_program(3);
    }
    m_bullet.friction = .4f;
    m_bullet.density = 1.35f*M_DENSITY;
    m_bullet.restitution = .2f;
    m_bullet.type = TYPE_METAL;

    m_pellet.pipeline[0].program = shader_edev_dark->get_program(0);
    m_pellet.pipeline[1].program = shader_gi->get_program(1);
    m_pellet.pipeline[2].program = 0;
    if (shadow_ao_combine) {
        m_pellet.pipeline[3].program = shader_ao->get_program(3);
    } else {
        m_pellet.pipeline[3].program = shader_ao_norot->get_program(3);
    }
    m_pellet.friction = .4f;
    m_pellet.density = 4.f*M_DENSITY;
    m_pellet.restitution = .2f;
    m_pellet.type = TYPE_METAL;

    m_magnet.pipeline[0].program = shader_textured->get_program(0);
    m_magnet.pipeline[1].program = shader_gi->get_program(1);
    m_magnet.pipeline[2].program = shader_pv_textured_m->get_program(2);
    m_magnet.pipeline[0].texture[0] = static_cast<tms_texture*>(tex_magnet);
    m_magnet.pipeline[2].texture[0] = static_cast<tms_texture*>(tex_magnet);
    if (shadow_ao_combine) {
        m_magnet.pipeline[3].program = shader_ao->get_program(3);
    } else {
        m_magnet.pipeline[3].program = shader_ao_norot->get_program(3);
    }
    m_magnet.friction = 4.f;
    m_magnet.density = 3.f*M_DENSITY;
    m_magnet.restitution = .0f;
    m_magnet.type = TYPE_METAL;

    if (!(m_gear.pipeline[0].program = shader_shiny->get_program(0)))
        m_gear.pipeline[0].program = shader_pv_textured->get_program(0);
    m_gear.pipeline[1].program = shader_gi->get_program(1);
    m_gear.pipeline[2].program = shader_pv_textured_m->get_program(2);
    m_gear.pipeline[0].texture[0] = static_cast<tms_texture*>(tex_gear);
    m_gear.pipeline[0].texture[1] = static_cast<tms_texture*>(tex_reflection);
    m_gear.pipeline[2].texture[0] = static_cast<tms_texture*>(tex_gear);
    if (shadow_ao_combine) {
        m_gear.pipeline[3].program = shader_ao->get_program(3);
    } else {
        m_gear.pipeline[3].program = shader_ao_norot->get_program(3);
    }
    m_gear.friction = 2.f;
    m_gear.density = .2f*M_DENSITY;
    m_gear.restitution = .1f;

    m_gear_ao.pipeline[0].program = shader_pv_textured_ao->get_program(0);
    m_gear_ao.pipeline[1].program = shader_gi->get_program(1);
    m_gear_ao.pipeline[2].program = shader_pv_textured_m->get_program(2);
    m_gear_ao.pipeline[0].texture[0] = static_cast<tms_texture*>(tex_rackhouse);
    m_gear_ao.pipeline[0].texture[1] = static_cast<tms_texture*>(tex_reflection);
    m_gear_ao.pipeline[2].texture[0] = static_cast<tms_texture*>(tex_gear);
    if (shadow_ao_combine) {
        m_gear_ao.pipeline[3].program = shader_ao->get_program(3);
    } else {
        m_gear_ao.pipeline[3].program = shader_ao_norot->get_program(3);
    }
    m_gear_ao.friction = m_gear.friction;
    m_gear_ao.density = m_gear.density*M_DENSITY;
    m_gear_ao.restitution = m_gear.restitution;

    m_mpanel.pipeline[0].program = shader_pv_textured_ao->get_program(0);
    m_mpanel.pipeline[1].program = shader_gi->get_program(1);
    m_mpanel.pipeline[2].program = shader_pv_textured_m->get_program(2);
    m_mpanel.pipeline[0].texture[0] = static_cast<tms_texture*>(tex_mpanel);
    m_mpanel.pipeline[2].texture[0] = static_cast<tms_texture*>(tex_mpanel);
    if (shadow_ao_combine) {
        m_mpanel.pipeline[3].program = shader_ao->get_program(3);
    } else {
        m_mpanel.pipeline[3].program = shader_ao_norot->get_program(3);
    }
    m_mpanel.friction = .5f;
    m_mpanel.density = .7f*M_DENSITY;
    m_mpanel.restitution = .1f;
    m_mpanel.type = TYPE_PLASTIC;

    m_bigpanel.pipeline[0].program = shader_pv_textured_ao->get_program(0);
    m_bigpanel.pipeline[1].program = shader_gi->get_program(1);
    m_bigpanel.pipeline[2].program = shader_pv_textured_m->get_program(2);
    m_bigpanel.pipeline[0].texture[0] = static_cast<tms_texture*>(tex_bigpanel);
    m_bigpanel.pipeline[2].texture[0] = static_cast<tms_texture*>(tex_bigpanel);
    if (shadow_ao_combine) {
        m_bigpanel.pipeline[3].program = shader_ao->get_program(3);
    } else {
        m_bigpanel.pipeline[3].program = shader_ao_norot->get_program(3);
    }
    m_bigpanel.friction = m_mpanel.friction;
    m_bigpanel.density = m_mpanel.density;
    m_bigpanel.restitution = m_mpanel.restitution;
    m_bigpanel.type = TYPE_PLASTIC;

    m_factory.pipeline[0].program = shader_pv_textured_ao->get_program(0);
    m_factory.pipeline[1].program = shader_gi->get_program(1);
    m_factory.pipeline[2].program = shader_pv_textured_m->get_program(2);
    m_factory.pipeline[0].texture[0] = static_cast<tms_texture*>(tex_factory);
    m_factory.pipeline[2].texture[0] = static_cast<tms_texture*>(tex_factory);
    if (shadow_ao_combine) {
        m_factory.pipeline[3].program = shader_ao->get_program(3);
    } else {
        m_factory.pipeline[3].program = shader_ao_norot->get_program(3);
    }
    m_factory.friction = m_mpanel.friction;
    m_factory.density = m_mpanel.density;
    m_factory.restitution = .0125f;
    m_factory.type = TYPE_PLASTIC;

    m_iomisc.pipeline[0].program = shader_pv_textured_ao->get_program(0);
    m_iomisc.pipeline[1].program = shader_gi->get_program(1);
    m_iomisc.pipeline[2].program = shader_pv_textured_m->get_program(2);
    m_iomisc.pipeline[0].texture[0] = static_cast<tms_texture*>(tex_iomisc);
    m_iomisc.pipeline[2].texture[0] = static_cast<tms_texture*>(tex_iomisc);
    if (shadow_ao_combine) {
        m_iomisc.pipeline[3].program = shader_ao->get_program(3);
    } else {
        m_iomisc.pipeline[3].program = shader_ao_norot->get_program(3);
    }
    m_iomisc.friction = m_edev.friction;
    m_iomisc.density = m_edev.density;
    m_iomisc.restitution = m_edev.restitution;
    m_iomisc.type = m_edev.type;

    m_smallpanel.pipeline[0].program = shader_pv_textured_ao->get_program(0);
    m_smallpanel.pipeline[1].program = shader_gi->get_program(1);
    m_smallpanel.pipeline[2].program = shader_pv_textured_m->get_program(2);
    m_smallpanel.pipeline[0].texture[0] = static_cast<tms_texture*>(tex_smallpanel);
    m_smallpanel.pipeline[2].texture[0] = static_cast<tms_texture*>(tex_smallpanel);
    if (shadow_ao_combine) {
        m_smallpanel.pipeline[3].program = shader_ao->get_program(3);
    } else {
        m_smallpanel.pipeline[3].program = shader_ao_norot->get_program(3);
    }
    m_smallpanel.friction = m_mpanel.friction;
    m_smallpanel.density = m_mpanel.density;
    m_smallpanel.restitution = m_mpanel.restitution;
    m_smallpanel.type = TYPE_PLASTIC;

    m_misc.pipeline[0].program = shader_pv_textured_ao->get_program(0);
    m_misc.pipeline[1].program = shader_gi->get_program(1);
    m_misc.pipeline[2].program = shader_pv_textured_m->get_program(2);
    m_misc.pipeline[0].texture[0] = static_cast<tms_texture*>(tex_misc);
    m_misc.pipeline[2].texture[0] = static_cast<tms_texture*>(tex_misc);
    if (shadow_ao_combine) {
        m_misc.pipeline[3].program = shader_ao->get_program(3);
    } else {
        m_misc.pipeline[3].program = shader_ao_norot->get_program(3);
    }
    m_misc.friction = .5f;
    m_misc.density = .7f*M_DENSITY;
    m_misc.restitution = .3f;
    m_misc.type = TYPE_PLASTIC;

    m_robot.pipeline[0].program = shader_pv_textured_ao->get_program(0);
    //m_robot.pipeline[0].program = shader_colored->get_program(0);
    m_robot.pipeline[1].program = shader_gi->get_program(1);
    m_robot.pipeline[2].program = shader_pv_textured_m->get_program(2);
    m_robot.pipeline[0].texture[0] = static_cast<tms_texture*>(tex_robot);
    m_robot.pipeline[2].texture[0] = static_cast<tms_texture*>(tex_robot);
    if (shadow_ao_combine) {
        m_robot.pipeline[3].program = 0;
    } else {
        m_robot.pipeline[3].program = shader_ao_norot->get_program(3);
    }
    m_robot.friction = .7f;
    m_robot.density = .5f*M_DENSITY*ROBOT_DENSITY_MUL;
    m_robot.restitution = .1f;
    m_robot.type = TYPE_SHEET_METAL;

    m_weapon.pipeline[0].program = shader_textured_ao->get_program(0);
    m_weapon.pipeline[1].program = shader_gi->get_program(1);
    m_weapon.pipeline[2].program = shader_pv_textured_m->get_program(2);
    m_weapon.pipeline[0].texture[0] = static_cast<tms_texture*>(tex_weapons);
    m_weapon.pipeline[2].texture[0] = static_cast<tms_texture*>(tex_weapons);
    m_weapon.pipeline[3].program = m_robot.pipeline[3].program;
    m_weapon.friction = .5f;
    m_weapon.density = 2.f;
    m_weapon.restitution = .1f;
    m_weapon.type = TYPE_METAL;

    m_weapon_nospecular = m_weapon;
    m_weapon_nospecular.pipeline[0].program = shader_pv_textured_ao->get_program(0);

    m_animal.pipeline[0].program = shader_pv_textured_ao->get_program(0);
    m_animal.pipeline[1].program = shader_gi->get_program(1);
    m_animal.pipeline[2].program = shader_pv_textured_m->get_program(2);
    m_animal.pipeline[0].texture[0] = static_cast<tms_texture*>(tex_animal);
    m_animal.pipeline[2].texture[0] = static_cast<tms_texture*>(tex_animal);
    m_animal.pipeline[3].program = m_robot.pipeline[3].program;
    m_animal.friction = .7f;
    m_animal.density = .8f*M_DENSITY*ROBOT_DENSITY_MUL;
    m_animal.restitution = .1f;
    m_animal.type = TYPE_SHEET_METAL;

    m_leaves.pipeline[0].program = shader_pv_textured_ao_tinted->get_program(0);
    m_leaves.pipeline[1].program = shader_gi->get_program(1);
    m_leaves.pipeline[2].program = shader_pv_textured_m->get_program(2);
    m_leaves.pipeline[0].texture[0] = static_cast<tms_texture*>(tex_leaves);
    m_leaves.pipeline[2].texture[0] = static_cast<tms_texture*>(tex_leaves);
    m_leaves.pipeline[3].program = shader_ao_norot->get_program(3);
    m_leaves.friction = .7f;
    m_leaves.density = .25f;
    m_leaves.restitution = 0.f;
    m_leaves.type = 0;

    m_robot_tinted.pipeline[0].program = shader_pv_textured_ao_tinted->get_program(0);
    m_robot_tinted.pipeline[1].program = shader_gi->get_program(1);
    m_robot_tinted.pipeline[2].program = shader_pv_textured_m->get_program(2);
    m_robot_tinted.pipeline[0].texture[0] = static_cast<tms_texture*>(tex_robot);
    m_robot_tinted.pipeline[2].texture[0] = static_cast<tms_texture*>(tex_robot);
    m_robot_tinted.pipeline[3].program = m_robot.pipeline[3].program;
    m_robot_tinted.friction = m_robot.friction;
    m_robot_tinted.density = m_robot.density;
    m_robot_tinted.restitution = m_robot.restitution;
    m_robot_tinted.type = TYPE_SHEET_METAL;

    m_robot_skeleton.pipeline[0].program = shader_textured->get_program(0);
    m_robot_skeleton.pipeline[1].program = shader_gi->get_program(1);
    m_robot_skeleton.pipeline[2].program = shader_textured->get_program(2);
    m_robot_skeleton.pipeline[0].texture[0] = static_cast<tms_texture*>(tex_items);
    m_robot_skeleton.pipeline[2].texture[0] = static_cast<tms_texture*>(tex_items);
    m_robot_skeleton.pipeline[3].program = m_robot.pipeline[3].program;
    m_robot_skeleton.friction = m_robot.friction;
    m_robot_skeleton.density = m_robot.density;
    m_robot_skeleton.restitution = m_robot.restitution;
    m_robot_skeleton.type = TYPE_SHEET_METAL;

    m_robot_head.pipeline[0].program = shader_pv_colored->get_program(0);
    m_robot_head.pipeline[1].program = shader_gi->get_program(1);
    m_robot_head.pipeline[2].program = shader_pv_colored_m->get_program(2);
    m_robot_head.pipeline[3].program = m_robot.pipeline[3].program;
    m_robot_head.friction = .5f;
    m_robot_head.density = .3f*M_DENSITY*ROBOT_DENSITY_MUL;
    m_robot_head.restitution = .1f;
    m_robot_head.type = TYPE_SHEET_METAL;

    m_robot_arm.pipeline[0].program = shader_pv_colored->get_program(0);
    m_robot_arm.pipeline[1].program = shader_gi->get_program(1);
    m_robot_arm.pipeline[2].program = shader_pv_colored_m->get_program(2);
    if (shadow_ao_combine) {
        m_robot_arm.pipeline[3].program = 0;
    } else {
        m_robot_arm.pipeline[3].program = shader_ao_norot->get_program(3);
    }
    m_robot_arm.friction = .5f;
    m_robot_arm.density = .5f*M_DENSITY;
    m_robot_arm.restitution = .1f;
    m_robot_arm.type = TYPE_SHEET_METAL;

    m_robot_leg.pipeline[0].program = shader_pv_colored->get_program(0);
    m_robot_leg.pipeline[1].program = shader_gi->get_program(1);
    m_robot_leg.pipeline[2].program = shader_pv_colored_m->get_program(2);
    if (shadow_ao_combine) {
        m_robot_leg.pipeline[3].program = 0;
    } else {
        m_robot_leg.pipeline[3].program = shader_ao_norot->get_program(3);
    }
    m_robot_leg.friction = .5f;
    m_robot_leg.density = .5f*M_DENSITY;
    m_robot_leg.restitution = .1f;
    m_robot_leg.type = TYPE_SHEET_METAL;

    m_robot_foot.pipeline[0].program = shader_pv_colored->get_program(0);
    m_robot_foot.pipeline[1].program = shader_gi->get_program(1);
    m_robot_foot.pipeline[2].program = shader_pv_colored_m->get_program(2);
    if (shadow_ao_combine) {
        m_robot_foot.pipeline[3].program = 0;
    } else {
        m_robot_foot.pipeline[3].program = shader_ao_norot->get_program(3);
    }
    m_robot_foot.friction = 20.f;
    m_robot_foot.density = 0.5f*M_DENSITY*ROBOT_DENSITY_MUL;
    m_robot_foot.restitution = .0f;
    m_robot_foot.type = TYPE_SHEET_METAL;

    m_i2o1.pipeline[0].program = shader_pv_textured_ao->get_program(0);
    m_i2o1.pipeline[1].program = shader_gi->get_program(1);
    m_i2o1.pipeline[2].program = shader_pv_textured_m->get_program(2);
    m_i2o1.pipeline[0].texture[0] = static_cast<tms_texture*>(tex_i2o1);
    m_i2o1.pipeline[2].texture[0] = static_cast<tms_texture*>(tex_i2o1);
    if (shadow_ao_combine) {
        m_i2o1.pipeline[3].program = shader_ao->get_program(3);
    } else {
        m_i2o1.pipeline[3].program = shader_ao_norot->get_program(3);
    }
    m_i2o1.friction = .5f;
    m_i2o1.density = .5f*M_DENSITY;
    m_i2o1.restitution = .2f;
    m_i2o1.type = TYPE_PLASTIC;

    m_i1o1.pipeline[0].program = shader_pv_textured_ao->get_program(0);
    m_i1o1.pipeline[1].program = shader_gi->get_program(1);
    m_i1o1.pipeline[2].program = shader_pv_textured_m->get_program(2);
    m_i1o1.pipeline[0].texture[0] = static_cast<tms_texture*>(tex_i1o1);
    m_i1o1.pipeline[2].texture[0] = static_cast<tms_texture*>(tex_i1o1);
    if (shadow_ao_combine) {
        m_i1o1.pipeline[3].program = shader_ao->get_program(3);
    } else {
        m_i1o1.pipeline[3].program = shader_ao_norot->get_program(3);
    }
    m_i1o1.friction = .5f;
    m_i1o1.density = .5f*M_DENSITY;
    m_i1o1.restitution = .2f;
    m_i1o1.type = TYPE_PLASTIC;

    /* TODO: use src_constcolored */
    m_conn.pipeline[0].program = shader_edev->get_program(0);
    m_conn.pipeline[1].program = shader_gi->get_program(1);
    m_conn.pipeline[2].program = 0;
    m_conn.pipeline[3].program = shader_ao_bias->get_program(3);

    m_conn_no_ao.pipeline[0].program = shader_edev->get_program(0);
    m_conn_no_ao.pipeline[1].program = shader_gi->get_program(1);
    m_conn_no_ao.pipeline[2].program = 0;
    m_conn_no_ao.pipeline[3].program = 0;

    m_red.pipeline[0].program = shader_red->get_program(0);
    m_red.pipeline[1].program = shader_gi->get_program(1);
    m_red.pipeline[2].program = shader_red_m->get_program(2);
    if (shadow_ao_combine) {
        m_red.pipeline[3].program = shader_ao->get_program(3);
    } else {
        m_red.pipeline[3].program = shader_ao_norot->get_program(3);
    }
    m_red.friction = .5f;
    m_red.density = .5f*M_DENSITY;
    m_red.restitution = .5f;
    m_red.type = TYPE_PLASTIC;

    m_cable_red.pipeline[0].program = shader_red->get_program(0);
    m_cable_red.pipeline[1].program = shader_gi->get_program(1);
    m_cable_red.pipeline[2].program = shader_red_m->get_program(2);

    m_cable_black.pipeline[0].program = shader_white->get_program(0);
    m_cable_black.pipeline[1].program = shader_gi->get_program(1);
    m_cable_black.pipeline[2].program = shader_white_m->get_program(2);

    m_cable_blue.pipeline[0].program = shader_blue->get_program(0);
    m_cable_blue.pipeline[1].program = shader_gi->get_program(1);
    m_cable_blue.pipeline[2].program = shader_blue_m->get_program(2);

    m_heavyedev.pipeline[0].program = shader_edev->get_program(0);
    m_heavyedev.pipeline[1].program = shader_gi->get_program(1);
    m_heavyedev.pipeline[2].program = shader_edev_m->get_program(2);
    if (shadow_ao_combine) {
        m_heavyedev.pipeline[3].program = shader_ao->get_program(3);
    } else {
        m_heavyedev.pipeline[3].program = shader_ao_norot->get_program(3);
    }
    m_heavyedev.friction = .2f;
    m_heavyedev.density = 1.0f*M_DENSITY;
    m_heavyedev.restitution = .4f;
    m_heavyedev.type = TYPE_PLASTIC;

    m_edev.pipeline[0].program = shader_edev->get_program(0);
    m_edev.pipeline[1].program = shader_gi->get_program(1);
    m_edev.pipeline[2].program = shader_edev_m->get_program(2);
    if (shadow_ao_combine) {
        m_edev.pipeline[3].program = shader_ao->get_program(3);
    } else {
        m_edev.pipeline[3].program = shader_ao_norot->get_program(3);
    }
    m_edev.friction = .5f;
    m_edev.density = .5f*M_DENSITY;
    m_edev.restitution = .2f;
    m_edev.type = TYPE_PLASTIC;

    m_edev_dark.pipeline[0].program = shader_edev_dark->get_program(0);
    m_edev_dark.pipeline[1].program = shader_gi->get_program(1);
    m_edev_dark.pipeline[2].program = shader_edev_dark_m->get_program(2);
    if (shadow_ao_combine) {
        m_edev_dark.pipeline[3].program = shader_ao->get_program(3);
    } else {
        m_edev_dark.pipeline[3].program = shader_ao_norot->get_program(3);
    }
    m_edev_dark.friction = .5f;
    m_edev_dark.density = .5f*M_DENSITY;
    m_edev_dark.restitution = .2f;
    m_edev_dark.type = TYPE_PLASTIC;

    m_spikes.pipeline[0].program = shader_edev->get_program(0);
    m_spikes.pipeline[1].program = shader_gi->get_program(1);
    m_spikes.pipeline[2].program = shader_edev_m->get_program(2);
    if (shadow_ao_combine) {
        m_spikes.pipeline[3].program = shader_ao->get_program(3);
    } else {
        m_spikes.pipeline[3].program = shader_ao_norot->get_program(3);
    }
    m_spikes.friction = 1.f;
    m_spikes.density = .8f*M_DENSITY;
    m_spikes.restitution = .1f;
    m_spikes.type = TYPE_METAL;

    if (!(m_rackhouse.pipeline[0].program = shader_shiny->get_program(0)))
        m_rackhouse.pipeline[0].program = shader_pv_textured->get_program(0);
    m_rackhouse.pipeline[1].program = shader_gi_tex->get_program(1);
    m_rackhouse.pipeline[2].program = shader_pv_textured_m->get_program(2);
    m_rackhouse.pipeline[0].texture[0] = static_cast<tms_texture*>(tex_rackhouse);
    m_rackhouse.pipeline[0].texture[1] = static_cast<tms_texture*>(tex_reflection);
    if (enable_gi) m_rackhouse.pipeline[1].texture[0] = static_cast<tms_texture*>(tex_rackhouse);
    m_rackhouse.pipeline[2].texture[0] = static_cast<tms_texture*>(tex_rackhouse);
    if (shadow_ao_combine) {
        m_rackhouse.pipeline[3].program = shader_ao->get_program(3);
    } else {
        m_rackhouse.pipeline[3].program = shader_ao_norot->get_program(3);
    }
    m_rackhouse.type = TYPE_METAL2;
    m_rackhouse.friction = .6f;
    m_rackhouse.density = .5f*M_DENSITY;
    m_rackhouse.restitution = .2f;

    m_rack.pipeline[0].program = shader_pv_textured_ao->get_program(0);
    m_rack.pipeline[1].program = shader_gi->get_program(1);
    m_rack.pipeline[2].program = shader_pv_textured_m->get_program(2);
    m_rack.pipeline[0].texture[0] = static_cast<tms_texture*>(tex_rack);
    m_rack.pipeline[2].texture[0] = static_cast<tms_texture*>(tex_rack);

    m_wheel.pipeline[0].program = shader_wheel->get_program(0);
    m_wheel.pipeline[1].program = shader_gi->get_program(1);
    m_wheel.pipeline[2].program = shader_pv_textured_m->get_program(2);
    m_wheel.pipeline[0].texture[0] = static_cast<tms_texture*>(tex_wheel);
    m_wheel.pipeline[0].texture[1] = static_cast<tms_texture*>(tex_reflection);
    m_wheel.pipeline[2].texture[0] = static_cast<tms_texture*>(tex_wheel);
    if (shadow_ao_combine) {
        m_wheel.pipeline[3].program = shader_ao->get_program(3);
    } else {
        m_wheel.pipeline[3].program = shader_ao_norot->get_program(3);
    }
    m_wheel.friction = 1.5f;
    m_wheel.density = .5f*M_DENSITY;
    m_wheel.restitution = .4f;
    m_wheel.type = TYPE_RUBBER;

    m_wmotor.pipeline[0].program = shader_pv_textured->get_program(0);
    m_wmotor.pipeline[1].program = shader_gi->get_program(1);
    m_wmotor.pipeline[2].program = shader_pv_textured_m->get_program(2);
    m_wmotor.pipeline[0].texture[0] = static_cast<tms_texture*>(tex_wmotor);
    m_wmotor.pipeline[2].texture[0] = static_cast<tms_texture*>(tex_wmotor);
    if (shadow_ao_combine) {
        m_wmotor.pipeline[3].program = shader_ao->get_program(3);
    } else {
        m_wmotor.pipeline[3].program = shader_ao_norot->get_program(3);
    }
    m_wmotor.type = TYPE_METAL;

    m_sticky.pipeline[0].program = shader_pv_sticky->get_program(0);
    m_sticky.pipeline[1].program = shader_gi->get_program(1);
    m_sticky.pipeline[2].program = shader_pv_sticky->get_program(2);
    m_sticky.pipeline[0].texture[0] = static_cast<tms_texture*>(&sticky::texture);
    m_sticky.pipeline[2].texture[0] = static_cast<tms_texture*>(&sticky::texture);
    if (shadow_ao_combine) {
        m_sticky.pipeline[3].program = shader_ao->get_program(3);
    } else {
        m_sticky.pipeline[3].program = shader_ao_norot->get_program(3);
    }

    m_cup.pipeline[0].program = shader_pv_textured_ao->get_program(0);
    m_cup.pipeline[1].program = shader_gi->get_program(1);
    m_cup.pipeline[2].program = shader_pv_textured_m->get_program(2);
    m_cup.pipeline[0].texture[0] = static_cast<tms_texture*>(tex_cup_ao);
    m_cup.pipeline[2].texture[0] = static_cast<tms_texture*>(tex_cup_ao);
    if (shadow_ao_combine) {
        m_cup.pipeline[3].program = shader_ao->get_program(3);
    } else {
        m_cup.pipeline[3].program = shader_ao_norot->get_program(3);
    }
    m_cup.type = TYPE_PLASTIC;

    m_ledbuf.pipeline[0].program = shader_ledbuf->get_program(0);
    m_ledbuf.pipeline[1].program = 0;
    m_ledbuf.pipeline[2].program = 0;

    m_digbuf.pipeline[0].program = shader_digbuf->get_program(0);
    m_digbuf.pipeline[1].program = 0;
    m_digbuf.pipeline[2].program = 0;

    m_field.pipeline[0].program = shader_field->get_program(0);
    //m_field.pipeline[0].flags |= TMS_MATERIAL_BLEND;
    m_field.pipeline[0].blend_mode = TMS_BLENDMODE__SRC_ALPHA__ONE_MINUS_SRC_ALPHA;
    m_field.pipeline[1].program = 0;
    m_field.pipeline[2].program = 0;
    m_field.type = 0;
    m_field.density = 0.01f; /* used by plasma bullet */
    m_field.friction = FLT_EPSILON;
    m_field.restitution = 0.0f;

    m_linebuf.pipeline[0].program = shader_linebuf->get_program(0);
    m_linebuf.pipeline[0].blend_mode = TMS_BLENDMODE__SRC_ALPHA__ONE_MINUS_SRC_ALPHA;
    m_linebuf.pipeline[0].texture[0] = static_cast<tms_texture*>(tex_line);
    m_linebuf.pipeline[1].program = 0;
    m_linebuf.pipeline[2].program = 0;

    m_linebuf2.pipeline[0].program = shader_linebuf->get_program(0);
    m_linebuf2.pipeline[0].blend_mode = TMS_BLENDMODE__SRC_ALPHA__ONE;
    m_linebuf2.pipeline[0].texture[0] = static_cast<tms_texture*>(tex_line);
    m_linebuf2.pipeline[1].program = 0;
    m_linebuf2.pipeline[2].program = 0;

    m_fluidbuf.pipeline[0].program = shader_fluidbuf->get_program(0);
    m_fluidbuf.pipeline[0].blend_mode = TMS_BLENDMODE__SRC_ALPHA__ONE_MINUS_SRC_ALPHA;
    m_fluidbuf.pipeline[0].texture[0] = static_cast<tms_texture*>(tex_sprites);
    m_fluidbuf.pipeline[1].program = 0;
    m_fluidbuf.pipeline[2].program = 0;

    m_spritebuf.pipeline[0].program = shader_spritebuf->get_program(0);
    m_spritebuf.pipeline[0].blend_mode = TMS_BLENDMODE__SRC_ALPHA__ONE_MINUS_SRC_ALPHA;
    m_spritebuf.pipeline[0].texture[0] = static_cast<tms_texture*>(tex_sprites);
    m_spritebuf.pipeline[1].program = 0;
    m_spritebuf.pipeline[2].program = 0;

    m_spritebuf2.pipeline[0].program = shader_spritebuf_light->get_program(0);
    m_spritebuf2.pipeline[0].blend_mode = TMS_BLENDMODE__SRC_ALPHA__ONE;
    m_spritebuf2.pipeline[0].texture[0] = static_cast<tms_texture*>(tex_sprites);
    m_spritebuf2.pipeline[1].program = 0;
    m_spritebuf2.pipeline[2].program = 0;

    m_charbuf.pipeline[0].program = shader_charbuf->get_program(0);
    //m_charbuf.pipeline[0].blend_mode = TMS_BLENDMODE__SRC_ALPHA__ONE_MINUS_SRC_ALPHA;
    m_charbuf.pipeline[0].blend_mode = TMS_BLENDMODE_OFF;
    m_charbuf.pipeline[0].texture[0] = &gui_spritesheet::atlas_text->texture;
    m_charbuf.pipeline[1].program = 0;
    m_charbuf.pipeline[2].program = 0;

    m_charbuf2.pipeline[0].program = shader_charbuf2->get_program(0);
    m_charbuf2.pipeline[0].blend_mode = TMS_BLENDMODE__SRC_ALPHA__ONE_MINUS_SRC_ALPHA;
    m_charbuf2.pipeline[0].texture[0] = &gui_spritesheet::atlas_text->texture;
    m_charbuf2.pipeline[1].program = 0;
    m_charbuf2.pipeline[2].program = 0;

    m_conveyor.pipeline[0].program = shader_pv_colored->get_program(0);
    m_conveyor.pipeline[1].program = shader_gi_col->get_program(1);
    m_conveyor.pipeline[2].program = shader_pv_colored->get_program(2);
    if (shadow_ao_combine) {
        m_conveyor.pipeline[3].program = shader_ao->get_program(3);
    } else {
        m_conveyor.pipeline[3].program = shader_ao_norot->get_program(3);
    }
    m_conveyor.friction = .6f;
    m_conveyor.density = .5f*M_DENSITY;
    m_conveyor.restitution = .1f;
    m_conveyor.type = TYPE_RUBBER;

    m_item.pipeline[0].program = shader_pv_textured->get_program(0);
    m_item.pipeline[1].program = shader_gi->get_program(1);
    m_item.pipeline[2].program = shader_pv_textured_m->get_program(2);
    m_item.pipeline[0].texture[0] = static_cast<tms_texture*>(tex_items);
    m_item.pipeline[2].texture[0] = static_cast<tms_texture*>(tex_items);
    if (shadow_ao_combine) {
        m_item.pipeline[3].program = 0;//shader_ao->get_program(3);
    } else {
        m_item.pipeline[3].program = shader_ao_norot->get_program(3);
    }
    m_item.friction = .6f;
    m_item.density = .5f*M_DENSITY;
    m_item.restitution = .3f;
    m_item.type = TYPE_PLASTIC;

    m_item_shiny.pipeline[0].program = shader_textured->get_program(0);
    m_item_shiny.pipeline[1].program = shader_gi->get_program(1);
    m_item_shiny.pipeline[2].program = shader_pv_textured_m->get_program(2);
    m_item_shiny.pipeline[0].texture[0] = static_cast<tms_texture*>(tex_items);
    m_item_shiny.pipeline[2].texture[0] = static_cast<tms_texture*>(tex_items);
    if (shadow_ao_combine) {
        m_item_shiny.pipeline[3].program = shader_ao->get_program(3);
    } else {
        m_item_shiny.pipeline[3].program = shader_ao_norot->get_program(3);
    }
    m_item_shiny.friction = .6f;
    m_item_shiny.density = .5f*M_DENSITY;
    m_item_shiny.restitution = .3f;
    m_item_shiny.type = TYPE_PLASTIC;

    m_chest.pipeline[0].program = shader_pv_textured_ao->get_program(0);
    m_chest.pipeline[1].program = shader_gi->get_program(1);
    m_chest.pipeline[2].program = shader_pv_textured_m->get_program(2);
    m_chest.pipeline[0].texture[0] = static_cast<tms_texture*>(tex_chests);
    m_chest.pipeline[2].texture[0] = static_cast<tms_texture*>(tex_chests);
    if (shadow_ao_combine) {
        m_chest.pipeline[3].program = shader_ao->get_program(3);
    } else {
        m_chest.pipeline[3].program = shader_ao_norot->get_program(3);
    }
    m_chest.friction = .6f;
    m_chest.density = .5f*M_DENSITY;
    m_chest.restitution = .3f;
    m_chest.type = TYPE_PLASTIC;

    m_chest_shiny.pipeline[0].program = shader_textured_ao->get_program(0);
    m_chest_shiny.pipeline[1].program = shader_gi->get_program(1);
    m_chest_shiny.pipeline[2].program = shader_pv_textured_m->get_program(2);
    m_chest_shiny.pipeline[0].texture[0] = static_cast<tms_texture*>(tex_chests);
    m_chest_shiny.pipeline[2].texture[0] = static_cast<tms_texture*>(tex_chests);
    if (shadow_ao_combine) {
        m_chest_shiny.pipeline[3].program = shader_ao->get_program(3);
    } else {
        m_chest_shiny.pipeline[3].program = shader_ao_norot->get_program(3);
    }
    m_chest_shiny.friction = .6f;
    m_chest_shiny.density = .5f*M_DENSITY;
    m_chest_shiny.restitution = .3f;
    m_chest_shiny.type = TYPE_PLASTIC;

    m_repairstation.pipeline[0].program = shader_pv_textured_ao->get_program(0);
    m_repairstation.pipeline[1].program = shader_gi->get_program(1);
    m_repairstation.pipeline[2].program = shader_pv_textured_m->get_program(2);
    m_repairstation.pipeline[0].texture[0] = static_cast<tms_texture*>(tex_repairstation);
    m_repairstation.pipeline[2].texture[0] = static_cast<tms_texture*>(tex_repairstation);
    if (shadow_ao_combine) {
        m_repairstation.pipeline[3].program = shader_ao->get_program(3);
    } else {
        m_repairstation.pipeline[3].program = shader_ao_norot->get_program(3);
    }
    m_repairstation.friction = .6f;
    m_repairstation.density = .5f*M_DENSITY;
    m_repairstation.restitution = .0125f;
    m_repairstation.type = TYPE_PLASTIC;

    m_robot2.pipeline[0].program = shader_pv_textured_ao->get_program(0);
    //m_robot2.pipeline[0].program = shader_colored->get_program(0);
    m_robot2.pipeline[1].program = shader_gi->get_program(1);
    m_robot2.pipeline[2].program = shader_pv_textured_m->get_program(2);
    m_robot2.pipeline[0].texture[0] = static_cast<tms_texture*>(tex_robot2);
    m_robot2.pipeline[2].texture[0] = static_cast<tms_texture*>(tex_robot2);
    m_robot2.pipeline[3].program = shader_ao_norot->get_program(3);
    m_robot2.friction = .7f;
    m_robot2.density = .5f*M_DENSITY*ROBOT_DENSITY_MUL;
    m_robot2.restitution = .1f;
    m_robot2.type = TYPE_SHEET_METAL;

    m_stone.pipeline[0].program = shader_pv_textured->get_program(0);
    m_stone.pipeline[1].program = shader_gi_tex->get_program(1);
    m_stone.pipeline[2].program = shader_pv_textured_m->get_program(2);
    m_stone.pipeline[0].texture[0] = static_cast<tms_texture*>(tex_tpixel);
    if (enable_gi) m_stone.pipeline[1].texture[0] = static_cast<tms_texture*>(tex_tpixel);
    m_stone.pipeline[2].texture[0] = static_cast<tms_texture*>(tex_tpixel);
    if (shadow_ao_combine) {
        m_stone.pipeline[3].program = shader_ao->get_program(3);
    } else {
        m_stone.pipeline[3].program = shader_ao_norot->get_program(3);
    }
    m_stone.friction = 1.f;
    m_stone.density = 3.0f*M_DENSITY;
    m_stone.restitution = .2f;
    m_stone.type = TYPE_STONE;

    m_decoration.pipeline[0].program = shader_pv_textured->get_program(0);
    m_decoration.pipeline[1].program = shader_gi_tex->get_program(1);
    m_decoration.pipeline[2].program = shader_pv_textured_m->get_program(2);
    m_decoration.pipeline[0].texture[0] = static_cast<tms_texture*>(tex_decoration);
    if (enable_gi) m_decoration.pipeline[1].texture[0] = static_cast<tms_texture*>(tex_decoration);
    m_decoration.pipeline[2].texture[0] = static_cast<tms_texture*>(tex_decoration);
    if (shadow_ao_combine) {
        m_decoration.pipeline[3].program = shader_ao->get_program(3);
    } else {
        m_decoration.pipeline[3].program = shader_ao_norot->get_program(3);
    }
    m_decoration.friction = .6f;
    m_decoration.density = .5f*M_DENSITY;
    m_decoration.restitution = .3f;
    m_decoration.type = TYPE_PLASTIC;

    m_robot_tinted_light.pipeline[0].program = shader_pv_textured_ao_tinted->get_program(0);
    m_robot_tinted_light.pipeline[1].program = shader_gi->get_program(1);
    m_robot_tinted_light.pipeline[2].program = shader_pv_textured_m->get_program(2);
    m_robot_tinted_light.pipeline[0].texture[0] = static_cast<tms_texture*>(tex_robot);
    m_robot_tinted_light.pipeline[2].texture[0] = static_cast<tms_texture*>(tex_robot);
    m_robot_tinted_light.pipeline[3].program = shader_ao_norot->get_program(3);
    m_robot_tinted_light.friction = m_robot.friction;
    m_robot_tinted_light.density = m_robot.density*.5;
    m_robot_tinted_light.restitution = m_robot.restitution;
    m_robot_tinted_light.type = TYPE_SHEET_METAL;

/*
    m_robot_armor.pipeline[0].program = shader_pv_textured_ao->get_program(0);
    //m_robot_armor.pipeline[0].program = shader_colored->get_program(0);
    m_robot_armor.pipeline[1].program = shader_gi->get_program(1);
    m_robot_armor.pipeline[2].program = shader_pv_textured_m->get_program(2);
    m_robot_armor.pipeline[0].texture[0] = static_cast<tms_texture*>(tex_robot_armor);
    m_robot_armor.pipeline[2].texture[0] = static_cast<tms_texture*>(tex_robot_armor);
    if (shadow_ao_combine) {
        m_robot_armor.pipeline[3].program = 0;
    } else {
        m_robot_armor.pipeline[3].program = shader_ao_norot->get_program(3);
    }
    m_robot_armor.friction = .7f;
    m_robot_armor.density = .5f*M_DENSITY*ROBOT_DENSITY_MUL;
    m_robot_armor.restitution = .1f;
    m_robot_armor.type = TYPE_SHEET_METAL;*/

    m_robot_armor.pipeline[0].program = shader_pv_textured_ao_tinted->get_program(0);
    m_robot_armor.pipeline[1].program = shader_gi->get_program(1);
    m_robot_armor.pipeline[2].program = shader_pv_textured_m->get_program(2);
    m_robot_armor.pipeline[0].texture[0] = static_cast<tms_texture*>(tex_robot_armor);
    m_robot_armor.pipeline[2].texture[0] = static_cast<tms_texture*>(tex_robot_armor);
    m_robot_armor.pipeline[3].program = m_robot.pipeline[3].program;
    m_robot_armor.friction = m_robot.friction;
    m_robot_armor.density = m_robot.density;
    m_robot_armor.restitution = m_robot.restitution;
    m_robot_armor.type = TYPE_SHEET_METAL;

}
