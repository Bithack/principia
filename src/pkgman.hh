#pragma once

#define LEVEL_VERSION_ANY     0
#define LEVEL_VERSION_1_0     15
#define LEVEL_VERSION_1_1_6   16
#define LEVEL_VERSION_1_1_7   17
#define LEVEL_VERSION_1_2     18
#define LEVEL_VERSION_1_2_1   19
#define LEVEL_VERSION_1_2_2   20
#define LEVEL_VERSION_1_2_3   21
#define LEVEL_VERSION_1_2_4   22
#define LEVEL_VERSION_1_3_0_1 23
#define LEVEL_VERSION_1_3_0_2 24
#define LEVEL_VERSION_1_3_0_3 25
#define LEVEL_VERSION_1_4     26
#define LEVEL_VERSION_1_4_0_2 27
#define LEVEL_VERSION_1_5     28
#define LEVEL_VERSION_1_5_1   29
#define LEVEL_VERSION_2023_06_05 30
#define LEVEL_VERSION_FUTURE 31

#define PKG_VERSION   3
#define LEVEL_VERSION LEVEL_VERSION_2023_06_05

#define LEVEL_VISIBLE 0
#define LEVEL_LOCKED  1
#define LEVEL_HIDDEN  2

static const char *level_version_strings[] = {
    "Null",
    "Beta 1",
    "Beta 2",
    "Beta 3",
    "Beta 4",
    "Beta 5",
    "Beta 6",
    "Beta 7",
    "Beta 8",
    "Beta 9",
    "Beta 10",
    "Beta 11",
    "Beta 12",
    "Beta 13",
    "Beta 14",
    "1.0",
    "1.1.6",
    "1.1.7",
    "1.2",
    "1.2.1",
    "1.2.2",
    "1.2.3",
    "1.2.4",
    "1.3.0.1",
    "1.3.0.2",
    "1.3.0.3",
    "1.4",
    "1.4.0.2",
    "1.5",
    "1.5.1",
    "2023-06-05",
    "31 (Pending)",
    0,
    0,
    0,
    0,
};

#define LCAT_PUZZLE      0
#define LCAT_ADVENTURE   1
#define LCAT_CUSTOM      2

#define LCAT_PARTIAL     100

// entity property types
#define P_INT    0
#define P_FLT    1
#define P_STR    2
#define P_INT8   3
#define P_ID     4

#define PKG_MAX_LEVELS 1024

#define LEVEL_NAME_MAX_LEN 256
#define LEVEL_DESCR_MAX_LEN (1024*1024*12)

#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#ifdef __cplusplus
#include "const.hh"

class lvlbuf;
class pkginfo;

#endif

inline const char *level_version_string(int level_version) {
    const char *ret = level_version_strings[level_version];

    if (ret)
        return ret;
    else
        return "N/A";
}

static const char *level_type_string(uint8_t level_type) {
    switch (level_type) {
        case LCAT_PARTIAL:      return "Partial";
        case LCAT_PUZZLE:       return "Puzzle";
        case LCAT_ADVENTURE:    return "Adventure";
        case LCAT_CUSTOM:       return "Custom";
        default:                return "Unknown";
    }
}

static const char *level_visibility_string(uint8_t level_visibility) {
    switch (level_visibility) {
        case LEVEL_VISIBLE:     return "Visible";
        case LEVEL_LOCKED:      return "Locked";
        case LEVEL_HIDDEN:      return "Hidden";
        default:                return "Unknown";
    }
}

struct lvlfile {
    uint32_t id;
    uint32_t save_id;
    int id_type;
    char name[LEVEL_NAME_MAX_LEN+1];
    char modified_date[20];
    time_t mtime;
    uint8_t version;
    struct lvlfile *next;

#ifdef __cplusplus
    lvlfile(int id_type, uint32_t id) {
        this->save_id = 0;
        this->id_type = id_type;
        this->id = id;
        this->modified_date[0] = '\0';
        this->mtime = 0;
        this->version = 0;
        this->next = 0;
    }
#endif
} __attribute__ ((__packed__));

#ifdef __cplusplus

class pkgman {
  public:
    static uint32_t get_next_level_id();
    static uint32_t get_next_object_id();
    static uint32_t get_next_pkg_id();
    static const char *get_level_path(int level_type);
    static const char *get_state_prefix(int level_type);
    static const char *get_level_ext(int level_type);
    static const char *get_cache_path(int level_type);
    static const char *get_pkg_path(int type);
    static lvlfile *get_levels(int level_type);
    static uint32_t get_latest_level_id(int level_type);
    static void get_level_full_path(int level_type, uint32_t id, uint32_t save_id, char *output);
    static void get_cache_full_path(int level_type, uint32_t id, uint32_t save_id, char *output);
    static bool get_level_name(int level_type, uint32_t id, uint32_t save_id, char *output);
    static bool get_level_data(int level_type, uint32_t id, uint32_t save_id, char *o_name, uint8_t *o_version);
    static pkginfo* get_pkgs(int type);

    static bool mtime_asc(lvlfile* a, lvlfile* b) {
        return a->mtime < b->mtime;
    }

    static bool mtime_desc(lvlfile* a, lvlfile* b) {
        return a->mtime > b->mtime;
    }
};

/// Class representing a package of levels (stored as .ppkg files)
class pkginfo {
  public:
    uint8_t   version;
    uint32_t  id;
    uint32_t  community_id;
    /// local, db, main
    uint8_t   type;
    char      name[256];
    uint8_t   num_levels;
    uint32_t *levels;
    /// how many levels to unlock past the last completed level
    uint8_t   unlock_count;
    /// if enabled, the first level in the package is used as level selection screen
    uint8_t   first_is_menu;
    /// always return to level selection screen/level when a level is finished, instead of proceeding to the next level
    uint8_t   return_on_finish;

    /// if used as a linked list, as returned by pkgman::get_pkgs()
    pkginfo  *next;

    pkginfo() {
        version = 0;
        community_id = 0;
        type = LEVEL_LOCAL;
        num_levels = 0;
        levels = 0;
        first_is_menu = false;
        return_on_finish = false;
        unlock_count = 2;
        next = 0;
        name[0] = '\0';
    }

    uint32_t get_next_level(uint32_t id);

    bool is_level_locked(uint8_t index);

    uint8_t get_level_index(uint32_t id) {
        for (int x=0; x<this->num_levels; x++)
            if (this->levels[x] == id) return x;

        return 0;
    }

    uint32_t get_level_by_index(uint8_t index) {
        if (index < this->num_levels)
            return this->levels[index];

        return 0;
    }

    void clear_levels() {
        this->num_levels = 0;

        if (this->levels) {
            free(this->levels);
            this->levels = 0;
        }
    }

    bool add_level(uint32_t id);

    /// Open package by type and ID.
    bool open(int type, uint32_t id);

    /// Save current package as .ppkg determined by type and ID.
    bool save();

    /// Save package to a specific path, rather than the default path determined by type and ID.
    bool save_to_path(const char *path);
};

// level flags
#define LVL_DISABLE_LAYER_SWITCH                (1ULL << 0)  // if adventure mode, manual layer switching of robot. if puzzle mode, layer switching of objects
#define LVL_DISABLE_INTERACTIVE                 (1ULL << 1)  // disable interaction with all interactive objects
#define LVL_DISABLE_FALL_DAMAGE                 (1ULL << 2)  // disable fall damage for all robots
#define LVL_DISABLE_CONNECTIONS                 (1ULL << 3)  // puzzle mode, disable connection creating
#define LVL_DISABLE_STATIC_CONNS                (1ULL << 4)  // puzzle mode, disable planks to static objects such as platforms
#define LVL_DISABLE_JUMP                        (1ULL << 5)  // adventure mode, disable manual jumping
#define LVL_DISABLE_ROBOT_HIT_SCORE             (1ULL << 6)  // disable score increase by shooting other robots (TODO: fix this)
#define LVL_DISABLE_ZOOM                        (1ULL << 7)  // disable zoom (TODO: What modes and states should this be enabled in?)
#define LVL_DISABLE_CAM_MOVEMENT                (1ULL << 8)  // disable camera movement (TODO: What modes and states should this be enabled in?)
#define LVL_DISABLE_INITIAL_WAIT                (1ULL << 9)  // disable the initial waiting time when starting a level
#define LVL_UNLIMITED_ENEMY_VISION              (1ULL << 10) // enemies always see the player from any distance and always target the player
#define LVL_ENABLE_INTERACTIVE_DESTRUCTION      (1ULL << 11) // enable shooting interactive objects to destroy them
#define LVL_ABSORB_DEAD_ENEMIES                 (1ULL << 12) // dead robots will be absorbed after a short interval
#define LVL_SNAP                                (1ULL << 13) // snap object by default for puzzle levels
#define LVL_NAIL_CONNS                          (1ULL << 14) // Use nail-shaped connections for planks and beams
#define LVL_DISABLE_CONTINUE_BUTTON             (1ULL << 15) // Disable the continue button
#define LVL_SINGLE_LAYER_EXPLOSIONS             (1ULL << 16) // Explosives reach only one layer
#define LVL_DISABLE_DAMAGE                      (1ULL << 17) // robots cannot take damage
#define LVL_DISABLE_3RD_LAYER                   (1ULL << 18) // disable third layer
#define LVL_PORTRAIT_MODE                       (1ULL << 19) // portrait mode
#define LVL_DISABLE_RC_CAMERA_SNAP              (1ULL << 20) // disable the camera from moving to a newly selected RC
#define LVL_DISABLE_PHYSICS                     (1ULL << 21) // disable physics simulation
#define LVL_DO_NOT_REQUIRE_DRAGFIELD            (1ULL << 22) // disable the need for dragfields to interact with interactive objects.
#define LVL_DISABLE_ROBOT_SPECIAL_ACTION        (1ULL << 23) // disable the robots special action (boxing)
#define LVL_DISABLE_ADVENTURE_MAX_ZOOM          (1ULL << 24) // disable the special adventure zoom
#define LVL_DISABLE_ROAM_LAYER_SWITCH           (1ULL << 25) // disable roam robots ability to layerswitch
#define LVL_CHUNKED_LEVEL_LOADING               (1ULL << 26) // load level in chunks
#define LVL_DISABLE_CAVEVIEW                    (1ULL << 27) // disable caveview
#define LVL_DISABLE_ROCKET_TRIGGER_EXPLOSIVES   (1ULL << 28) // disable the rocket and thrusters ability to trigger explosives
#define LVL_STORE_SCORE_ON_GAME_OVER            (1ULL << 29)
#define LVL_ALLOW_HIGH_SCORE_SUBMISSIONS        (1ULL << 30)
#define LVL_LOWER_SCORE_IS_BETTER               (1ULL << 31)
#define LVL_DISABLE_ENDSCREENS                  (1ULL << 32) // disable any end-game sound or messages
#define LVL_ALLOW_QUICKSAVING                   (1ULL << 33)
#define LVL_ALLOW_RESPAWN_WITHOUT_CHECKPOINT    (1ULL << 34)
#define LVL_DEAD_CREATURE_DESTRUCTION           (1ULL << 35)
#define LVL_AUTOMATICALLY_SUBMIT_SCORE          (1ULL << 36)
#define LVL_ENABLE_LUASOCKET                    (1ULL << 37)

// level format
class lvlinfo
{
  public:
    /// id used locally. Unique for local levels, equal to community_id for downloaded community levels, unique for MAIN levels
    uint32_t local_id;

    /// id of the current save
    uint32_t save_id;

    uint8_t  version;
    uint8_t  type;
    uint32_t community_id;
    uint32_t autosave_id;
    uint32_t revision;
    uint32_t parent_id;
    uint8_t  name_len;
    uint16_t descr_len;
    uint64_t flags;
    uint8_t  visibility;
    uint32_t parent_revision;
    bool     pause_on_finish;
    bool     show_score;
    uint8_t  bg;
    uint32_t bg_color;
    uint16_t size_x[2]; // board size
    uint16_t size_y[2];
    uint8_t  velocity_iterations, position_iterations;
    uint32_t final_score;
    float    sandbox_cam_x;
    float    sandbox_cam_y;
    float    sandbox_cam_zoom;
    float    gravity_x;
    float    gravity_y;

    float min_x;
    float max_x;
    float min_y;
    float max_y;

    // level version 26 new stuff
    float prismatic_tolerance;
    float pivot_tolerance;

    // level version 28 (1.5)
    uint64_t seed;
    float    linear_damping;
    float    angular_damping;
    float    joint_friction;
    float    dead_enemy_absorb_time;
    float    time_before_player_can_respawn;
    uint64_t compression_length;
  private:
    uint32_t adventure_id;
  public:
    inline uint32_t get_adventure_id() {
        return this->adventure_id;
    }

    inline void set_adventure_id(uint32_t id) {
        this->adventure_id = id;
    }

    /// make sure things added in later version are not used by old levels
    void sanity_check();

    /// NOT null terminated
    char     name[256];
    uint8_t  icon[128*128];
    /// null terminated, can be null
    char    *descr;

    uint32_t num_groups;
    uint32_t num_entities;
    uint32_t num_connections;
    uint32_t num_cables;

    // level version 28
    uint32_t num_chunks;
    uint32_t state_size;
    /// number of occupied but pending gentypes
    uint32_t num_gentypes;

    /// Create a new level
    void create(int type, uint64_t seed = 0, uint32_t version = 0);

    bool read(lvlbuf *lb, bool skip_description);
    bool read(lvlbuf *lb) {
        return this->read(lb, false);
    }

    void write(lvlbuf *lb);

    /// get size of level header
    int get_size() const;

    inline bool flag_active(uint64_t flag) const {
        return this->flags & flag;
    }

    lvlinfo() {
        descr = 0;
        descr_len = 0;
        name_len = 0;
        version = 0;
        type = LCAT_CUSTOM;
        save_id = 0;
        local_id = 0;
        memset(this->name, 0, sizeof(this->name));
    }

    ~lvlinfo() {
        if(this->descr) free(this->descr);
    }

    void print() const;
};

class lvlbuf {
  public:
    uint64_t size;
    uint64_t cap;
    uint64_t min_cap;
    uint8_t *buf;
    int rp;
    bool sparse_resize;

    lvlbuf() {
        size = 0;
        cap = 0;
        buf = 0;
        rp = 0;
        min_cap = 20480;
        sparse_resize = false;
    }

    void ensure(uint64_t s);

    inline void reset() {
        this->clear();
    }

    inline void clear() {
        this->rp = 0;
        this->size = 0;
        if (this->cap > this->min_cap*2) {
            this->cap = this->min_cap;
            this->buf = static_cast<uint8_t*>(realloc(this->buf, this->cap));
        }
    }

    inline bool eof() {
        return this->rp >= this->size;
    }

    inline bool eof(int s) {
        return this->rp + s > this->size;
    }

    inline bool r_bool() {
        return (this->r_uint8() != 0);
    }

    // Read routines

    /// Read unsigned 8-bit integer
    inline uint8_t r_uint8() {
        if (eof(1))
            return 0;

        uint8_t r = *(buf+rp);
        rp += sizeof(uint8_t);
        return r;
    }

    /// Read unsigned 16-bit integer
    inline uint16_t r_uint16() {
        if (eof(sizeof(uint16_t)))
            return 0;

        uint16_t r;
        if (rp & 1) {
            r = 0;
            r |= (*(uint8_t*)(buf+rp));
            r |= (*(uint8_t*)(buf+rp+1)) << 8;
        } else r = *(uint16_t*)(buf+rp);
        rp+=sizeof(uint16_t);
        return r;
    }

    /// Read unsigned 32-bit integer
    inline uint32_t r_uint32() {
        if (eof(sizeof(uint32_t)))
            return 0;

        uint32_t r;

        if ((rp) & 3) {
            r = 0;

            // the address is misaligned, alternative fetch
            r |= (*(uint8_t*)(buf+rp));
            r |= (*(uint8_t*)(buf+rp+1)) << 8;
            r |= (*(uint8_t*)(buf+rp+2)) << 16;
            r |= (*(uint8_t*)(buf+rp+3)) << 24;
        } else
            r = *(uint32_t*)(buf+rp);

        rp += sizeof(uint32_t);
        return r;
    }

    /// Read signed 32-bit integer
    inline int32_t r_int32() {
        return static_cast<int32_t>(r_uint32());
    }

    /// Read unsigned 64-bit integer
    inline uint64_t r_uint64() {
        if (eof(sizeof(uint64_t)))
            return 0;

        uint64_t v1 = this->r_uint32();
        uint64_t v2 = this->r_uint32();

        return v1 | (v2 << 32);
    }

    /// Read signed 64-bit integer
    inline int64_t r_int64() {
        return static_cast<int64_t>(r_uint64());
    }

    /// Read 32-bit float
    inline float r_float() {
        if (eof(sizeof(float)))
            return 0;

        float r;
        if ((rp) & 3) {
            // the address is misaligned, we need to do two fetches
            uint32_t r2 = 0;
            r2 |= (*(uint8_t*)(buf+rp));
            r2 |= (*(uint8_t*)(buf+rp+1)) << 8;
            r2 |= (*(uint8_t*)(buf+rp+2)) << 16;
            r2 |= (*(uint8_t*)(buf+rp+3)) << 24;

            memcpy(&r, &r2, sizeof(uint32_t));
        } else
            r = *(float*)(buf+rp);

        rp += sizeof(float);
        return r;
    }

    /// Read raw buffer
    inline void r_buf(char *out, uint32_t len) {
        if (eof(len))
            memset(out, 0, len);
        else
            memcpy(out, buf + rp, len);

        rp += len;
    }

    /// Read ID (32-bit int)
    inline uint32_t r_id() {
        return r_uint32();
    }

    // Write routines

    /// Write unsigned 8-bit integer
    inline void w_uint8(uint8_t i) {
        *(uint8_t*)(buf + size) = i;
        size += sizeof(uint8_t);
    }

    /// Write signed 8-bit integer (automatically expand capacity if needed)
    inline void w_s_uint8(uint8_t i) {
        ensure(sizeof(uint8_t));
        w_uint8(i);
    }

    /// Write unsigned 16-bit integer
    inline void w_uint16(uint16_t i) {
        *(uint8_t*)(buf+size) = (uint8_t)(i & 0xff);
        *(uint8_t*)(buf+size+1) = (uint8_t)((i & 0xff00) >> 8);
        size += 2;
    }

    /// Write unsigned 16-bit integer (automatically expand capacity if needed)
    inline void w_s_uint16(uint16_t i) {
        ensure(sizeof(uint16_t));
        w_uint16(i);
    }

    /// Write unsigned 32-bit integer
    inline void w_uint32(uint32_t i) {
        *(uint8_t*)(buf+size) = (uint8_t)(i & 0xff);
        *(uint8_t*)(buf+size+1) = (uint8_t)((i & 0xff00) >> 8);
        *(uint8_t*)(buf+size+2) = (uint8_t)((i & 0xff0000) >> 16);
        *(uint8_t*)(buf+size+3) = (uint8_t)((i & 0xff000000) >> 24);
        size += 4;
    }

    /// Write unsigned 32-bit integer (automatically expand capacity if needed)
    inline void w_s_uint32(uint32_t i) {
        ensure(sizeof(uint32_t));
        w_uint32(i);
    }

    /// Write signed 32-bit integer
    inline void w_int32(int32_t i) {
        w_uint32(static_cast<uint32_t>(i));
    }

    /// Write signed 32-bit integer (automatically expand capacity if needed)
    inline void w_s_int32(int32_t i) {
        w_s_uint32(static_cast<uint32_t>(i));
    }

    /// Write unsigned 64-bit integer
    inline void w_uint64(uint64_t i) {
        uint32_t ln = i & 0xffffffff;
        uint32_t mn = i >> 32;
        this->w_uint32(ln);
        this->w_uint32(mn);
    }

    /// Write unsigned 64-bit integer (automatically expand capacity if needed)
    inline void w_s_uint64(uint64_t i) {
        ensure(sizeof(uint64_t));
        w_uint64(i);
    }

    /// Write signed 64-bit integer
    inline void w_int64(int64_t i) {
        w_uint64(static_cast<uint64_t>(i));
    }

    /// Write signed 64-bit integer (automatically expand capacity if needed)
    inline void w_s_int64(int64_t i) {
        w_s_uint64(static_cast<uint64_t>(i));
    }

    /// Write ID (32-bit int)
    inline void w_id(uint32_t id){
        w_uint32(id);
    }

    /// Write ID (32-bit int, automatically expand capacity if needed)
    inline void w_s_id(uint32_t id) {
        w_s_uint32(id);
    }

    /// Write 32-bit float
    inline void w_float(float i) {
        union {float vf; uint32_t vi;} u;
        u.vf = i;
        *(uint8_t*)(buf+size) = (uint8_t)(u.vi & 0xff);
        *(uint8_t*)(buf+size+1) = (uint8_t)((u.vi & 0xff00) >> 8);
        *(uint8_t*)(buf+size+2) = (uint8_t)((u.vi & 0xff0000) >> 16);
        *(uint8_t*)(buf+size+3) = (uint8_t)((u.vi & 0xff000000) >> 24);
        size += 4;
    }

    /// Write 32-bit float (automatically expand capacity if needed)
    inline void w_s_float(float i) {
        ensure(sizeof(float));
        w_float(i);
    }

    /// Write boolean
    inline void w_bool(bool v) {
        w_uint8(v ? 1 : 0);
    }

    /// Write boolean (automatically expand capacity if needed)
    inline void w_s_bool(bool v) {
        w_s_uint8(v ? 1 : 0);
    }

    /// Write raw buffer
    inline void w_buf(const char* s, uint32_t len) {
        memcpy(this->buf + this->size, s, len);
        size += len;
    }

    /// Write raw buffer (automatically expand capacity if needed)
    inline void w_s_buf(const char* s, uint32_t len) {
        ensure(len);
        w_buf(s, len);
    }

    /// Compress buffer with zlib
    void zcompress(const lvlinfo &level, unsigned char **dest, uint64_t *dest_len) const;

    /// Decompress zlib compressed buffer
    void zuncompress(const lvlinfo &level);
};

/**
 * Class used for reading and editing level properties without
 * modifying level contents
 */
class lvledit {
  private:
    int header_size;

    int lvl_type;

  public:
    lvlbuf  lb;
    lvlinfo lvl;
    uint32_t lvl_id;

    lvledit() {
        this->lvl_type = 0;
        this->lvl_id = 0;
        this->lb.cap = 20480;
        this->lb.size = 0;
        this->lb.buf = (unsigned char*)malloc(20480);
    }

    ~lvledit() {
        if (this->lb.buf)
            free(this->lb.buf);
    }

    bool open(int lvl_type, uint32_t lvl_id);
    bool save();

    /**
     * The open_from_path and save_to_path methods are mainly used for the lvledit tool,
     * which means it doesn't check any of the vitals (i.e. if the
     * level is stored in the apk or outside.)
     */
    bool open_from_path(const char *path);
    bool save_to_path(const char *path);

    void print_gids();
};

#endif
