#include "pkgman.hh"
#include <sys/stat.h>
#include <time.h>
#include <dirent.h>

#include "zlib.h"
#include "misc.hh"

/* for lvledit command line tool */
#ifdef _NO_TMS
    #include <stdlib.h>
    #include <stdio.h>

    #define tms_infof(...)
    #define tms_warnf(...)
    #define tms_fatalf(...)
    #define tms_errorf(...)

    static const char *tbackend_get_storage_path() {
        return "/tmp/";
    }
#else
    #include <tms/bindings/cpp/cpp.hh>
#endif

static const char *_level_path[4];
static char *_state_path = 0;
static const char *_cache_path[4];
static char *_cache_state_path = 0;
static const char *_pkg_path[4];
static const char *_dir_names[] = {
    "local", "db", "main", "sys"
};

lvlbuf tmpbuf;
lvlinfo tmplvl;

/* create a new level */
void
lvlinfo::create(int type, uint64_t seed/*=0*/, uint32_t version/*=0*/)
{
    this->local_id = 0;
    this->save_id = 0;
    this->version = (version ? version : LEVEL_VERSION);
    this->type = type;
    this->name[0] = '\0';
    this->name_len = 0;

    if (this->type == LCAT_PARTIAL) {
        return;
    }

    this->community_id = 0;
    this->autosave_id = 0;
    this->revision = 0;
    this->parent_id = 0;
    this->parent_revision = 0;
    this->pause_on_finish = true;
    this->show_score = (type == LCAT_ADVENTURE);
    this->descr_len = 0;
    this->visibility = LEVEL_VISIBLE;

    static const int random_bgs[] = {
        0,1,2,4,8,9,10,11,12,13,14
    };
    static const int random_bgs_adventure[] = {
        0,1,2,4,5,8,9,10,11,12,13,14
    };

    if (type == LCAT_ADVENTURE) {
        this->bg = random_bgs_adventure[rand()%(sizeof(random_bgs_adventure)/sizeof(int))];
    } else {
        this->bg = random_bgs[rand()%(sizeof(random_bgs)/sizeof(int))];
    }

    this->bg_color = 0;

    this->descr = 0;
    this->size_x[0] = 40;
    this->size_x[1] = 40;
    this->size_y[0] = 4;
    this->size_y[1] = 10;
    this->gravity_x = 0;
    this->gravity_y = -20.f;
    this->flags = 0;
    this->min_x = 0;
    this->max_x = 0;
    this->min_y = 0;
    this->max_y = 0;
    this->final_score = 0;
    memset(this->icon, 0, 128*128);

    this->sandbox_cam_x = 0.f;
    this->sandbox_cam_y = 0.f;
    this->sandbox_cam_zoom = 12.f;

    this->velocity_iterations = 10;
    this->position_iterations = 10;

    this->seed = 0;
    this->adventure_id = 0xffffffff;

    this->prismatic_tolerance = 0.0125f;
    this->pivot_tolerance = 0.0125f;

    this->linear_damping = 0.1f;
    this->angular_damping = 0.2f;
    this->joint_friction = 0.3f;
    if (type == LCAT_ADVENTURE) {
        this->dead_enemy_absorb_time = 60.0f;
    } else {
        this->dead_enemy_absorb_time = 10.0f;
    }
    this->time_before_player_can_respawn = 1.5f;
    this->compression_length = 0;

    this->flags |= LVL_ALLOW_RESPAWN_WITHOUT_CHECKPOINT;

    if (type == LCAT_ADVENTURE) {
        this->flags |= LVL_ABSORB_DEAD_ENEMIES;
        this->flags |= LVL_ENABLE_INTERACTIVE_DESTRUCTION;
        this->flags |= LVL_DEAD_CREATURE_DESTRUCTION;

        if (seed) {
            this->flags |= LVL_CHUNKED_LEVEL_LOADING;
            this->flags |= LVL_ALLOW_QUICKSAVING;
            this->show_score = false;
            this->seed = seed;
            this->size_y[0] = 500;
            this->bg = 5;
        }
    }

    this->num_groups = 0;
    this->num_entities = 0;
    this->num_connections = 0;
    this->num_cables = 0;
    this->num_chunks = 0;
    this->state_size = 0;
    this->num_gentypes = 0;
}

/* make sure things added in later version are not used by old levels */
void
lvlinfo::sanity_check()
{
    if (this->version < LEVEL_VERSION_1_5) {
        /* remove flags added in 1.5 */
        this->flags &= ~(
                LVL_CHUNKED_LEVEL_LOADING |
                LVL_DISABLE_CAVEVIEW |
                LVL_DISABLE_ROCKET_TRIGGER_EXPLOSIVES |
                LVL_STORE_SCORE_ON_GAME_OVER |
                LVL_ALLOW_HIGH_SCORE_SUBMISSIONS |
                LVL_LOWER_SCORE_IS_BETTER |
                LVL_DISABLE_ENDSCREENS |
                LVL_ALLOW_QUICKSAVING |
                LVL_DEAD_CREATURE_DESTRUCTION
            );

        /* add flags added in 1.5 that must be enabled by default */
        this->flags |= (
                LVL_ALLOW_RESPAWN_WITHOUT_CHECKPOINT
            );

        this->dead_enemy_absorb_time = 10.0f;
        this->time_before_player_can_respawn = 1.5f;
        this->linear_damping = 0.0f;
        this->angular_damping = 0.0f;
        this->joint_friction = 0.0f;
        this->seed = 0;
        this->adventure_id = 0xffffffff;
        this->compression_length = 0;
    }
}

int
lvlinfo::get_size() const
{
    return sizeof(uint8_t)  /* version */
          +sizeof(uint8_t)  /* type */
          +sizeof(uint32_t) /* community_id */
          +(this->version >= LEVEL_VERSION_1_5 ? sizeof(uint32_t) : 0) /* autosave_id */
          +sizeof(uint32_t) /* revision */
          +sizeof(uint32_t) /* parent_id */
          +sizeof(uint8_t)  /* name_len */
          +sizeof(uint16_t) /* descr_len */
          +sizeof(uint8_t)  /* allow_derivatives */
          +(this->version >= 3 ? sizeof(uint8_t) : 0) /* visibility */
          +(this->version >= 7 ? sizeof(uint32_t) : 0) /* parent_revision */
          +(this->version >= 7 ? sizeof(uint8_t) : 0) /* pause_on_finish */
          +(this->version >= 7 ? sizeof(uint8_t) : 0) /* show_score */

          +sizeof(uint8_t)  /* bg */
          +(this->version >= LEVEL_VERSION_1_5 ? sizeof(uint32_t) : 0) /* bg color, stored as uint32_t */

          +sizeof(uint16_t)  /* size_x */
          +sizeof(uint16_t)  /* size_y */
          +(this->version>=12 ? sizeof(uint16_t) : 0)  /* size_x 2 */
          +(this->version>=12 ? sizeof(uint16_t) : 0)  /* size_y 2 */

          +sizeof(uint8_t)  /* velocity_iterations */
          +sizeof(uint8_t)  /* position_iterations */
          +sizeof(uint32_t)  /* final_score */
          +sizeof(float)  /* sandbox_cam_x */
          +sizeof(float)  /* sandbox_cam_y */
          +sizeof(float)  /* sandbox_cam_zoom */
          +(this->version >= 3 ? sizeof(float) : 0)  /* gravity_x */
          +(this->version >= 3 ? sizeof(float) : 0)  /* gravity_y */
          +(this->version >= 13 ? sizeof(float)*4 : 0) /* bounds */
          +(this->version >= 9 ? sizeof(uint64_t) : 0)  /* flags */

          +(this->version >= LEVEL_VERSION_1_4 ? sizeof(float) : 0)  /* prismatic_tolerance */
          +(this->version >= LEVEL_VERSION_1_4 ? sizeof(float) : 0)  /* pivot_tolerance */

          +(this->version >= LEVEL_VERSION_1_5 ? sizeof(uint64_t) : 0)  /* seed */
          +(this->version >= LEVEL_VERSION_1_5 ? sizeof(uint32_t) : 0)  /* adventure_id */

          /* num groups, entities, connections, cables, chunks, state_size, num_gentypes */
          +(this->version >= LEVEL_VERSION_1_5 ? 7 * sizeof(uint32_t) : 4 * sizeof(uint16_t))

          +(this->version >= LEVEL_VERSION_1_5 ? sizeof(float) : 0) /* linear damping */
          +(this->version >= LEVEL_VERSION_1_5 ? sizeof(float) : 0) /* angular damping */
          +(this->version >= LEVEL_VERSION_1_5 ? sizeof(float) : 0) /* joint friction */
          +(this->version >= LEVEL_VERSION_1_5 ? sizeof(float) : 0)  /* dead enemy absorb time */
          +(this->version >= LEVEL_VERSION_1_5 ? sizeof(float) : 0)  /* time before player can respawn */
          +(this->version >= LEVEL_VERSION_1_5 ? sizeof(uint64_t) : 0)  /* compression buffer length */

          +this->name_len
          +(this->version >= 6 ? 128*128 : 0)
          +this->descr_len;
}

void
lvlinfo::write(lvlbuf *lb)
{
    this->sanity_check();

    //printf("Writing a lvl with type %d", this->type);
    lb->ensure(this->get_size());

    lb->w_uint8(this->version);
    lb->w_uint8(this->type);

    if (this->type != LCAT_PARTIAL) {
        lb->w_uint32(this->community_id);

        if (this->version >= LEVEL_VERSION_1_5) {
            lb->w_uint32(this->autosave_id);
        }

        lb->w_uint32(this->revision);
        lb->w_uint32(this->parent_id);
    }

    lb->w_uint8(this->name_len);

    if (this->type != LCAT_PARTIAL) {
        lb->w_uint16(this->descr_len);
        lb->w_uint8(1);
        if (this->version >= 3) {
            lb->w_uint8(this->visibility);
        }
        if (this->version >= 7) {
            lb->w_uint32(this->parent_revision);
            lb->w_uint8(this->pause_on_finish);
            lb->w_uint8(this->show_score);
        }
        lb->w_uint8(this->bg);
        if (this->version >= LEVEL_VERSION_1_5) {
            lb->w_uint32(this->bg_color);
        }

        if (this->version >= 12) {
            lb->w_uint16(this->size_x[0]);
            lb->w_uint16(this->size_x[1]);
            lb->w_uint16(this->size_y[0]);
            lb->w_uint16(this->size_y[1]);
        } else {
            lb->w_uint16(this->size_x[0]+this->size_x[1]);
            lb->w_uint16(this->size_y[0]+this->size_y[1]);
        }

        lb->w_uint8(this->velocity_iterations);
        lb->w_uint8(this->position_iterations);
        lb->w_uint32(this->final_score);
        lb->w_float(this->sandbox_cam_x);
        lb->w_float(this->sandbox_cam_y);
        lb->w_float(this->sandbox_cam_zoom);

        if (this->version >= 3) {
            lb->w_float(this->gravity_x);
            lb->w_float(this->gravity_y);
        }
    }

    if (this->version >= 13) {
        lb->w_float(this->min_x);
        lb->w_float(this->max_x);
        lb->w_float(this->min_y);
        lb->w_float(this->max_y);
    }

    if (this->type != LCAT_PARTIAL) {
        if (this->version >= 9) {
            lb->w_uint64(this->flags);
        }

        if (this->version >= LEVEL_VERSION_1_4) {
            lb->w_float(this->prismatic_tolerance);
            lb->w_float(this->pivot_tolerance);
        }

        if (this->version >= LEVEL_VERSION_1_5) {
            lb->w_uint64(this->seed);
            lb->w_uint32(this->adventure_id);
            lb->w_float(this->linear_damping);
            lb->w_float(this->angular_damping);
            lb->w_float(this->joint_friction);
            lb->w_float(this->dead_enemy_absorb_time);
            lb->w_float(this->time_before_player_can_respawn);
            lb->w_uint64(this->compression_length);
        }
    }

    if (this->name_len) {
        lb->w_buf(this->name, this->name_len);
    }

    if (this->type != LCAT_PARTIAL) {
        if (this->version >= 6) {
            int nn = 0;
            int n_max = 0;
            for (int y=0; y<128*128; y++) {
                if (icon[y] != 0) nn ++;
                if (icon[y] > n_max) n_max = icon[y];
            }
            lb->w_buf((const char *)&this->icon, 128*128);
        }
        if (this->descr_len)
            lb->w_buf(this->descr, this->descr_len);
    }

    if (this->version >= LEVEL_VERSION_1_5) {
        lb->w_uint32(this->num_groups);
        lb->w_uint32(this->num_entities);
        lb->w_uint32(this->num_connections);
        lb->w_uint32(this->num_cables);
        lb->w_uint32(this->num_chunks);
        lb->w_uint32(this->state_size);
        lb->w_uint32(this->num_gentypes);
    } else {
        lb->w_uint16((uint16_t)this->num_groups);
        lb->w_uint16((uint16_t)this->num_entities);
        lb->w_uint16((uint16_t)this->num_connections);
        lb->w_uint16((uint16_t)this->num_cables);
    }
}

bool
lvlinfo::read(lvlbuf *lb, bool skip_description)
{
    this->version = lb->r_uint8();
    if (this->version > LEVEL_VERSION) {
        return false;
    }

    this->type = lb->r_uint8();

    if (this->type != LCAT_PARTIAL) {
        this->community_id = lb->r_uint32();

        if (this->version >= LEVEL_VERSION_1_5) {
            this->autosave_id = lb->r_uint32();
        }

        this->revision = lb->r_uint32();
        this->parent_id = lb->r_uint32();
    }

    this->name_len = lb->r_uint8();

    if (this->type != LCAT_PARTIAL) {
        this->descr_len = lb->r_uint16();
        // XXX: Get rid of this value in next level version (allow_derivatives)
        (void)lb->r_uint8();
        if (this->version >= 3)
            this->visibility = lb->r_uint8();
        else
            this->visibility = LEVEL_VISIBLE;
        if (this->version >= 7) {
            this->parent_revision = lb->r_uint32();
            this->pause_on_finish = (bool)lb->r_uint8();
            this->show_score = (bool)lb->r_uint8();
        } else {
            this->parent_revision = 0;
            this->pause_on_finish = true;
            this->show_score = 1;
        }
        this->bg = lb->r_uint8();

        if (this->version >= LEVEL_VERSION_1_5) {
            this->bg_color = lb->r_uint32();
        }

        if (this->version >= 12) {
            this->size_x[0] = lb->r_uint16();
            this->size_x[1] = lb->r_uint16();
            this->size_y[0] = lb->r_uint16();
            this->size_y[1] = lb->r_uint16();
        } else {
            this->size_x[0] = lb->r_uint16();
            this->size_y[0] = lb->r_uint16();
            //tms_infof("read sizes %u %u", this->size_x[0], this->size_y[0]);
            this->size_x[1] = this->size_x[0];
            this->size_y[1] = this->size_y[0];
        }

        this->velocity_iterations = lb->r_uint8();
        this->position_iterations = lb->r_uint8();
        this->final_score = lb->r_uint32();
        this->sandbox_cam_x = lb->r_float();
        this->sandbox_cam_y = lb->r_float();
        this->sandbox_cam_zoom = lb->r_float();

        if (this->version >= 3) {
            this->gravity_x = lb->r_float();
            this->gravity_y = lb->r_float();
        } else {
            this->gravity_x = 0.f;
            this->gravity_y = -20.f;
        }
    }

    if (this->version >= 13) {
        min_x = lb->r_float();
        max_x = lb->r_float();
        min_y = lb->r_float();
        max_y = lb->r_float();
    } else {
        min_x = 0.f;
        max_x = 0.f;
        min_y = 0.f;
        max_y = 0.f;
    }

    if (this->type != LCAT_PARTIAL) {
        if (this->version >= 9) {
            this->flags = lb->r_uint64();

            if (this->version < LEVEL_VERSION_1_5) {
                this->flags |= LVL_ALLOW_RESPAWN_WITHOUT_CHECKPOINT;
            }
        } else {
            this->flags = 0;
        }

        if (this->version >= 26) {
            this->prismatic_tolerance = lb->r_float();
            this->pivot_tolerance = lb->r_float();
        } else {
            this->prismatic_tolerance = 0.f;
            this->pivot_tolerance = 0.f;
        }

        if (this->version >= LEVEL_VERSION_1_5) {
            this->seed = lb->r_uint64();
            this->adventure_id = lb->r_uint32();
            this->linear_damping = lb->r_float();
            this->angular_damping = lb->r_float();
            this->joint_friction = lb->r_float();
            this->dead_enemy_absorb_time = lb->r_float();
            this->time_before_player_can_respawn = lb->r_float();
            this->compression_length = lb->r_uint64();
        } else {
            this->seed = 0;
            this->adventure_id = 0xffffffff;
            this->linear_damping = 0.f;
            this->angular_damping = 0.f;
            this->joint_friction = 0.f;
            this->dead_enemy_absorb_time = 10.f;
            this->time_before_player_can_respawn = 0.f;
            this->compression_length = 0;
        }
    }

    if (this->name_len) {
        lb->r_buf(this->name, this->name_len);
    }

    if (this->type != LCAT_PARTIAL) {
        if (this->version >= 6) {
            //tms_infof(" reading icon, pt: %d, end: %d", lb->rp, lb->size);
            lb->r_buf((char *)&this->icon, 128*128);
        } else
            memset(this->icon, 0, 128*128);

        if (this->descr_len && !skip_description) {
            this->descr = (char*)realloc(this->descr, this->descr_len+1);
            lb->r_buf(this->descr, this->descr_len);
            this->descr[this->descr_len] = '\0';
        }
    }

    if (this->version >= LEVEL_VERSION_1_5) {
        this->num_groups = lb->r_uint32();
        this->num_entities = lb->r_uint32();
        this->num_connections = lb->r_uint32();
        this->num_cables = lb->r_uint32();
        this->num_chunks = lb->r_uint32();
        this->state_size = lb->r_uint32();
        this->num_gentypes = lb->r_uint32();
    } else {
        this->num_groups = lb->r_uint16();
        this->num_entities = lb->r_uint16();
        this->num_connections = lb->r_uint16();
        this->num_cables = lb->r_uint16();
        this->num_chunks = 0;
        this->state_size = 0;
        this->num_gentypes = 0;
    }

    this->sanity_check();

    return true;
}

uint32_t get_next_id(const char *storage, const char *ext) {
    char path[1024];
    struct stat s;

    // Check IDs up to 2 billion. Ought to be enough for everyone.
    for (uint32_t x = 1; x < 2e9; x++) {
        snprintf(path, 1023, "%s/%d.%s", storage, x, ext);
        int i = stat(path, &s);

        if (i == -1)
            return x;
    }

    return 0;
}

uint32_t
pkgman::get_next_pkg_id()
{
    return get_next_id(pkgman::get_pkg_path(LEVEL_LOCAL), "ppkg");
}

uint32_t
pkgman::get_next_level_id()
{
    return get_next_id(pkgman::get_level_path(LEVEL_LOCAL), "plvl");
}

uint32_t
pkgman::get_next_object_id()
{
    return get_next_id(pkgman::get_level_path(LEVEL_LOCAL), "pobj");
}

bool
pkginfo::save()
{
    if (this->type < 0 || this->type >= 3) {
        tms_errorf("invalid level type");
        return false;
    }

    char path[1024];
    char *storage = (char*)pkgman::get_pkg_path(this->type);
    snprintf(path, 1023, "%s/%d.ppkg", storage, this->id);

    FILE *fp = fopen(path, "wb");

    if (fp) {
        /* always update the version to the latest on save */
        this->version = PKG_VERSION;

        fwrite(&this->version, 1, 1, fp);
        fwrite(&this->community_id, 1, sizeof(uint32_t), fp);
        fwrite(this->name, 1, 255, fp);
        fwrite(&this->unlock_count, 1, sizeof(uint8_t), fp);
        fwrite(&this->first_is_menu, 1, sizeof(uint8_t), fp);
        fwrite(&this->return_on_finish, 1, sizeof(uint8_t), fp);
        fwrite(&this->num_levels, 1, sizeof(uint8_t), fp);

        for (int x=0; x<this->num_levels; x++) {
            fwrite(&this->levels[x], 1, sizeof(uint32_t), fp);
        }

        fclose(fp);
    } else {
        tms_errorf("could not open: %s", path);
        return false;
    }

    return true;
}

bool
pkginfo::open(int type, uint32_t id)
{
    if (type < 0 || type >= 4) {
        tms_errorf("invalid level type");
        return "";
    }

    char path[1024];
    char *storage = (char*)pkgman::get_pkg_path(type);
    snprintf(path, 1023, "%s/%d.ppkg", storage, id);

    this->type = type;
    this->id = id;

    if (this->levels) free(this->levels);
    this->levels = 0;
    this->num_levels = 0;
    this->name[0] = '\0';
    this->name[255] = '\0';

    FILE_IN_ASSET(type == LEVEL_MAIN);

    FILE *fp = _fopen(path, "rb");

    if (fp) {
        _fread(&this->version, 1, 1, fp);
        if (this->version >= 2)
            _fread(&this->community_id, 1, sizeof(uint32_t), fp);
        else
            this->community_id = 0;

        _fread(this->name, 1, 255, fp);

        if (this->version >= 3)
            _fread(&this->unlock_count, 1, sizeof(uint8_t), fp);

        if (this->version >= 1) {
            _fread(&this->first_is_menu, 1, sizeof(uint8_t), fp);
            _fread(&this->return_on_finish, 1, sizeof(uint8_t), fp);
        }

        _fread(&this->num_levels, 1, sizeof(uint8_t), fp);

        if (this->num_levels)
            this->levels = (uint32_t*)malloc(this->num_levels * sizeof(uint32_t));

        for (int x=0; x<this->num_levels; x++) {
            _fread(&this->levels[x], 1, sizeof(uint32_t), fp);
        }

        _fclose(fp);
    } else {
        return false;
    }

    return true;
}

const char *
pkgman::get_pkg_path(int type)
{
    if (type < 0 || type >= 4) {
        tms_errorf("invalid level type");
        return "";
    }

    if (!_pkg_path[type]) {
        _pkg_path[type] = (char*)malloc(1024); /* XXX free this somewhere */

        if (type == LEVEL_MAIN) {
            /* main levels are stored internally in data/ */
            snprintf((char*)_pkg_path[type], 1023, "data/pkg/%s",
                    _dir_names[type]);
        } else {
            snprintf((char*)_pkg_path[type], 1023, "%s/pkg/%s",
                    tbackend_get_storage_path(),
                    _dir_names[type]);
        }
    }

    return _pkg_path[type];
}

const char *
pkgman::get_level_ext(int level_type)
{
    if (level_type == LEVEL_PARTIAL) return "pobj";
    else if (level_type >= LEVEL_LOCAL_STATE) return "psav";
    else return "plvl";
}

const char *
pkgman::get_level_path(int level_type)
{
    if (level_type == LEVEL_PARTIAL) level_type = LEVEL_LOCAL;

    if (level_type >= LEVEL_LOCAL_STATE) {
        if (!_state_path) {
            _state_path = (char*)malloc(1024); /* XXX free this somewhere */
            snprintf((char*)_state_path, 1023,
                    "%s/sav",
                     tbackend_get_storage_path());
        }

        return _state_path;
    }

    if (level_type < 0 || level_type >= 4) {
        tms_errorf("invalid level type");
        return "";
    }

    if (!_level_path[level_type]) {
        _level_path[level_type] = (char*)malloc(1024); /* XXX free this somewhere */

        if (level_type == LEVEL_MAIN) {
            /* main levels are stored internally in data/ */
            snprintf((char*)_level_path[level_type], 1023,
                    "data/lvl/%s",
                    _dir_names[level_type]);
        } else {
            snprintf((char*)_level_path[level_type], 1023,
                    "%s/lvl/%s",
                    tbackend_get_storage_path(),
                    _dir_names[level_type]);
        }
    }

    return _level_path[level_type];
}

const char *state_prefixes[3] = {
    "local",
    "db",
    "unknown",
};

const char *
pkgman::get_state_prefix(int level_type)
{
    if (level_type >= LEVEL_LOCAL_STATE) {
        level_type -= LEVEL_LOCAL_STATE;
    }

    if (level_type == LEVEL_LOCAL) {
        return state_prefixes[0];
    } else if (level_type == LEVEL_DB) {
        return state_prefixes[1];
    }

    tms_errorf("Unknown state prefix for level type %d", level_type);

    return state_prefixes[2];
}

const char *
pkgman::get_cache_path(int level_type)
{
    if (level_type == LEVEL_PARTIAL) level_type = LEVEL_LOCAL;

    if (level_type >= LEVEL_LOCAL_STATE) {
        if (!_cache_state_path) {
            _cache_state_path = (char*)malloc(1024);

            snprintf((char*)_cache_state_path, 1023,
                    "%s/cache/sav",
                    tbackend_get_storage_path());
        }

        return _cache_state_path;
    }

    if (level_type < 0 || level_type >= 4) {
        tms_errorf("invalid level type");
        return "";
    }

    if (!_cache_path[level_type]) {
        _cache_path[level_type] = (char*)malloc(1024); /* XXX free this somewhere */

        snprintf((char*)_cache_path[level_type], 1023,
                "%s/cache/%s",
                tbackend_get_storage_path(),
                _dir_names[level_type]);
    }

    return _cache_path[level_type];
}

void
pkgman::get_level_full_path(int level_type, uint32_t id, uint32_t save_id, char *output)
{
    if (level_type >= LEVEL_LOCAL_STATE) {
        snprintf(output, 1023, "%s/%s.%d.%u.%s",
                pkgman::get_level_path(level_type),
                pkgman::get_state_prefix(level_type),
                id,
                save_id,
                pkgman::get_level_ext(level_type));
    } else {
        snprintf(output, 1023, "%s/%d.%s",
                pkgman::get_level_path(level_type),
                id,
                pkgman::get_level_ext(level_type));
    }
}

void
pkgman::get_cache_full_path(int level_type, uint32_t id, uint32_t save_id, char *output)
{
    if (level_type >= LEVEL_LOCAL_STATE) {
        snprintf(output, 1023, "%s/%s.%u.%u.pcache",
                pkgman::get_cache_path(level_type),
                pkgman::get_state_prefix(level_type),
                id,
                save_id);
    } else {
        snprintf(output, 1023, "%s/%d.pcache",
                pkgman::get_cache_path(level_type),
                id);
    }
}

bool
pkgman::get_level_name(int level_type, uint32_t id, uint32_t save_id, char *output)
{
    char filename[1024];

    pkgman::get_level_full_path(level_type, id, save_id, filename);

    FILE_IN_ASSET(level_type == LEVEL_MAIN);

    FILE *fp = _fopen(filename, "rb");
    if (fp) {
        tmpbuf.reset();
        tmpbuf.ensure(sizeof(lvlinfo));

        _fread(tmpbuf.buf, 1, sizeof(lvlinfo), fp);
        tmpbuf.size = sizeof(lvlinfo);

        tmplvl.read(&tmpbuf, true);

        if (tmplvl.name_len == 0) {
            strcpy(output, "<no name>");
        } else {
            memcpy(output, tmplvl.name, tmplvl.name_len*sizeof(char));
            output[tmplvl.name_len] = '\0';
        }

        _fclose(fp);

        return true;
    } else {
        strcpy(output, "<no name>");
        tms_warnf("unable to open file for lid %u", id);
    }

    return false;
}

bool
pkgman::get_level_data(int level_type, uint32_t id, uint32_t save_id, char *o_name, uint8_t *o_version)
{
    char filename[1024];

    pkgman::get_level_full_path(level_type, id, save_id, filename);

    FILE_IN_ASSET(level_type == LEVEL_MAIN);

    FILE *fp = _fopen(filename, "rb");
    if (fp) {
        tmpbuf.reset();
        tmpbuf.ensure(sizeof(lvlinfo));

        _fread(tmpbuf.buf, 1, sizeof(lvlinfo), fp);
        tmpbuf.size = sizeof(lvlinfo);

        tmplvl.read(&tmpbuf, true);

        if (tmplvl.name_len == 0) {
            strcpy(o_name, "<no name>");
        } else {
            memcpy(o_name, tmplvl.name, tmplvl.name_len*sizeof(char));
            o_name[tmplvl.name_len] = '\0';
        }

        *o_version = tmplvl.version;

        _fclose(fp);

        return true;
    } else {
        tms_warnf("unable to open file for lid %u", id);
    }

    return false;
}

pkginfo*
pkgman::get_pkgs(int type)
{
    if (type < 0 || type >= 4)
        return 0;

    const char *path = pkgman::get_pkg_path(type);

    DIR *dir;
    struct dirent *ent;
    pkginfo *first = 0, *curr = 0;

    if ((dir = opendir(path))) {

        while ((ent = readdir(dir))) {
            int len = strlen(ent->d_name);
            if (len > 5 && memcmp(&ent->d_name[len-5], ".ppkg", 5) == 0) {
                uint32_t pkg_id = atoi(ent->d_name);

                if (pkg_id != 0) {
                    pkginfo *ff = new pkginfo();

                    if (ff->open(type, pkg_id)) {
                        if (curr) curr->next = ff;
                        if (!first) first = ff;
                        curr = ff;
                    } else {
                        tms_errorf("could not open pkg %d", pkg_id);
                        delete ff;
                    }
                }
            }
        }
        closedir(dir);
    }

    return first;
}

#ifdef TMS_BACKEND_WINDOWS
time_t
filetime_to_timet(FILETIME & ft)
{
    ULARGE_INTEGER ull;
    ull.LowPart = ft.dwLowDateTime;
    ull.HighPart = ft.dwHighDateTime;
    return ull.QuadPart / 10000000ULL - 11644473600ULL;
}
#endif

lvlfile*
pkgman::get_levels(int level_type)
{
    bool state = false;

    int orig_level_type = level_type;

    if (level_type >= LEVEL_LOCAL_STATE) {
        level_type -= LEVEL_LOCAL_STATE;
        state = true;
    }

    if (level_type < 0 || level_type >= 5) {
        tms_warnf("unknown level type");
        return 0;
    }

    char ext[6];
    snprintf(ext, 6, ".%s", pkgman::get_level_ext(orig_level_type));
    const char *path = pkgman::get_level_path(orig_level_type);

#ifdef TMS_BACKEND_WINDOWS
    wchar_t tmp[1024];
#else
    char tmp[1024];
#endif

    DIR *dir;
    struct dirent *ent;
    lvlfile *first = 0, *last = 0;

    if ((dir = opendir(path))) {

        while ((ent = readdir(dir))) {
            int len = strlen(ent->d_name);

            if (len > 5 && memcmp(&ent->d_name[len-5], ext, 5) == 0) {
                uint32_t level_id = 0;
                uint32_t save_id = 0;
                uint8_t state_level_type = LEVEL_LOCAL_STATE;

                if (state) {
                    /* State format: {0:s:type}.{1:d:id}.{2:d:save_id}.{3:s:ext} */
                    if (ent->d_name[0] == 'l') {
                        state_level_type = LEVEL_LOCAL_STATE;
                    } else if (ent->d_name[0] == 'd') {
                        state_level_type = LEVEL_DB_STATE;
                    }
                    char *level_id_c = strchr(ent->d_name, '.');
                    level_id = atoi(level_id_c+1);
                    save_id = atoi(strchr(level_id_c+1, '.')+1);
                } else {
                    /* Regular level format: {0:d:id}.{1:s:save_id} */
                    level_id = atoi(ent->d_name);

                    save_id = atoi(strchr(ent->d_name, '.')+1);
                }

                time_t mtime;
                char date[21];

#ifdef TMS_BACKEND_WINDOWS
                WIN32_FILE_ATTRIBUTE_DATA data;
                wsprintf(tmp, L"%hs\\%hs", path, ent->d_name);

                GetFileAttributesEx((LPCWSTR)(tmp), GetFileExInfoStandard, &data);

                FILETIME time = data.ftLastWriteTime;
                SYSTEMTIME sys_time, local_time;

                FileTimeToSystemTime(&time, &sys_time);
                SystemTimeToTzSpecificLocalTime(0, &sys_time, &local_time);
                snprintf(date, 20, "%04d-%02d-%02d %02d:%02d:%02d", local_time.wYear, local_time.wMonth, local_time.wDay, local_time.wHour, local_time.wMinute, local_time.wSecond);
                mtime = filetime_to_timet(time);
#else
                snprintf(tmp, 1023, "%s/%s", path, ent->d_name);
                struct stat st;
                stat(tmp, &st);
                strftime(date, 20, "%Y-%m-%d %H:%M:%S", gmtime((time_t*)&(st.st_mtime)));
                mtime = st.st_mtime;
#endif

                if (level_id != 0 || state) {
                    lvlfile *ff = new lvlfile((state ? state_level_type : level_type), level_id);
                    strcpy(ff->modified_date, date);
                    ff->mtime = mtime;
                    ff->save_id = save_id;

                    /**
                     * For state searches, we do not care about the original level type.
                     * The original level type is merely there to notify us that it's states
                     * we are looking for.
                     * The level type of the current state is discerned via the filename
                     * using the first "dotted" argument (level or db).
                     **/
                    if (pkgman::get_level_data((state ? state_level_type : orig_level_type), level_id, save_id, ff->name, &ff->version)) {
                        if (!first) {
                            first = ff;
                        } else {
                            /* loop through and insert the lvlfile as soon as the date is newer */

                            lvlfile *l = first, *prev = 0;
                            while (l) {
                                if (strcmp(ff->modified_date, l->modified_date) > 0) {
                                    break;
                                }
                                prev = l;
                                l = l->next;
                            }

                            if (!prev) {
                                ff->next = first;
                                first = ff;
                            } else {
                                ff->next = prev->next;
                                prev->next = ff;
                            }
                        }
                    } else {
                        tms_warnf("Unable to get level name for lid %u", level_id);
                        delete ff;
                    }
                }
            }
        }
        closedir(dir);
    } else {
        tms_errorf("could not open directory %s", path);
    }

    return first;
}

uint32_t
pkgman::get_latest_level_id(int level_type)
{
    lvlfile *level = pkgman::get_levels(LEVEL_LOCAL);

    uint32_t latest_id = 0;
    time_t latest_mtime = 0;

    while (level) {
        if (level->mtime > latest_mtime) {
            latest_id = level->id;
            latest_mtime = level->mtime;
        }

        lvlfile *next = level->next;
        delete level;
        level = next;
    }

    return latest_id;
}

extern "C" struct lvlfile* pkgman_get_levels(int level_type)
{
    return pkgman::get_levels(level_type);
}

bool
lvledit::open(int lvl_type, uint32_t lvl_id)
{
    char filename[1024];

    const char *ext, *path;
    ext = pkgman::get_level_ext(lvl_type);
    path = pkgman::get_level_path(lvl_type);

    if (lvl_id == 0) {
        snprintf(filename, 1023, "%s/.autosave", pkgman::get_level_path(lvl_type));
    } else {
        snprintf(filename, 1023, "%s/%d.%s", pkgman::get_level_path(lvl_type), lvl_id, ext);
    }

    this->lvl_type = 0;
    this->lvl_id = 0;

    FILE_IN_ASSET(lvl_type == LEVEL_MAIN);

    FILE *fp = _fopen(filename, "rb");

    if (fp) {
        _fseek(fp, 0, SEEK_END);
        long size = _ftell(fp);
        _fseek(fp, 0, SEEK_SET);

        if (size > 2*1024*1024) {
            tms_fatalf("file too big");
        }

        this->lb.reset();
        this->lb.size = 0;
        this->lb.ensure((int)size);

        _fread(this->lb.buf, 1, size, fp);

        _fclose(fp);

        this->lb.size = size;
        this->lvl.read(&this->lb);
        this->header_size = this->lvl.get_size();
        this->lvl_type = lvl_type;
        this->lvl_id = lvl_id;
    } else {
        return false;
    }

    return true;
}

/* This method is mainly used for the lvledit tool,
 * which means it doesn't check any of the vitals (i.e. if the
 * level is stored in the apk or outside.) */
bool
lvledit::open_from_path(const char *path)
{
    FILE *fp = fopen(path, "rb");

    if (fp) {
        fseek(fp, 0, SEEK_END);
        long size = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        if (size > 2*1024*1024) {
            tms_fatalf("file too big");
        }

        this->lb.reset();
        this->lb.size = 0;
        this->lb.ensure((int)size);

        fread(this->lb.buf, 1, size, fp);

        fclose(fp);

        this->lb.size = size;
        bool r = this->lvl.read(&this->lb);
        if (!r) {
            fprintf(stderr, "uh oh\n");
        }
        this->header_size = this->lvl.get_size();
    } else {
        return false;
    }

    return true;
}

/* IMPORTANT: keep this in sync with of::read() */
void
lvledit::print_gids()
{
    this->lb.rp = this->lvl.get_size();

    uint32_t num_groups = this->lvl.num_groups;
    uint32_t num_entities = this->lvl.num_entities;

    /* skip groups */
#if 0
    this->lb.rp +=
        num_groups *
        (
          sizeof(uint32_t)/*id*/
          +sizeof(float)/*pos.x*/
          +sizeof(float)/*pos.y*/
          +sizeof(float)/*angle*/
          +(version >= LEVEL_VERSION_1_5 ? sizeof(uint32_t) : 0)
        );
#endif
    for (int x=0; x<num_groups; ++x) {
        lb.r_uint32(); /* id */
        lb.rp += sizeof(float) /* pos.x */
               + sizeof(float) /* pos.y */
               + sizeof(float);/* angle */

        if (this->lvl.version >= LEVEL_VERSION_1_5) {
            uint32_t state_size = lb.r_uint32();
            lb.rp += state_size;
        }
    }

    for (int x=0; x<num_entities; ++x) {
        uint8_t np;
        uint8_t nc = 0;
        uint32_t state_size = 0;

        uint8_t g_id = lb.r_uint8(); /* g_id */
        lb.r_uint32(); /* id */
        lb.r_uint16(); /* group_id half */
        lb.r_uint16(); /* group id half */

        printf("%s%u",((x>0)?"\n":""), g_id); /* g_id */

        np = lb.r_uint8(); /* num properties */

        if (this->lvl.version >= LEVEL_VERSION_1_5) {
            nc = lb.r_uint8(); /* num chunks */
            state_size = lb.r_uint32(); /* state size */
        }

        (void)lb.r_float(); /* pos.x */
        (void)lb.r_float(); /* pos.y */
        (void)lb.r_float(); /* angle */
        (void)lb.r_uint8(); /* layer */

        if (this->lvl.version >= LEVEL_VERSION_1_5) {
            lb.r_uint64(); /* state flags */
            /* this includes moveable and axisrot */

            for (int x=0; x<nc; x++) {
                lb.r_int32(); /* chunk x */
                lb.r_int32(); /* chunk y */
            }

            /* skip state buffer */
            lb.rp += state_size;
        } else {
            (void)lb.r_uint8(); /* axisrot */
            if (this->lvl.version >= 10) {
                (void)lb.r_uint8(); /* moveable */
            }
        }

        for (int x=0; x<np; x++) {
            uint8_t type = lb.r_uint8();

            switch (type) {
                case P_INT8: lb.r_uint8(); break;
                case P_INT: case P_ID: lb.r_uint32(); break;
                case P_FLT: lb.r_float(); break;
                case P_STR:
                    {
                        if (this->lvl.version >= LEVEL_VERSION_1_5) {
                            uint32_t len = lb.r_uint32();
                            lb.rp += len;
                        } else {
                            uint16_t len = lb.r_uint16();
                            lb.rp += len;
                        }
                    }
                    break;
                default: fprintf(stderr, "invalid object property %d", type); exit(1);
            }
        }
    }
}

bool
lvledit::save(void)
{
    if (this->lvl.get_size() != this->header_size) {
        /* new header size does not match old header size,
         * we need to perform a memmove on the object data */

        int diff = this->lvl.get_size() - this->header_size;

        if (diff > 0)
            this->lb.ensure(diff);

        char *header_end = (char*)this->lb.buf + this->header_size;
        memmove(header_end + diff, header_end, this->lb.size - this->header_size);

        this->header_size += diff;
        this->lb.size += diff;
    }

    int saved_size = this->lb.size;
    this->lb.size = 0;
    this->lvl.write(&this->lb);
    this->lb.size = saved_size;

    char filename[1024];
    snprintf(filename, 1023, "%s/%d.%s", pkgman::get_level_path(this->lvl_type), this->lvl_id, pkgman::get_level_ext(this->lvl_type));

    FILE *fp = fopen(filename, "wb");

    if (fp) {
        fwrite(this->lb.buf, 1, this->lb.size, fp);
        fclose(fp);

        return true;
    }

    tms_errorf("could not open file '%s' for writing", filename);
    return false;
}

bool
lvledit::save_to_path(const char *path)
{
    if (this->lvl.get_size() != this->header_size) {
        /* new header size does not match old header size,
         * we need to perform a memmove on the object data */

        int diff = this->lvl.get_size() - this->header_size;

        if (diff > 0)
            this->lb.ensure(diff);

        char *header_end = (char*)this->lb.buf + this->header_size;
        memmove(header_end + diff, header_end, this->lb.size - this->header_size);

        this->header_size += diff;
        this->lb.size += diff;
    }

    int saved_size = this->lb.size;
    this->lb.size = 0;
    this->lvl.write(&this->lb);
    this->lb.size = saved_size;

    FILE *fp = fopen(path, "wb");

    if (fp) {
        fwrite(this->lb.buf, 1, this->lb.size, fp);
        fclose(fp);

        return true;
    }

    tms_errorf("could not open file '%s' for writing", path);
    return false;
}

#ifdef DEBUG

void
lvlinfo::print() const
{
    printf("Level headers:\n");

    printf("Level version:       %u\t(%s)\n",
            this->version, level_version_string(this->version));

    printf("Level type:          %u\t(%s)\n",
            this->type, level_type_string(this->type));

    if (this->type != LCAT_PARTIAL) {
        printf("Community ID:        %u\n",
                this->community_id);

        if (this->version >= LEVEL_VERSION_1_5) {
            printf("Autosave ID:         %u\n",
                    this->autosave_id);
        }

        printf("Revision:            %u\n",
                this->revision);

        printf("Parent ID:           %u\n",
                this->parent_id);
    }

    printf("Name len:            %u\n",
            this->name_len);

    if (this->type != LCAT_PARTIAL) {
        printf("Description len:     %u\n",
                this->descr_len);

        printf("Visibility:          %s\n",
                level_visibility_string(this->visibility));

        printf("Visibility:          %u\n",
                this->parent_revision);

        printf("Pause on finish:     %s\n",
                this->pause_on_finish ? "yes" : "no");

        printf("Show score:          %s\n",
                this->show_score ? "yes" : "no");

        printf("Background ID:       %u\n",
                this->bg);

        if (this->version >= LEVEL_VERSION_1_5) {
            printf("Background color:    %u\n",
                    this->bg_color);
        }

        printf("Border left:         %u\n",
                this->size_x[0]);

        printf("Border right:        %u\n",
                this->size_x[1]);

        printf("Border down:         %u\n",
                this->size_y[0]);

        printf("Border up:           %u\n",
                this->size_y[1]);

        printf("Vel iterations:      %u\n",
                this->velocity_iterations);

        printf("Pos iterations:      %u\n",
                this->position_iterations);

        printf("Final score:         %u\n",
                this->final_score);

        printf("Sandbox cam X:       %f\n",
                this->sandbox_cam_x);

        printf("Sandbox cam Y:       %f\n",
                this->sandbox_cam_y);

        printf("Sandbox cam Z:       %f\n",
                this->sandbox_cam_zoom);

        if (this->version >= 3) {
            printf("Gravity X:           %.6f\n",
                    this->gravity_x);
            printf("Gravity Y:           %.6f\n",
                    this->gravity_y);
        }
    }

    printf("Min/Max X:           %.2f/%.2f\n",
            this->min_x,
            this->max_x);

    printf("Min/Max Y:           %.2f/%.2f\n",
            this->min_y,
            this->max_y);

    if (this->type != LCAT_PARTIAL) {
        printf("Flags:               %" PRIu64 "\n",
                this->flags);

        printf("Prismatic tolerance: %.2f\n",
                this->prismatic_tolerance);

        printf("Pivot tolerance:     %.2f\n",
                this->pivot_tolerance);

        printf("Seed:                0x%016" PRIx64 "\n",
                this->seed);

        printf("Adventure ID:        0x%08" PRIx32 "\n",
                this->adventure_id);

        printf("Linear damping:      %.2f\n",
                this->linear_damping);

        printf("Angular damping:     %.2f\n",
                this->angular_damping);

        printf("Joint friction:      %.2f\n",
                this->joint_friction);

        printf("Enemy absorb time:   %.2f\n",
                this->dead_enemy_absorb_time);

        printf("Player respawn time: %.2f\n",
                this->time_before_player_can_respawn);
    }

    if (this->name_len) {
        printf("Name:                (%u) '%s'\n",
                (unsigned)strlen(this->name), this->name);
    }

    if (this->type != LCAT_PARTIAL) {
        if (this->descr_len) {
            printf("Description:         (%u) '%s'\n",
                    (unsigned)strlen(this->descr), this->descr);
        }
    }

    printf("Num groups:          %u\n",
            this->num_groups);

    printf("Num entities:        %u\n",
            this->num_entities);

    printf("Num connections:     %u\n",
            this->num_connections);

    printf("Num chunks:          %u\n",
            this->num_chunks);

    printf("Num gentypes:        %u\n",
            this->num_gentypes);

    printf("State size:          %u\n",
            this->state_size);

    printf("Num cables:          %u\n",
            this->num_cables);
}

#endif

void
lvlbuf::ensure(uint64_t s)
{
    uint64_t ns = this->size + s;
    if (ns > this->cap) {
        uint64_t old_cap = this->cap;
        if (this->sparse_resize) {
            /* round up to nearest power of two */
            this->cap = ns;
            this->cap --;
            this->cap |= this->cap >> 1;
            this->cap |= this->cap >> 2;
            this->cap |= this->cap >> 4;
            this->cap |= this->cap >> 8;
            this->cap |= this->cap >> 16;
            this->cap++;
        } else {
            this->cap = ((ns > this->cap+4096) ? ns+4096 : this->cap+4096);
        }

        this->buf = static_cast<uint8_t*>(realloc(this->buf, this->cap));
    }
}

void
lvlbuf::zcompress(const lvlinfo &level, unsigned char **dest, uint64_t *dest_len) const
{
    uint64_t header_size = level.get_size();

    *dest_len = compressBound(this->size-header_size);
    *dest = (unsigned char*)malloc(*dest_len);

    int ret = compress(*dest, (uLong*)dest_len, this->buf+header_size, this->size-header_size);

    /* TODO: Better error-handling */
    switch (ret) {
        case Z_OK:
            tms_infof("zcompress success");
            break;

        case Z_MEM_ERROR:
            tms_errorf("no mem");
            break;

        case Z_BUF_ERROR:
            tms_errorf("not enough room in buffer");
            break;

        case Z_STREAM_ERROR:
            tms_errorf("z stream error");
            break;

        default:
            tms_infof("unknown ret: %d", ret);
            break;
    }

    tms_infof("Compression status: Old: %" PRIu64 ". New: %" PRIu64 ". Total: %.2f", this->size-header_size, *dest_len, (float)(float)*dest_len/((this->size-header_size)));
}

void
lvlbuf::zuncompress(const lvlinfo &level)
{
    uint64_t header_size = level.get_size();

    uLong dest_len = level.compression_length;
    unsigned char *dest = (unsigned char*)malloc(dest_len);

    int ret = uncompress(dest, &dest_len, this->buf+header_size, this->size-header_size);

    /* TODO: Better error-handling */
    switch (ret) {
        case Z_OK:
            tms_infof("zuncompress success");
            break;

        case Z_MEM_ERROR:
            tms_errorf("no mem");
            break;

        case Z_BUF_ERROR:
            tms_errorf("not enough room in buffer");
            break;

        case Z_STREAM_ERROR:
            tms_errorf("z stream error");
            break;

        default:
            tms_infof("unknown ret: %d", ret);
            break;
    }

    tms_infof("Uncompression status: Old: %" PRIu64 ". New: %u. Total: %.2f", this->size-header_size, (unsigned)dest_len, (float)(float)dest_len/((this->size-header_size)));

    this->size = header_size;
    this->ensure(dest_len);
    this->size = header_size + dest_len;

    memcpy(this->buf+header_size, dest, dest_len);

    free(dest);
}
