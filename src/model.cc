#include "model.hh"
#include "misc.hh"
#include "pkgman.hh"
#include "settings.hh"

#define NUM_MISC_MODELS 3
struct tms_model *model_misc[NUM_MISC_MODELS];
int cur_model = 0;

#define cache_path_VERSION 34

static int i1o1_shift_i = 1;
static int i2o1_shift_i = 1;
static int cpad_shift_i = 1;

#define PATH(x) "data/models/" x ".3ds"
struct model_load_data mesh_factory::models[NUM_MODELS] = {
    {PATH("plank1")},
    {PATH("plank2")},
    {PATH("plank3")},
    {PATH("plank4")},
    {PATH("thinplank1")},
    {PATH("thinplank2")},
    {PATH("thinplank3")},
    {PATH("thinplank4")},
    {PATH("splank")},
    {PATH("splank1")},
    {PATH("splank2")},
    {PATH("splank3")},
    {PATH("splank_front")},
    {PATH("splank_back")},
    {PATH("impact1")},
    {PATH("impact2")},
    {PATH("impact3")},
    {PATH("impact4")},
    {PATH("field1")},
    {PATH("field2")},
    {PATH("field3")},
    {PATH("field4")},
    {PATH("room")},
    {PATH("room_corner_1")},
    {PATH("room_corner_2")},
    {PATH("room_corner_3")},
    {PATH("room_corner_4")},
    {PATH("arm_shotgun")},
    {PATH("arm_bomber")},
    {PATH("arm_bomber_chamber")},
    {PATH("arm_railgun")},
    {PATH("diamond")},
    {PATH("ruby")},
    {PATH("stone")},
    {PATH("weight")},
    {PATH("separator")},
    {PATH("robot")},
    {PATH("robot_gunarm")},
    {PATH("robot_dragarm")},
    {PATH("robot_head")},
    {PATH("robot_body")},
    {PATH("robot_back")},
    {PATH("robot_front")},
    {PATH("feet_frame")},
    {PATH("bot")},
    {PATH("spikebot-2")},
    {PATH("minibot")},
    {PATH("bomber")},
    {PATH("sphere")},
    {PATH("sphere2")},
    {PATH("sphere3")},
    {PATH("generator")},
    {PATH("battery3v")},
    {PATH("wmotor")},
    {PATH("flatmotor")},
    {PATH("simplemotor")},
    {PATH("gear0")},
    {PATH("gear1")},
    {PATH("gear2")},
    {PATH("gear3")},
    {PATH("trampolinebase")},
    {PATH("trampolinepad")},
    {PATH("plug.simple")},
    {PATH("plug.simple.low")},
    {PATH("plug.male")},
    {PATH("plug.female")},
    {PATH("plug.transmitter")},
    {PATH("c_ifplug.male")},
    {PATH("c_ifplug.female")},
    {PATH("script")},
    {PATH("sticky")},
    {PATH("breadboard")},
    {PATH("motor")},
    {PATH("wheel")},
    {PATH("cup")},
    {PATH("cylinder05")},
    {PATH("cylinder1")},
    {PATH("cylinder1.5")},
    {PATH("cylinder2")},
    {PATH("wallthing00")},
    {PATH("wallthing0")},
    {PATH("wallthing1")},
    {PATH("wallthing2")},
    {PATH("joint")},
    {PATH("plate")},
    {PATH("platejoint_damaged")},
    {PATH("pivotjoint")},
    {PATH("corner")},
    {PATH("panel.big")},
    {PATH("panel.medium")},
    {PATH("panel.small")},
    {PATH("switch")},
    {PATH("robotman")},
    {PATH("magnet")},
    {PATH("railstraight")},
    {PATH("railturn")},
    {PATH("railskewed")},
    {PATH("railskewed2")},
    {PATH("gyroscope")},
    {PATH("tiltmeter")},
    {PATH("lmotor0")},
    {PATH("lmotor1")},
    {PATH("lmotor2")},
    {PATH("lmotor3")},
    {PATH("lmotor0_r")},
    {PATH("lmotor1_r")},
    {PATH("lmotor2_r")},
    {PATH("lmotor3_r")},
    {PATH("magplug")},
    {PATH("magsocket")},
    {PATH("fifo")},
    {PATH("seesaw")},
    {PATH("pivot")},
    {PATH("anchor")},
    {PATH("limb")},
    {PATH("bullet")},
    {PATH("missile")},
    {PATH("ropeend")},
    {PATH("pipeline.base")},
    {PATH("pipeline.piston")},
    {PATH("pipeline.house")},
    {PATH("clip")},
    {PATH("cclip")},
    {PATH("landmine")},
    {PATH("gearbox")},
    {PATH("box_notex")},
    {PATH("box_tex")},
    {PATH("tribox_tex0")},
    {PATH("tribox_tex1")},
    {PATH("tribox_tex2")},
    {PATH("tribox_tex3")},
    {PATH("gb.axle")},
    {PATH("gb.4")},
    {PATH("gb.6")},
    {PATH("gb.8")},
    {PATH("gb.10")},
    {PATH("gb.12")},
    {PATH("controller.mini")},
    {PATH("controller.pass")},
    {PATH("controller.servo")},
    {PATH("controller.fplus")},
    {PATH("damper_0")},
    {PATH("damper_1")},
    {PATH("display")},
    {PATH("display_active")},
    {PATH("crane")},
    {PATH("border-new")},
    {PATH("scanner")},
    {PATH("mirror")},
    {PATH("lasersensor")},
    {PATH("rocket")},
    {PATH("thruster")},
    {PATH("bomb")},
    {PATH("plasma_gun_inner")},
    {PATH("plasma_gun")},
    {PATH("tesla_gun")},
    {PATH("mega_buster")},
    {PATH("arm_rocket_launcher")},
    {PATH("btn")},
    {PATH("btn_switch")},
    {PATH("proximity")},
    {PATH("gameman")},
    {PATH("debris")},
    {PATH("box1")},
    {PATH("box2")},
    {PATH("debugger_0")},
    {PATH("debugger_1")},
    {PATH("graph")},
    {PATH("estabilizer")},
    {PATH("factory_generic")},
    {PATH("factory_robot")},
    {PATH("factory_armory")},
    {PATH("factory_oil_mixer")},
    {PATH("repair_station")},
    {PATH("ladder")},
    {PATH("jetpack")},
    {PATH("advanced_jetpack")},
    {PATH("pig")},
    {PATH("cow")},
    {PATH("cow_head")},
    {PATH("pig_head")},
    {PATH("leaves1")},
    {PATH("emitter")},
    {PATH("emitter_frame")},
    {PATH("miniemitter")},
    {PATH("rubberend")},
    {PATH("i0o1")},
    {PATH("i0o2")},
    {PATH("i0o3")},
    {PATH("i1o0")},
    {PATH("i1o2")},
    {PATH("i1o3")},
    {PATH("i1o4")},
    {PATH("i1o8")},
    {PATH("i2o0")},
    {PATH("i2o2")},
    {PATH("i3o1")},
    {PATH("i4o0")},
    {PATH("i4o1")},
    {PATH("pointer_body")},
    {PATH("pointer_arrow")},
    {PATH("dragfield")},
    {PATH("gravity")},
    {PATH("gravityset")},
    {PATH("spikes")},
    {PATH("backpack")},
    {PATH("adamper")},
    {PATH("checkpoint")},
    {PATH("fan")},
    {PATH("fan_blades")},
    {PATH("transmitter")},
    {PATH("barrel")},
    {PATH("heisenberg")},
    {PATH("wizardhat")},
    {PATH("ninjahelmet")},
    {PATH("robe")},
    {PATH("suctioncup")},
    {PATH("canister")},
    {PATH("oilrig")},
    {PATH("conveyor0")},
    {PATH("conveyor1")},
    {PATH("conveyor2")},
    {PATH("conveyor3")},
    {PATH("conveyor4")},
    {PATH("conveyor5")},
    {PATH("robot_head_inside")},
    {PATH("conicalhat")},
    {PATH("ostrich")},
    {PATH("ostrich_head")},
    {PATH("policehat")},
    {PATH("circuit")},
    {PATH("oilbarrel")},
    {PATH("canister_armour")},
    {PATH("canister_speed")},
    {PATH("canister_jump")},
    {PATH("ore")},
    {PATH("ore_inside")},
    {PATH("ore2")},
    {PATH("ore2_inside")},
    {PATH("vendor")},
    {PATH("treasurechest")},
    {PATH("builder")},
    {PATH("miner")},
    {PATH("factionwand")},
    {PATH("boltset_steel")},
    {PATH("boltset_wood")},
    {PATH("boltset_titanium")},
    {PATH("boltset_diamond")},
    {PATH("lobber")},
    {PATH("spikeball")},
    {PATH("guardpoint")},
    {PATH("stone1")},
    {PATH("stone2")},
    {PATH("stone3")},
    {PATH("stone4")},
    {PATH("stone5")},
    {PATH("stone6")},
    {PATH("mushroom1")},
    {PATH("mushroom2")},
    {PATH("mushroom3")},
    {PATH("mushroom4")},
    {PATH("mushroom5")},
    {PATH("mushroom6")},
    {PATH("leaves2")},
    {PATH("leaves3")},
    {PATH("sign1")},
    {PATH("sign2")},
    {PATH("sign3")},
    {PATH("sign4")},
    {PATH("wood")},
    {PATH("fence")},
    {PATH("tophat")},
    {PATH("compressor")},
    {PATH("compressor_lamp1")},
    {PATH("compressor_lamp2")},
    {PATH("compressor_lamp3")},
    {PATH("compressor_lamp4")},
    {PATH("ladder_step")},
    {PATH("kingscrown")},
    {PATH("dummy_head")},
    {PATH("statue_head")},
    {PATH("jesterhat")},
    {PATH("woodsword")},
    {PATH("hammer")},
    {PATH("witch_hat")},
    {PATH("simple_axe")},
    {PATH("saw1")},
    {PATH("saw")},
    {PATH("saw_blade")},
    {PATH("spiked_club")},
    {PATH("steel_sword")},
    {PATH("baseballbat")},
    {PATH("spear")},
    {PATH("plant1")},
    {PATH("plant2")},
    {PATH("plant3")},
    {PATH("plant4")},
    {PATH("war_axe")},
    {PATH("pixel_sword")},
    {PATH("hard_hat")},
    {PATH("serpent_sword")},
    {PATH("pioneer_front")},
    {PATH("pioneer_back")},
    {PATH("vikinghelmet")},
    {PATH("pickaxe")},
    {PATH("i1o1")}, // MODEL_I1O1_EMPTY
    {0, MODEL_I1O1_EMPTY, tvec2f(0.f, -.125f*i1o1_shift_i++)}, // MODEL_I1O1_INTEGER
    {0, MODEL_I1O1_EMPTY, tvec2f(0.f, -.125f*i1o1_shift_i++)}, // MODEL_I1O1_SQUARE
    {0, MODEL_I1O1_EMPTY, tvec2f(0.f, -.125f*i1o1_shift_i++)}, // MODEL_I1O1_SQRT
    {0, MODEL_I1O1_EMPTY, tvec2f(0.f, -.125f*i1o1_shift_i++)}, // MODEL_I1O1_SPARSIFY
    {0, MODEL_I1O1_EMPTY, tvec2f(0.f, -.125f*i1o1_shift_i++)}, // MODEL_I1O1_BESSERWISSER
    {0, MODEL_I1O1_EMPTY, tvec2f(0.f, -.125f*i1o1_shift_i++)}, // MODEL_I1O1_EPSILON
    {0, MODEL_I1O1_EMPTY, tvec2f(0.f, -.125f*i1o1_shift_i++)}, // MODEL_I1O1_INVERT
    {PATH("i2o1_empty")}, // MODEL_I2O1_EMPTY
    {PATH("i2o1_1")}, // MODEL_I2O1_AND
    {PATH("i2o1_2")}, // MODEL_I2O1_OR
    {PATH("i2o1_3")}, // MODEL_I2O1_XOR
    {PATH("i2o1_4")}, // MODEL_I2O1_NAND
    {PATH("i2o1_5")}, // MODEL_I2O1_EQUAL
    {PATH("i2o1_6")}, // MODEL_I2O1_LESS
    {PATH("i2o1_7")}, // MODEL_I2O1_LESS_EQUAL
    {PATH("i2o1_8")}, // MODEL_I2O1_SUM
    {PATH("i2o1_9")}, // MODEL_I2O1_WRAP_ADD
    {PATH("i2o1_10")}, // MODEL_I2O1_WRAP_SUB
    {PATH("i2o1_11")}, // MODEL_I2O1_UNUSED
    {PATH("cpad")},
    // Different command pad variants, shift the texture used for each while reusing the model
    {0, MODEL_CPAD, tvec2f(0.f, (-1.f/16.f)*cpad_shift_i++)},
    {0, MODEL_CPAD, tvec2f(0.f, (-1.f/16.f)*cpad_shift_i++)},
    {0, MODEL_CPAD, tvec2f(0.f, (-1.f/16.f)*cpad_shift_i++)},
    {0, MODEL_CPAD, tvec2f(0.f, (-1.f/16.f)*cpad_shift_i++)},
    {0, MODEL_CPAD, tvec2f(0.f, (-1.f/16.f)*cpad_shift_i++)},
    {0, MODEL_CPAD, tvec2f(0.f, (-1.f/16.f)*cpad_shift_i++)},
    {0, MODEL_CPAD, tvec2f(0.f, (-1.f/16.f)*cpad_shift_i++)},
    {0, MODEL_CPAD, tvec2f(0.f, (-1.f/16.f)*cpad_shift_i++)},
    {0, MODEL_CPAD, tvec2f(0.f, (-1.f/16.f)*cpad_shift_i++)},
    {0, MODEL_CPAD, tvec2f(0.f, (-1.f/16.f)*cpad_shift_i++)},
    {0, MODEL_CPAD, tvec2f(0.f, (-1.f/16.f)*cpad_shift_i++)},
    {0, MODEL_CPAD, tvec2f(0.f, (-1.f/16.f)*cpad_shift_i++)},
    {0, MODEL_CPAD, tvec2f(0.f, (-1.f/16.f)*cpad_shift_i++)},
    {0, MODEL_CPAD, tvec2f(0.f, (-1.f/16.f)*cpad_shift_i++)},
    {0, MODEL_CPAD, tvec2f(0.f, (-1.f/16.f)*cpad_shift_i++)},
    {0, MODEL_CPAD, tvec2f(0.f, (-1.f/16.f)*cpad_shift_i++)}
};
#undef PATH

int mesh_factory::cur_mesh = 0;

char mesh_factory::cache_path[512];
bool mesh_factory::use_cache = false;

bool mesh_factory::open_cache(lvlbuf *lb) {
    FILE *fp = fopen(cache_path, "rb");
    if (!fp) {
        tms_errorf("Unable to open cache_path %s", cache_path);
        return false;
    }

    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    lb->reset();
    lb->size = 0;
    lb->ensure((int)size);

    fread(lb->buf, 1, size, fp);

    fclose(fp);

    lb->size = size;

    return true;
}

bool mesh_factory::read_cache(lvlbuf *lb) {
    uint8_t version = lb->r_uint8();
    uint32_t num_meshes = lb->r_uint32();
    uint32_t num_models = lb->r_uint32();

    if (version != cache_path_VERSION) {
        tms_errorf("Mismatching version code in model cache.");
        return false;
    }

    if (num_meshes != NUM_MODELS) {
        tms_errorf("Mismatching mesh count in model cache.");
        return false;
    }

    if (num_models != NUM_MISC_MODELS) {
        tms_errorf("Mismatching model count in model cache.");
        return false;
    }

    char *data;
    for (int x = 0; x < num_models; ++x) {
        struct tms_model *m = (struct tms_model*)calloc(1, sizeof(struct tms_model));

        uint32_t vertices_size = lb->r_uint32();
        data = (char*)malloc(vertices_size);
        lb->r_buf(data, vertices_size);
        m->vertices = tms_gbuffer_alloc_fill(data, vertices_size);
        free(data);

        uint32_t indices_size = lb->r_uint32();
        data = (char*)malloc(indices_size);
        lb->r_buf(data, indices_size);
        m->indices = tms_gbuffer_alloc_fill(data, indices_size);
        m->indices->target = GL_ELEMENT_ARRAY_BUFFER;
        free(data);

        m->va = tms_varray_alloc(3);
        tms_varray_map_attribute(m->va, "position", 3, GL_FLOAT, m->vertices);
        tms_varray_map_attribute(m->va, "normal", 3, GL_FLOAT, m->vertices);
        tms_varray_map_attribute(m->va, "texcoord", 2, GL_FLOAT, m->vertices);

        uint32_t num_meshes = lb->r_uint32();

        for (int n = 0; n < num_meshes; ++n) {
            struct tms_mesh *mesh = tms_model_create_mesh(m);

            mesh->id = lb->r_int32();

            mesh->i_start = lb->r_int32();
            mesh->i_count = lb->r_int32();

            mesh->v_start = lb->r_int32();
            mesh->v_count = lb->r_int32();

            mesh_factory::models[mesh->id].mesh = mesh;
        }

        m->flat = 0;

        model_misc[x] = m;
    }

    return true;
}

bool mesh_factory::write_cache(lvlbuf *lb) {
    lb->w_s_uint8(cache_path_VERSION);
    lb->w_s_uint32(NUM_MODELS);
    lb->w_s_uint32(NUM_MISC_MODELS);

    for (int x = 0; x < NUM_MISC_MODELS; ++x) {
        struct tms_model *m = model_misc[x];

        lb->w_s_uint32(m->vertices->size);
        lb->w_s_buf(m->vertices->buf, m->vertices->size);

        lb->w_s_uint32(m->indices->size);
        lb->w_s_buf(m->indices->buf, m->indices->size);

        lb->w_s_uint32(m->num_meshes);

        struct tms_mesh *mesh;
        for (int n=0; n<m->num_meshes; ++n) {
            mesh = m->meshes[n];

            if (!mesh) return false;

            lb->w_s_int32(mesh->id);

            lb->w_s_int32(mesh->i_start);
            lb->w_s_int32(mesh->i_count);
            lb->w_s_int32(mesh->v_start);
            lb->w_s_int32(mesh->v_count);
        }
    }

    return true;
}

bool mesh_factory::save_cache(lvlbuf *lb) {
    FILE *fp = fopen(cache_path, "wb");
    if (!fp)
        return false;

    fwrite(lb->buf, 1, lb->size, fp);
    fclose(fp);
    return true;
}

void mesh_factory::init_models() {
    snprintf(cache_path, 511, "%s/models.cache", tms_storage_cache_path());

    GLuint err = glGetError();
    if (err != 0) {
        tms_errorf("GL Error after initializing models: %d", err);
    }

    use_cache = false;

    tms_infof("Model cache path: %s", cache_path);

    if (!settings["always_reload_data"]->v.b && file_exists(cache_path)) {
        tms_infof("Checking if we want to use cache...");
        /* The cache file exists, make sure we want to use it. */
        use_cache = true;
#ifndef SDL_PLATFORM_ANDROID
        time_t cache_mtime = get_mtime(cache_path);
        time_t model_mtime;

        for (int x=0; x<NUM_MODELS; ++x) {
            if (!mesh_factory::models[x].path) continue;

            model_mtime = get_mtime(mesh_factory::models[x].path);
            if (model_mtime >= cache_mtime) {
                tms_infof("Not using cache, %s has been modified", mesh_factory::models[x].path);
                use_cache = false;
                break;
            }
        }
#endif
    }

    if (use_cache) {
        tms_infof("Initializing models... (cache)");

        lvlbuf lb;

        if (!open_cache(&lb)) {
            tms_errorf("Error opening cache, reverting to non-cached model loading.");
            use_cache = false;
        } else {
            if (read_cache(&lb)) {
                cur_mesh = NUM_MODELS;
            } else {
                tms_errorf("Error reading cache, reverting to non-cached model loading.");
                use_cache = false;
            }
        }

        if (lb.buf) {
            free(lb.buf);
        }
    }

    if (!use_cache) {
        tms_infof("Initializing models... (no cache)");
        for (int x=0; x<NUM_MISC_MODELS; ++x) {
            model_misc[x] = tms_model_alloc();
        }
    }
}

void mesh_factory::upload_models() {
    tms_infof("Uploading models...");

    for (int x=0; x<NUM_MISC_MODELS; ++x) {
        tms_model_upload(model_misc[x]);
    }

    GLuint err = glGetError();
    if (err != 0) {
        tms_errorf("GL Error after uploading models: %d", err);
    }

    if (!use_cache) {
        /* write cache file! */
        lvlbuf lb;

        /* dump models to cache file */
        if (!write_cache(&lb)) {
            tms_errorf("An error occurred while trying to write model cache.");
        } else if (!save_cache(&lb)) {
            tms_errorf("An error occurred while trying to save model cache to a file. (not enough permission/disk space?)");
        }
    }
}

bool mesh_factory::load_next() {
    if (cur_mesh >= NUM_MODELS) return false;

    struct model_load_data *mld = &mesh_factory::models[cur_mesh ++];
    struct tms_model *model = model_misc[cur_model];

    int status = T_OK;

    if (mld->path) {
        mld->mesh = tms_model_load(model, mld->path, &status);
    } else {
        // If the base mesh model and the current chosen model are not the same,
        // we will have to reload
        if (model != models[mld->base_id].model) {
            cur_mesh = mld->base_id;
            tms_warnf("Base mesh and shift-mesh model mismatch, reloading from base_mesh ID. (%d)", cur_mesh);

            return true;
        }

        mld->mesh = tms_model_shift_mesh_uv(models[mld->base_id].model, models[mld->base_id].mesh, mld->offset.x, mld->offset.y);
    }
    mld->mesh->id = cur_mesh - 1;
    mld->model = model;

    if (model->vertices->size > 2000000) {
        // We exceeded 2 mil vertices with this mesh, load it again into another model
        ++ cur_model;
        -- cur_mesh;
    }

    if (status != T_OK) {
        tms_errorf("Error loading mesh %s into model: %d", mld->path, status);
    }

    return (cur_mesh < NUM_MODELS);
}
