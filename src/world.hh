#pragma once

#include <Box2D/Box2D.h>
#include "entity.hh"
#include "edevice.hh"
#include "debugdraw.hh"
#include "chunk.hh"
#include "pkgman.hh"
#include <set>
#include <map>
#include <vector>
#include <string>
#include <deque>

enum {
    WORLD_EVENT_PLAYER_DIE,
    WORLD_EVENT_ENEMY_DIE,
    WORLD_EVENT_INTERACTIVE_DESTROY,
    WORLD_EVENT_PLAYER_RESPAWN,
    WORLD_EVENT_CLICK_DOWN,
    WORLD_EVENT_CLICK_UP,
    WORLD_EVENT_ABSORB,
    WORLD_EVENT_LEVEL_COMPLETED,
    WORLD_EVENT_GAME_OVER,

    WORLD_EVENT__NUM
};

enum {
    SAVE_TYPE_DEFAULT  = 0,
    SAVE_TYPE_AUTOSAVE = 1,
    SAVE_TYPE_STATE    = 2,
};

#define WORLD_STEP 8000
#define WORLD_STEP_SPEEDUP 3000
#define WORLD_DT ((float)(WORLD_STEP+WORLD_STEP_SPEEDUP) / 1000000.f)

#define EMIT_DELAY_COLLIDE_EMITTER (5*10)

class activator;
class game;
class screenshot_marker;
class receiver_base;
class solver_ingame;
class solver;
class eventlistener;
class key_listener;
class localgravity;
class escript;
class soundman;

enum {
    ACTION_FINALIZE_GROUP,
    ACTION_REBUILD_GROUP,
    ACTION_MOVE_ENTITY,
    ACTION_SET_ANIMAL_TYPE,
    ACTION_CALL_ON_LOAD,
};

struct entity_action
{
    uint32_t entity_id;
    uint32_t action_id;
    void *data;
};


struct pending_absorb
{
    entity *e;
    entity *absorber;
    b2Vec2 absorber_point;
    uint8_t absorber_frame;

    pending_absorb(entity *e)
    {
        this->e = e;
        this->absorber = 0;
    }

    pending_absorb(entity *e, entity *absorber, b2Vec2 absorber_point, uint8_t absorber_frame)
    {
        this->e = e;
        this->absorber = absorber;
        this->absorber_point = absorber_point;
        this->absorber_frame = absorber_frame;
    }
};

static inline bool operator <(const pending_absorb& lhs, const pending_absorb &rhs)
{
    return lhs.e < rhs.e;
}

class pending_emit
{
  public:
    bool partial;

    union {
        struct {
            entity *e;
            entity *emitter;
            float velocity_x;
            float velocity_y;
        } single;

        struct {
            float displacement_x;
            float displacement_y;
            const char *buf;
            uint16_t    buf_len;
        } multi;
    } data;

    pending_emit(entity *e, entity *emitter, b2Vec2 velocity)
    {
        this->partial = false;
        this->data.single.e = e;
        this->data.single.emitter = emitter;
        this->data.single.velocity_x = velocity.x;
        this->data.single.velocity_y = velocity.y;
    }

    pending_emit(const char *buf, uint16_t buf_len, b2Vec2 displacement)
    {
        this->partial = true;
        this->data.multi.displacement_x = displacement.x;
        this->data.multi.displacement_y = displacement.y;
        this->data.multi.buf = buf;
        this->data.multi.buf_len = buf_len;
    }
};

class world : public b2QueryCallback
{
  private:
    class b2_destruction_listener : public b2DestructionListener
    {
        void SayGoodbye(b2Joint *j);
        void SayGoodbye(b2Fixture *f);
    } *destruction_listener;

    class b2_sleep_listener : public b2SleepListener
    {
      public:
        void OnWakeup(b2Body *b);
        void OnSleep(b2Body *b);
    } *sleep_listener;

    void init(bool paused);
    void optimize_connections();
    void apply_puzzle_constraints();

  public:
    debugdraw *debug;
    bool       paused;
    bool       locked;
    uint64_t   electronics_accum;

    int events[WORLD_EVENT__NUM];

    solver_ingame              *contacts_playing;
    solver                     *contacts_paused;

    /* paused state */
    std::set<entity*>           tickable;

    /* playing state */
    std::set<entity*>           stepable;
    std::set<entity*>           prestepable;
    std::set<entity*>           mstepable;
    std::vector<edevice*>       electronics;
    std::set<activator*>        activators;

    /* special types of entities that we need to keep track of */
    std::set<eventlistener*>    eventlisteners;
    std::set<key_listener*>     key_listeners;
    std::set<localgravity*>     localgravities;
    std::set<escript*>          escripts;
    std::set<entity*>           repair_stations;

    std::map<uint32_t, entity*> all_entities; /* all entities except groups and cables */
    std::map<uint32_t, group*>  groups;
    std::set<cable*>            cables;
    std::set<connection *>      connections;

    /* stuff marked for action */
    std::vector<pending_emit> to_be_emitted;
    std::vector<pending_emit> post_to_be_emitted;
    std::set<entity*>         post_interact;
    std::set<pending_absorb>  to_be_absorbed;
    std::set<b2Joint*>        to_be_destroyed;
    std::set<level_chunk*>    to_be_reloaded;

    std::map<uint32_t, int64_t> timed_absorb;
    std::map<b2Joint*, float> destructable_joints;

    lvlbuf lb;
    size_t state_ptr;

    /* temp query stuff */
    int query_layer;
    bool query_exact;
    entity *query_nearest;
    b2Body *query_nearest_b;
    b2Fixture *query_nearest_fx;
    b2Vec2 query_point;
    b2Vec2 query_offs;
    float query_dist;
    uint8_t query_frame;
    bool query_force;

    void init_level(bool soft=false);
    void init_level_entities(std::map<uint32_t, entity*> *entities=0, std::map<uint32_t, group*> *groups=0);

    float gravity_x;
    float gravity_y;
    b2Vec2 last_gravity;

    /*       key  force */
    std::map<int, b2Vec2> gravity_forces;

  public:
    bool first_solve;
    uint32_t step_count;
    uint32_t edevice_order;
    lvlinfo level;
    int     level_id_type; /* id types (i.e. LEVEL_DB, LEVEL_LOCAL) */

    std::deque<struct entity_action> actions;

    chunk_window    *cwindow;

    b2Body *ground;
    b2Fixture *ground_fx[4];

    std::vector<entity*> poststep;
    b2World *b2;
    world();
    bool step();
    void add(entity *e);
    bool remove(entity *e);
    void insert(entity *e);
    void erase(entity *e);
    void draw_debug(tms::camera *cam);
    void init_simulation(void);
    void reload_modified_chunks();

    void add_action(uint32_t entity_id, uint32_t action_id, void *data=0);
    void perform_actions(void);

    inline bool is_paused(){return this->paused;}
    inline bool is_playing(){return !this->is_paused();}
    inline bool is_puzzle(){return this->level.type == LCAT_PUZZLE;}
    inline bool is_adventure(){return this->level.type == LCAT_ADVENTURE;}
    inline bool is_custom(){return this->level.type == LCAT_CUSTOM;}

    inline bool is_buildable()
    {
        return this->is_paused() || this->is_adventure();
    }

    void destroy_connection_joint(connection *c);

    entity *get_entity_by_id(uint32_t id);
    bool has_num_entities_with_gid(uint32_t gid, int count);

    void emit_all();
    void absorb_all();
    void destroy_joints();

    std::map<uint32_t, screenshot_marker*> cam_markers;
    std::map<std::string, double> level_variables;

    void add_gravity_force(int key, b2Vec2 force);
    void remove_gravity_force(int key);

    /*            frequency    receiver */
    std::multimap<uint32_t,    receiver_base*> receivers;
    void add_receiver(uint32_t frequency, receiver_base *t);
    void remove_receiver(uint32_t frequency, receiver_base *t);

    /*            sound_id    soundman */
    std::multimap<uint32_t,   soundman*> soundmanagers;
    void add_soundman(uint32_t sound_id, soundman *sm);
    void remove_soundman(uint32_t sound_id, soundman *sm);

    void insert_connection(connection *cc);
    void erase_connection(connection *cc);

    void create(int type, uint64_t seed, bool play);
    void open_autosave(void);
    bool open(int id_type, uint32_t id, bool paused, bool sandbox, uint32_t save_id=0);
    void begin();
    bool read_cache(int level_type, uint32_t id, uint32_t save_id=0);
    bool save_cache(int level_type, uint32_t id, uint32_t save_id=0);
    bool save(int save_type=SAVE_TYPE_DEFAULT);
    void calculate_bounds(std::set<entity*> *entities, float *min_x, float *max_x, float *min_y, float *max_y);
    bool load_partial_from_buffer(lvlbuf *lb, b2Vec2 position, std::map<uint32_t, entity*> *entities, std::map<uint32_t, group*> *groups, std::set<connection*> *connections, std::set<cable*> *cables);
    bool load_partial(uint32_t id, b2Vec2 position, std::map<uint32_t, entity*> *entities, std::map<uint32_t, group*> *groups, std::set<connection*> *connections, std::set<cable*> *cables);
    void save_partial(std::set<entity*> *entity_list, const char *name, uint32_t id);
    void fill_buffer(lvlinfo *lvl, lvlbuf *buf, std::map<uint32_t, group*> *groups, std::map<uint32_t, entity*> *entities, std::set<connection*> *connections, std::set<cable*> *cables, uint32_t id_modifier=0, b2Vec2 displacement=b2Vec2(0.f,0.f), bool fill_unloaded=false, bool fill_states=false);
    bool load_buffer(lvlinfo *lvl, lvlbuf *buf, uint32_t id_modifier=0, b2Vec2 displacement=b2Vec2(0.f,0.f), std::map<uint32_t, entity*> *entities=0, std::map<uint32_t, group*> *groups =0, std::set<connection*> *connections =0, std::set<cable*> *cables=0);

    bool write_level(const char *filename, lvlbuf *lb);

    entity *load_entity(lvlbuf *buf, int version, uint32_t id_modifier=0, b2Vec2 displacement=b2Vec2(0.f,0.f), std::map<uint32_t, entity*> *entities=0, std::vector<chunk_pos> *affected_chunks=0);
    group  *load_group(lvlbuf *buf, int version, uint32_t id_modifier=0, b2Vec2 displacement=b2Vec2(0.f,0.f), std::map<uint32_t, group*> *groups=0);
    cable  *load_cable(lvlbuf *buf, int version, uint64_t flags, uint32_t id_modifier=0, b2Vec2 displacement=b2Vec2(0.f,0.f), std::set<cable*> *cables=0);
    connection *load_connection(lvlbuf *buf, int version, uint64_t flags, uint32_t id_modifier=0, b2Vec2 displacement=b2Vec2(0.f,0.f), std::set<connection*> *connections=0);

    void reset(void);
    void set_level_type(int type);

    bool ReportFixture(b2Fixture *f);
    int query(tms::camera *cam, int x, int y, entity **out_ent, b2Body **out_body, tvec2 *offs, uint8_t *frame, int layer_mask, bool force_selection=false, b2Fixture **out_fx=0, bool is_exact=false);
    int get_layer_point(tms::camera *cam, int x, int y, float layer, tvec3 *out);
    void solve_electronics(void);
    void apply_local_gravities();
    int solve_edevice(edevice *e);

    /* helper function for raycasting */
    void raycast(b2RayCastCallback *callback,
            const b2Vec2 &point1, const b2Vec2 &point2,
            float r=.3f, float g=.9f, float b=.3f,
            int64_t life=2500);

    /* helper function for querying */
    void query_aabb(b2QueryCallback *callback,
            const b2AABB &aabb,
            float r=.3f, float g=.3f, float b=.6f,
            int64_t life=2500);

    void explode(entity *source, b2Vec2 pos, int layer,
               int num_rays, float force,
               float damage_multiplier, float dist_multiplier);

    /* chunk stuff */
    float get_height(float x);

    static inline b2Filter get_filter_for_layer(int z)
    {
        b2Filter r;
        r.categoryBits = (15 << z*4);
        r.maskBits = (15 << z*4);
        r.groupIndex = 0;
        return r;
    }

    static inline b2Filter get_filter_for_layer(int z, int mask)
    {
        b2Filter r;
        r.categoryBits = (mask << z*4);
        r.maskBits = (mask << z*4);
        r.groupIndex = 0;
        return r;
    }

    static inline b2Filter get_filter_for_multilayer(int mask_0, int mask_1, int mask_2)
    {
        b2Filter r;
        r.categoryBits = (mask_0) | (mask_1 << 4) | (mask_2 << 8);
        r.maskBits = (mask_0) | (mask_1 << 4) | (mask_2 << 8);
        r.groupIndex = 0;
        return r;
    }

    static inline int fixture_get_layer(b2Fixture *f)
    {
        return (f->GetFilterData().categoryBits & (15 << 8)) ? 2 :
                ((f->GetFilterData().categoryBits & (15<<4)) ? 1 : 0);
    }

    static inline int fixture_get_lower_layer(b2Fixture *f)
    {
        return (f->GetFilterData().categoryBits & (15)) ? 0 :
                ((f->GetFilterData().categoryBits & (15<<4)) ? 1 : 2);
    }

    static inline bool fixture_in_layer(b2Fixture *f, int layer, int sublayer=15)
    {
        return (f->GetFilterData().categoryBits & (sublayer << (layer*4)));
    }

    inline b2Vec2 get_gravity()
    {
        return this->b2->GetGravity();
    }

    inline void set_gravity(float x, float y)
    {
        if (x == this->gravity_x && y == this->gravity_y)
            return;

        this->gravity_x = x;
        this->gravity_y = y;
    }

    std::map<uint32_t, entity*> get_all_entities()
    {
        return this->all_entities;
    }

    int score_helper; /* ;) */

    friend class game;
    friend class entity;
};

extern world *W;
