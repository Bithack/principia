#include "model.hh"
#include "misc.hh"
#include "pkgman.hh"
#include "settings.hh"

#define NUM_MISC_MODELS 3
struct tms_model *model_misc[NUM_MISC_MODELS];
int cur_model = 0;

#define MODEL_CACHE_VERSION 34

static int i1o1_shift_i = 1;
static int i2o1_shift_i = 1;
static int cpad_shift_i = 1;

struct model_load_data mesh_factory::models[NUM_MODELS] = {
    {"data/models/plank1.3ds"},
    {"data/models/plank2.3ds"},
    {"data/models/plank3.3ds"},
    {"data/models/plank4.3ds"},
    {"data/models/thinplank1.3ds"},
    {"data/models/thinplank2.3ds"},
    {"data/models/thinplank3.3ds"},
    {"data/models/thinplank4.3ds"},
    {"data/models/splank.3ds"},
    {"data/models/splank1.3ds"},
    {"data/models/splank2.3ds"},
    {"data/models/splank3.3ds"},
    {"data/models/splank_front.3ds"},
    {"data/models/splank_back.3ds"},
    {"data/models/impact1.3ds"},
    {"data/models/impact2.3ds"},
    {"data/models/impact3.3ds"},
    {"data/models/impact4.3ds"},
    {"data/models/field1.3ds"},
    {"data/models/field2.3ds"},
    {"data/models/field3.3ds"},
    {"data/models/field4.3ds"},
    {"data/models/room.3ds"},
    {"data/models/room_corner_1.3ds"},
    {"data/models/room_corner_2.3ds"},
    {"data/models/room_corner_3.3ds"},
    {"data/models/room_corner_4.3ds"},
    {"data/models/arm_shotgun.3ds"},
    {"data/models/arm_bomber.3ds"},
    {"data/models/arm_bomber_chamber.3ds"},
    {"data/models/arm_railgun.3ds"},
    {"data/models/diamond.3ds"},
    {"data/models/ruby.3ds"},
    {"data/models/stone.3ds"},
    {"data/models/weight.3ds"},
    {"data/models/separator.3ds"},
    {"data/models/robot.3ds"},
    {"data/models/robot_gunarm.3ds"},
    {"data/models/robot_dragarm.3ds"},
    {"data/models/robot_head.3ds"},
    {"data/models/robot_body.3ds"},
    {"data/models/robot_back.3ds"},
    {"data/models/robot_front.3ds"},
    {"data/models/feet_frame.3ds"},
    {"data/models/bot.3ds"},
    {"data/models/spikebot-2.3ds"},
    {"data/models/minibot.3ds"},
    {"data/models/bomber.3ds"},
    {"data/models/sphere.3ds"},
    {"data/models/sphere2.3ds"},
    {"data/models/sphere3.3ds"},
    {"data/models/generator.3ds"},
    {"data/models/battery3v.3ds"},
    {"data/models/wmotor.3ds"},
    {"data/models/flatmotor.3ds"},
    {"data/models/simplemotor.3ds"},
    {"data/models/gear0.3ds"},
    {"data/models/gear1.3ds"},
    {"data/models/gear2.3ds"},
    {"data/models/gear3.3ds"},
    {"data/models/trampolinebase.3ds"},
    {"data/models/trampolinepad.3ds"},
    {"data/models/plug.simple.3ds"},
    {"data/models/plug.simple.low.3ds"},
    {"data/models/plug.male.3ds"},
    {"data/models/plug.female.3ds"},
    {"data/models/plug.transmitter.3ds"},
    {"data/models/c_ifplug.male.3ds"},
    {"data/models/c_ifplug.female.3ds"},
    {"data/models/script.3ds"},
    {"data/models/sticky.3ds"},
    {"data/models/breadboard.3ds"},
    {"data/models/motor.3ds"},
    {"data/models/wheel.3ds"},
    {"data/models/cup.3ds"},
    {"data/models/cylinder05.3ds"},
    {"data/models/cylinder1.3ds"},
    {"data/models/cylinder1.5.3ds"},
    {"data/models/cylinder2.3ds"},
    {"data/models/wallthing00.3ds"},
    {"data/models/wallthing0.3ds"},
    {"data/models/wallthing1.3ds"},
    {"data/models/wallthing2.3ds"},
    {"data/models/joint.3ds"},
    {"data/models/plate.3ds"},
    {"data/models/platejoint_damaged.3ds"},
    {"data/models/pivotjoint.3ds"},
    {"data/models/corner.3ds"},
    {"data/models/panel.big.3ds"},
    {"data/models/panel.medium.3ds"},
    {"data/models/panel.small.3ds"},
    {"data/models/switch.3ds"},
    {"data/models/robotman.3ds"},
    {"data/models/magnet.3ds"},
    {"data/models/railstraight.3ds"},
    {"data/models/railturn.3ds"},
    {"data/models/railskewed.3ds"},
    {"data/models/railskewed2.3ds"},
    {"data/models/gyroscope.3ds"},
    {"data/models/tiltmeter.3ds"},
    {"data/models/lmotor0.3ds"},
    {"data/models/lmotor1.3ds"},
    {"data/models/lmotor2.3ds"},
    {"data/models/lmotor3.3ds"},
    {"data/models/lmotor0_r.3ds"},
    {"data/models/lmotor1_r.3ds"},
    {"data/models/lmotor2_r.3ds"},
    {"data/models/lmotor3_r.3ds"},
    {"data/models/magplug.3ds"},
    {"data/models/magsocket.3ds"},
    {"data/models/fifo.3ds"},
    {"data/models/seesaw.3ds"},
    {"data/models/pivot.3ds"},
    {"data/models/anchor.3ds"},
    {"data/models/limb.3ds"},
    {"data/models/bullet.3ds"},
    {"data/models/missile.3ds"},
    {"data/models/ropeend.3ds"},
    {"data/models/pipeline.base.3ds"},
    {"data/models/pipeline.piston.3ds"},
    {"data/models/pipeline.house.3ds"},
    {"data/models/clip.3ds"},
    {"data/models/cclip.3ds"},
    {"data/models/landmine.3ds"},
    {"data/models/gearbox.3ds"},
    {"data/models/box_notex.3ds"},
    {"data/models/box_tex.3ds"},
    {"data/models/tribox_tex0.3ds"},
    {"data/models/tribox_tex1.3ds"},
    {"data/models/tribox_tex2.3ds"},
    {"data/models/tribox_tex3.3ds"},
    {"data/models/gb.axle.3ds"},
    {"data/models/gb.4.3ds"},
    {"data/models/gb.6.3ds"},
    {"data/models/gb.8.3ds"},
    {"data/models/gb.10.3ds"},
    {"data/models/gb.12.3ds"},
    {"data/models/controller.mini.3ds"},
    {"data/models/controller.pass.3ds"},
    {"data/models/controller.servo.3ds"},
    {"data/models/controller.fplus.3ds"},
    {"data/models/damper_0.3ds"},
    {"data/models/damper_1.3ds"},
    {"data/models/display.3ds"},
    {"data/models/display_active.3ds"},
    {"data/models/crane.3ds"},
    {"data/models/border-new.3ds"},
    {"data/models/scanner.3ds"},
    {"data/models/mirror.3ds"},
    {"data/models/lasersensor.3ds"},
    {"data/models/rocket.3ds"},
    {"data/models/thruster.3ds"},
    {"data/models/bomb.3ds"},
    {"data/models/plasma_gun_inner.3ds"},
    {"data/models/plasma_gun.3ds"},
    {"data/models/tesla_gun.3ds"},
    {"data/models/mega_buster.3ds"},
    {"data/models/arm_rocket_launcher.3ds"},
    {"data/models/btn.3ds"},
    {"data/models/btn_switch.3ds"},
    {"data/models/proximity.3ds"},
    {"data/models/gameman.3ds"},
    {"data/models/debris.3ds"},
    {"data/models/box1.3ds"},
    {"data/models/box2.3ds"},
    {"data/models/debugger_0.3ds"},
    {"data/models/debugger_1.3ds"},
    {"data/models/graph.3ds"},
    {"data/models/estabilizer.3ds"},
    {"data/models/factory_generic.3ds"},
    {"data/models/factory_robot.3ds"},
    {"data/models/factory_armory.3ds"},
    {"data/models/factory_oil_mixer.3ds"},
    {"data/models/repair_station.3ds"},
    {"data/models/ladder.3ds"},
    {"data/models/jetpack.3ds"},
    {"data/models/advanced_jetpack.3ds"},
    {"data/models/pig.3ds"},
    {"data/models/cow.3ds"},
    {"data/models/cow_head.3ds"},
    {"data/models/pig_head.3ds"},
    {"data/models/leaves1.3ds"},
    {"data/models/emitter.3ds"},
    {"data/models/emitter_frame.3ds"},
    {"data/models/miniemitter.3ds"},
    {"data/models/rubberend.3ds"},
    {"data/models/i0o1.3ds"},
    {"data/models/i0o2.3ds"},
    {"data/models/i0o3.3ds"},
    {"data/models/i1o0.3ds"},
    {"data/models/i1o2.3ds"},
    {"data/models/i1o3.3ds"},
    {"data/models/i1o4.3ds"},
    {"data/models/i1o8.3ds"},
    {"data/models/i2o0.3ds"},
    {"data/models/i2o2.3ds"},
    {"data/models/i3o1.3ds"},
    {"data/models/i4o0.3ds"},
    {"data/models/i4o1.3ds"},
    {"data/models/pointer_body.3ds"},
    {"data/models/pointer_arrow.3ds"},
    {"data/models/dragfield.3ds"},
    {"data/models/gravity.3ds"},
    {"data/models/gravityset.3ds"},
    {"data/models/spikes.3ds"},
    {"data/models/backpack.3ds"},
    {"data/models/adamper.3ds"},
    {"data/models/checkpoint.3ds"},
    {"data/models/fan.3ds"},
    {"data/models/fan_blades.3ds"},
    {"data/models/transmitter.3ds"},
    {"data/models/barrel.3ds"},
    {"data/models/heisenberg.3ds"},
    {"data/models/wizardhat.3ds"},
    {"data/models/ninjahelmet.3ds"},
    {"data/models/robe.3ds"},
    {"data/models/suctioncup.3ds"},
    {"data/models/canister.3ds"},
    {"data/models/oilrig.3ds"},
    {"data/models/conveyor0.3ds"},
    {"data/models/conveyor1.3ds"},
    {"data/models/conveyor2.3ds"},
    {"data/models/conveyor3.3ds"},
    {"data/models/conveyor4.3ds"},
    {"data/models/conveyor5.3ds"},
    {"data/models/robot_head_inside.3ds"},
    {"data/models/conicalhat.3ds"},
    {"data/models/ostrich.3ds"},
    {"data/models/ostrich_head.3ds"},
    {"data/models/policehat.3ds"},
    {"data/models/circuit.3ds"},
    {"data/models/oilbarrel.3ds"},
    {"data/models/canister_armour.3ds"},
    {"data/models/canister_speed.3ds"},
    {"data/models/canister_jump.3ds"},
    {"data/models/ore.3ds"},
    {"data/models/ore_inside.3ds"},
    {"data/models/ore2.3ds"},
    {"data/models/ore2_inside.3ds"},
    {"data/models/vendor.3ds"},
    {"data/models/treasurechest.3ds"},
    {"data/models/builder.3ds"},
    {"data/models/miner.3ds"},
    {"data/models/factionwand.3ds"},
    {"data/models/boltset_steel.3ds"},
    {"data/models/boltset_wood.3ds"},
    {"data/models/boltset_titanium.3ds"},
    {"data/models/boltset_diamond.3ds"},
    {"data/models/lobber.3ds"},
    {"data/models/spikeball.3ds"},
    {"data/models/guardpoint.3ds"},
    {"data/models/stone1.3ds"},
    {"data/models/stone2.3ds"},
    {"data/models/stone3.3ds"},
    {"data/models/stone4.3ds"},
    {"data/models/stone5.3ds"},
    {"data/models/stone6.3ds"},
    {"data/models/mushroom1.3ds"},
    {"data/models/mushroom2.3ds"},
    {"data/models/mushroom3.3ds"},
    {"data/models/mushroom4.3ds"},
    {"data/models/mushroom5.3ds"},
    {"data/models/mushroom6.3ds"},
    {"data/models/leaves2.3ds"},
    {"data/models/leaves3.3ds"},
    {"data/models/sign1.3ds"},
    {"data/models/sign2.3ds"},
    {"data/models/sign3.3ds"},
    {"data/models/sign4.3ds"},
    {"data/models/wood.3ds"},
    {"data/models/fence.3ds"},
    {"data/models/tophat.3ds"},
    {"data/models/compressor.3ds"},
    {"data/models/compressor_lamp1.3ds"},
    {"data/models/compressor_lamp2.3ds"},
    {"data/models/compressor_lamp3.3ds"},
    {"data/models/compressor_lamp4.3ds"},
    {"data/models/ladder_step.3ds"},
    {"data/models/kingscrown.3ds"},
    {"data/models/dummy_head.3ds"},
    {"data/models/statue_head.3ds"},
    {"data/models/jesterhat.3ds"},
    {"data/models/woodsword.3ds"},
    {"data/models/hammer.3ds"},
    {"data/models/witch_hat.3ds"},
    {"data/models/simple_axe.3ds"},
    {"data/models/saw1.3ds"},
    {"data/models/saw.3ds"},
    {"data/models/saw_blade.3ds"},
    {"data/models/spiked_club.3ds"},
    {"data/models/steel_sword.3ds"},
    {"data/models/baseballbat.3ds"},
    {"data/models/spear.3ds"},
    {"data/models/plant1.3ds"},
    {"data/models/plant2.3ds"},
    {"data/models/plant3.3ds"},
    {"data/models/plant4.3ds"},
    {"data/models/war_axe.3ds"},
    {"data/models/pixel_sword.3ds"},
    {"data/models/hard_hat.3ds"},
    {"data/models/serpent_sword.3ds"},
    {"data/models/pioneer_front.3ds"},
    {"data/models/pioneer_back.3ds"},
    {"data/models/vikinghelmet.3ds"},
    {"data/models/pickaxe.3ds"},
    {"data/models/i1o1.3ds"}, // MODEL_I1O1_EMPTY
    {0, MODEL_I1O1_EMPTY, tvec2f(0.f, -.125f*i1o1_shift_i++)}, // MODEL_I1O1_INTEGER
    {0, MODEL_I1O1_EMPTY, tvec2f(0.f, -.125f*i1o1_shift_i++)}, // MODEL_I1O1_SQUARE
    {0, MODEL_I1O1_EMPTY, tvec2f(0.f, -.125f*i1o1_shift_i++)}, // MODEL_I1O1_SQRT
    {0, MODEL_I1O1_EMPTY, tvec2f(0.f, -.125f*i1o1_shift_i++)}, // MODEL_I1O1_SPARSIFY
    {0, MODEL_I1O1_EMPTY, tvec2f(0.f, -.125f*i1o1_shift_i++)}, // MODEL_I1O1_BESSERWISSER
    {0, MODEL_I1O1_EMPTY, tvec2f(0.f, -.125f*i1o1_shift_i++)}, // MODEL_I1O1_EPSILON
    {0, MODEL_I1O1_EMPTY, tvec2f(0.f, -.125f*i1o1_shift_i++)}, // MODEL_I1O1_INVERT
    {"data/models/i2o1_empty.3ds"}, // MODEL_I2O1_EMPTY
    {"data/models/i2o1_1.3ds"}, // MODEL_I2O1_AND
    {"data/models/i2o1_2.3ds"}, // MODEL_I2O1_OR
    {"data/models/i2o1_3.3ds"}, // MODEL_I2O1_XOR
    {"data/models/i2o1_4.3ds"}, // MODEL_I2O1_NAND
    {"data/models/i2o1_5.3ds"}, // MODEL_I2O1_EQUAL
    {"data/models/i2o1_6.3ds"}, // MODEL_I2O1_LESS
    {"data/models/i2o1_7.3ds"}, // MODEL_I2O1_LESS_EQUAL
    {"data/models/i2o1_8.3ds"}, // MODEL_I2O1_SUM
    {"data/models/i2o1_9.3ds"}, // MODEL_I2O1_WRAP_ADD
    {"data/models/i2o1_10.3ds"}, // MODEL_I2O1_WRAP_SUB
    {"data/models/i2o1_11.3ds"}, // MODEL_I2O1_UNUSED
    {"data/models/cpad.3ds"},
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

static char cache_path[512];
static bool use_cache = false;

static bool
open_cache(lvlbuf *lb)
{
    FILE *fp = fopen(cache_path, "rb");
    if (fp) {
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

    tms_errorf("Unable to open cache_path %s", cache_path);
    return false;
}

static bool
read_cache(lvlbuf *lb)
{
    uint8_t version = lb->r_uint8();
    uint32_t num_meshes = lb->r_uint32();
    uint32_t num_models = lb->r_uint32();

    if (version != MODEL_CACHE_VERSION) {
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
    for (int x=0; x<num_models; ++x) {
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
        free(data);

        m->va = tms_varray_alloc(3);
        tms_varray_map_attribute(m->va, "position", 3, GL_FLOAT, m->vertices);
        tms_varray_map_attribute(m->va, "normal", 3, GL_FLOAT, m->vertices);
        tms_varray_map_attribute(m->va, "texcoord", 2, GL_FLOAT, m->vertices);

        uint32_t num_meshes = lb->r_uint32();

        for (int n=0; n<num_meshes; ++n) {
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

static bool
write_cache(lvlbuf *lb)
{
    lb->w_s_uint8(MODEL_CACHE_VERSION);
    lb->w_s_uint32(NUM_MODELS);
    lb->w_s_uint32(NUM_MISC_MODELS);

    for (int x=0; x<NUM_MISC_MODELS; ++x) {
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

static bool
save_cache(lvlbuf *lb)
{
    FILE *fp = fopen(cache_path, "wb");
    if (fp) {
        fwrite(lb->buf, 1, lb->size, fp);
        fclose(fp);

        return true;
    }

    return false;
}

void
mesh_factory::init_models(void)
{
    snprintf(cache_path, 511, "%s/models.cache", tbackend_get_storage_path());

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
#ifndef TMS_BACKEND_ANDROID
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

void
mesh_factory::upload_models(void)
{
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
            tms_errorf("An error occured while trying write model cache.");
        } else {
            if (!save_cache(&lb)) {
                tms_errorf("An error occured while trying to save model cache to a file. (not enough permission/disk space?)");
            }
        }
    }
}

int cur_mesh = 0; /* extern */

/* Returns true if there are any more models to load */
bool
mesh_factory::load_next(void)
{
    if (cur_mesh >= NUM_MODELS) return false;

    struct model_load_data *mld = &mesh_factory::models[cur_mesh ++];
    struct tms_model *model = model_misc[cur_model];

    int status = T_OK;

    if (mld->path) {
        mld->mesh = tms_model_load(model, mld->path, &status);
    } else {
        /* If the base mesh model and the current chosen model are not the same,
         * we will have to reload */
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
        /* We exceeded 2 mil vertices with this mesh, load it again into another model */
        ++ cur_model;
        -- cur_mesh;
    }

    if (status != T_OK) {
        tms_errorf("Error loading mesh %s into model: %d", mld->path, status);
    }

    return (cur_mesh < NUM_MODELS);
}
