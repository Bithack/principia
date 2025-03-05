#pragma once

#include <tms/bindings/cpp/cpp.hh>
#include <Box2D/Box2D.h>

#include "pkgman.hh"
#include "material.hh"
#include "types.hh"
#include "const.hh"
#include "object_factory.hh"

#include <set>
#include <map>

class activator;
class game;
class world;
class base_prompt;

struct worth
{
    uint32_t resources[NUM_RESOURCES];
    float oil;

    worth();

    /* Add num resources */
    struct worth& add(uint8_t resource_type, uint32_t num);

    /* Add oil */
    struct worth& add_oil(float val);

    /* Add resources and oil from another worth object */
    struct worth& add(const struct worth& worth);
};

#define ENTITY_MAX_HEALTH 15.f

#define ENTITY_NOT_SEARCHED         (1U << 0)
#define ENTITY_PROT_PLATFORM        (1U << 1)
#define ENTITY_PROT_AUTOPROTECTOR   (1U << 2)

#define ENTITY_MAX_SLIDERS 2

// XXX: duplicate from game.hh
#define EVENT_CONT                 0
#define EVENT_DONE                 1
#define EVENT_SPECIAL              2

#define ENTITY_DEFAULT     0
#define ENTITY_ROBOT       1
#define ENTITY_PLUG        2
#define ENTITY_EDEVICE     3
#define ENTITY_PLANK       4
#define ENTITY_GEAR        5
#define ENTITY_BREADBOARD  6
#define ENTITY_WHEEL       7
#define ENTITY_GROUP       8
#define ENTITY_CABLE       9
#define ENTITY_RACK        10
#define ENTITY_PIPELINE    11
#define ENTITY_BALL        12

#define ENTITY_UPDATE_GHOST    0
#define ENTITY_UPDATE_FASTBODY 1
#define ENTITY_UPDATE_GROUPED  2
#define ENTITY_UPDATE_JOINT_PIVOT  3
#define ENTITY_UPDATE_JOINT 4
//#define ENTITY_UPDATE_BR 5
#define ENTITY_UPDATE_NULL 6
#define ENTITY_UPDATE_STATIC 7
#define ENTITY_UPDATE_STATIC_CUSTOM 8

#define ENTITY_UPDATE_CUSTOM  -1

#define ENTITY_DENSITY_SCALE_MIN .005f
#define ENTITY_DENSITY_SCALE_MAX 2.f

#define CULL_EFFECTS_DEFAULT     0
#define CULL_EFFECTS_BY_POSITION 1
#define CULL_EFFECTS_DISABLE     2

#define POLYGON_MAX_CORNERS 6

#define P_INT    0
#define P_FLT    1
#define P_STR    2
#define P_INT8   3
#define P_ID     4

#define CONN_CUSTOM 0
#define CONN_WELD   1
#define CONN_PIVOT  2
#define CONN_GEAR   3
#define CONN_RACK   4 /* gear-rack connection */
#define CONN_GROUP  5
#define CONN_PLATE  6 /* identical too weld joint but with another mesh */
#define CONN_BR     7 /* dummy pending connection for breadboard attachments */

#define CONN_RENDER_DEFAULT 0
#define CONN_RENDER_SMALL   1
#define CONN_RENDER_NAIL    2
#define CONN_RENDER_HIDE    3

#define CONN_DAMPING_MAX 10.f

#define CONN_MAX_FORCE 5000.f

/* entity flags */
#define ENTITY_IS_BETA                  (1ULL << 0)
#define ENTITY_IS_LOW_PRIO              (1ULL << 1)  // low prio when selecting objects
#define ENTITY_IS_HIGH_PRIO             (1ULL << 2)  // high prio when selecting objects
#define ENTITY_ALLOW_CONNECTIONS        (1ULL << 3)  // allow other entities connecting to this entity
#define ENTITY_IS_STATIC                (1ULL << 4)
#define ENTITY_IS_ABSORBED              (1ULL << 5)  // entities with this flag should no longer be handled by anything!
#define ENTITY_IS_ROBOT                 (1ULL << 6)
#define ENTITY_IS_COMPOSABLE            (1ULL << 7)
#define ENTITY_IS_EDEVICE               (1ULL << 8)
#define ENTITY_IS_BRDEVICE              (1ULL << 9)
#define ENTITY_IS_CONTROL_PANEL         (1ULL << 10)
#define ENTITY_DISABLE_UNLOADING        (1ULL << 11)
#define ENTITY_IS_PROMPT                (1ULL << 12)
#define ENTITY_IS_MAGNETIC              (1ULL << 13) // specified whether the entity is affected by magnetic powers
#define ENTITY_IS_MOVEABLE              (1ULL << 14)
#define ENTITY_ALLOW_ROTATION           (1ULL << 15)
#define ENTITY_DISABLE_LAYERS           (1ULL << 16) // disable manually setting this entities layer
#define ENTITY_HAS_CONFIG               (1ULL << 17)
#define ENTITY_HAS_INGAME_CONFIG        (1ULL << 18)
#define ENTITY_IS_INTERACTIVE           (1ULL << 19)
#define ENTITY_HAS_TRACKER              (1ULL << 20)
#define ENTITY_DO_STEP                  (1ULL << 21)
#define ENTITY_DO_MSTEP                 (1ULL << 22)
#define ENTITY_DO_UPDATE_EFFECTS        (1ULL << 23)
#define ENTITY_DO_UPDATE                (1ULL << 24) // on by default. this isn't even checked though
#define ENTITY_DO_TICK                  (1ULL << 25)
#define ENTITY_FADE_ON_ABSORB           (1ULL << 26)
#define ENTITY_ALLOW_AXIS_ROT           (1ULL << 27) // display axis rotation button
#define ENTITY_IS_OWNED                 (1ULL << 28)
#define ENTITY_MUST_BE_DYNAMIC          (1ULL << 29)
#define ENTITY_TRIGGER_EXPLOSIVES       (1ULL << 30)
#define ENTITY_CONNECTED_TO_BREADBOARD  (1ULL << 31)
#define ENTITY_AXIS_ROT                 (1ULL << 32)
#define ENTITY_CAN_MOVE                 (1ULL << 33)
#define ENTITY_CUSTOM_GHOST_UPDATE      (1ULL << 34)
#define ENTITY_IS_BULLET                (1ULL << 35)
#define ENTITY_UNUSED_FLAG_2            (1ULL << 36)
#define ENTITY_IS_DEBRIS                (1ULL << 37)
#define ENTITY_IS_BEAM                  (1ULL << 38)
#define ENTITY_IS_CRANE_PULLEY          (1ULL << 39)
#define ENTITY_IS_PLUG                  (1ULL << 40)
#define ENTITY_IS_CREATURE              (1ULL << 41)
#define ENTITY_IS_EXPLOSIVE             (1ULL << 42)
#define ENTITY_WAS_HIDDEN               (1ULL << 43)
#define ENTITY_DYNAMIC_UNLOADING        (1ULL << 44) // if this entity can be unloaded from a level not sleeping
#define ENTITY_IS_LOCKED                (1ULL << 45)
#define ENTITY_IS_RESIZABLE             (1ULL << 46)
#define ENTITY_DO_PRE_STEP              (1ULL << 47)
#define ENTITY_IS_PLASTIC               (1ULL << 48)
#define ENTITY_HAS_ACTIVATOR            (1ULL << 49)
#define ENTITY_STATE_SLEEPING           (1ULL << 50)
#define ENTITY_CAN_BE_GRABBED           (1ULL << 51) // can be grabbed using Builder
#define ENTITY_CAN_BE_COMPRESSED        (1ULL << 52) // can be compressed using Compressor
#define ENTITY_IS_ZAPPABLE              (1ULL << 53) // can be zapped using the Zapper
#define ENTITY_IS_DEV                   (1ULL << 54) // object is a development/unused object (displays with a DEV label)

#define MATERIAL_PLASTIC 0

#define ENTITY_MAX_CHUNK_INTERSECTIONS 8

class entity;
class composable;
class group;
class connection_entity;
class material;
class edevice;
class connection;

#define JOINT_TYPE_UNKNOWN     -1
#define JOINT_TYPE_CONN         0
#define JOINT_TYPE_CABLE        1
#define JOINT_TYPE_MOVER        2
#define JOINT_TYPE_BACKPACK     3
#define JOINT_TYPE_SCUP         4
#define JOINT_TYPE_RAGDOLL      5

class joint_info {
  public:
    int type;
    void *data;
    bool should_destroy;
    joint_info(int type, void *data)
    {
        this->type = type;
        this->data = data;
        this->should_destroy = true;
    }
    joint_info(){this->should_destroy=true;};

    void destroy() const { if (this->should_destroy) delete this; }

  protected:
    ~joint_info() {}
};

class connection
{
  private:
    void remove_self_ent();
    void create_self_ent(bool add);

  public:
    joint_info *ji;

    uint8_t type;
    entity *self_ent;
    b2Joint *j;

    /* if the connection has an owner, e is the owner */
    entity *e;
    entity *o;

    bool fixed; /* can't be removed */
    bool tolerant; /* if this joint allows slight displacement */
    bool destroyed;

    /* layer of the connection, if the connection spans multiple layers,
     * this is the lowest layer of the two */
    int layer;

    /* sublayer mask of the connection, if the connection span multiple sublayers,
     * this is the lowest sublayer of the two */
    int layer_mask;

    int sublayer_dist;

    /* if this connection is active, the point is in e's local
     * coordinate frame, otherwise it is a world point */
    b2Vec2 p, p_s; /* p_s is the secondary local anchor if weld or pivot joint */

    /* Coordinate frames for 'e' and 'o' */
    uint8_t f[2];

    bool owned;
    uint8_t o_index;
    bool pending;
    bool multilayer; /* true if the connection spans two layers,
                                the rendered nail will not be rotated */

    float relative_angle; /* relative angle of the entities level version 22 */

    float angle;
    uint8_t render_type;
    int     typeselect;
    uint8_t option;

    float max_force;
    float damping; /* level VERSION 8, damping for rotary joints */

    uint32_t e_data; /* level version 28 (1.5) information about e */
    uint32_t o_data; /* level version 28 (1.5) information about o */

    /* we keep track of where this conn was last written or read */
    size_t write_ptr;
    size_t write_size;

    connection *next[2];

    connection()
    {
        this->write_ptr = 0;
        this->write_size = 0;
        this->reset();
    }

    b2Vec2 get_position();

    void update_relative_angle(bool force);

    inline void init_owned(int index, entity *e)
    {
        this->reset();
        this->e = e;
        this->o_index = index;
    }

    inline entity *get_other(entity *e)
    {
        if (e == this->e) {
            return this->o;
        } else {
            return this->e;
        }
    }

    inline connection *get_next(entity *e)
    {
        if (e == this->e) {
            return next[0];
        } else {
            return next[1];
        }
    }

    inline void reset(void)
    {
        this->ji = 0;
        this->layer_mask = 0;
        this->sublayer_dist = 0;
        f[0] = 0;
        f[1] = 0;
        self_ent = 0;
        type = CONN_CUSTOM;
        render_type = 0;
        next[0] = 0;
        next[1] = 0;
        e_data = 0;
        o_data = 0;
        j = 0;
        e = 0;
        o = 0;
        layer = 0;
        owned = true;
        o_index = 0;
        pending = true;
        multilayer = false;
        typeselect = 0;
        tolerant = false;
        fixed = false;
        angle = 0;
        max_force = INFINITY;
        damping = 0.f;
        option = 0;
        relative_angle = 0.f;
        destroyed = false;
    }

    void update(void);
    void apply(void);
    void create_joint(bool add);
    void destroy_joint(void);

    void update_render_type();

    connection *clone()
    {
        return new connection(*this);
    }
};

class property
{
  public:
    uint8_t type;
    union {
        float f;
        uint32_t i;
        uint8_t i8;
        struct {
            char *buf;
            uint32_t len;
        } s;
    } v;

    property()
    {
        clear();
    }
    void clear()
    {
        type = 0;
        memset(&this->v, 0, sizeof(this->v));
    }

    /* The receiving function is responsible for freeing the return value */
    char* stringify();

    void parse(char *buf);
};

struct entity_listener {
    entity *self;
    void *userdata;
    void (*cb)(entity* self,void* userdata);
};

/**
 *
 * entity pointer is stored in UserData of fixtures
 * uint8 frame ID is stored in the body's UserData
 **/
class entity : public tms::entity
{
  protected:
    void easy_update();

    /* shape creation functions, should be called from add_to_world */
    void create_rect(b2BodyType type, float width, float height, m *m){create_rect(type,width,height,m,NULL);};
    void create_rect(b2BodyType type, float width, float height, m *m, b2Fixture **fixture_out=NULL);
    void create_circle(b2BodyType type, float radius, m *mat);

    void create_rect(b2BodyType type, float width, float height, struct tms_material *mat){create_rect(type,width,height,static_cast<m*>(mat),NULL);};
    void create_circle(b2BodyType type, float radius, struct tms_material* mat){ create_circle(type,radius,static_cast<m*>(mat)); };

    float m_scale;
    float width; /* approximate width of the object, used for the menu rendering
                    and for positioning the rotation anchor */

  public:
    inline void fastbody_update()
    {
        const b2Transform &t = this->body->GetTransform();

        //tmat4_load_identity(this->M);
        this->M[0] = t.q.c;
        this->M[1] = t.q.s;
        this->M[4] = -t.q.s;
        this->M[5] = t.q.c;
        this->M[12] = t.p.x;
        this->M[13] = t.p.y;
        this->M[14] = this->prio * LAYER_DEPTH;

        tmat3_copy_mat4_sub3x3(this->N, this->M);

        tmat4_scale(this->M, this->get_scale(), this->get_scale(), 1.f);
    }

    void subscribe(entity *target, int event, void (*listener_func)(entity *self, void *userdata), void *userdata=0);
    void unsubscribe(entity *target);
    void signal(int event);

    inline float get_scale() const
    {
        return this->m_scale;
    }

    void set_scale(float new_scale)
    {
        float old_scale = this->get_scale();

        this->m_scale = new_scale;

        this->scale_changed(old_scale, new_scale);
    }

    inline float get_width() const
    {
        return this->width * this->get_scale();
    }

    virtual void scale_changed(float old_scale, float new_scale)
    {
        this->recreate_fixtures(false);
    }

    virtual void recreate_fixtures(bool initial) { }

    virtual void set_color(tvec4 c) {}
    virtual tvec4 get_color() {return tvec4f(0.f, 0.f, 0.f, 0.f);}

    void set_color4(float r, float g, float b, float a=1.0f)
    {
        this->set_color(tvec4f(r, g, b, a));
    }

    /*          event, cb */
    std::multimap<int, entity_listener> listeners;
    std::set<entity*> subscriptions;
    int update_method;
    int curr_update_method;
    b2Body *body;
    uint8_t layer_mask;
    //entity *owner;
    property *properties;
    uint8_t num_properties;

    p_gid g_id; /* global object type identifier */
    uint32_t emitted_by;
    uint32_t emit_step;

    b2Fixture *fx;
    group *gr;
    float height;
    float menu_scale;

    b2Vec2 menu_pos;
    b2Vec2 old_pos;

    /* Used as a base for connection-checking */
    b2Vec2 query_sides[4];
    float query_len;

    b2Vec2 _pos;
    float _angle;
    int dialog_id;

    /* default state contains velocity and angular velocity of a single body */
    float state[3];
    size_t state_ptr;
    size_t state_size;

    /* we keep track of where this entity was last written or read */
    size_t write_ptr;
    size_t write_size;

    chunk_intersection chunk_intersections[ENTITY_MAX_CHUNK_INTERSECTIONS];
    int                num_chunk_intersections;

    connection *conn_ll;

    uint8_t protection_status;
    void update_protection_status();

    virtual void set_moveable(bool moveable)
    {
        this->set_flag(ENTITY_IS_MOVEABLE, moveable);
    }

    bool is_moveable()
    {
        return this->flag_active(ENTITY_IS_MOVEABLE);
    }

    bool is_protected(bool include_platforms=false);

    uint8_t num_sliders;
    int  cull_effects_method;
    float interactive_hp;
    int  in_dragfield;

    uint64_t flags;

    virtual uint32_t get_fixture_connection_data(b2Fixture *f){return 0;};

    inline bool flag_active(uint64_t flag)
    {
        return this->flags & flag;
    }

    float get_total_mass()
    {
        float mass = 0.f;
        b2Body *b;

        for (int x=0; x<this->get_num_bodies(); x++) {
            if ((b = this->get_body(x))) {
                mass += b->GetMass();
            }
        }

        return mass;
    }

    inline void set_flag(uint64_t flag, bool v)
    {
        if (v) {
            this->flags |= flag;
        } else {
            this->flags &= ~flag;
        }
    }

    /* Returns true if the entity has a wireless functionality, meaning it
     * has at least one property and that first property is used for as
     * a wireless frequency. */
    inline bool is_wireless()
    {
        return (this->g_id == O_RECEIVER || this->g_id == O_TRANSMITTER || this->g_id == O_MINI_TRANSMITTER);
    }

    inline bool is_item()
    {
        return (this->g_id == O_ITEM);
    }

    inline bool is_prompt_compatible()
    {
        return this->flag_active(ENTITY_IS_PROMPT);
    }

    virtual bool is_zappable() { return this->flag_active(ENTITY_IS_ZAPPABLE); }

    inline bool is_explosive() { return this->flag_active(ENTITY_IS_EXPLOSIVE); }
    inline bool is_compressable() { return this->flag_active(ENTITY_CAN_BE_COMPRESSED); }
    inline bool is_static() { return this->flag_active(ENTITY_IS_STATIC); }
    inline bool is_bullet() { return this->flag_active(ENTITY_IS_BULLET); }
    inline bool is_creature() { return this->flag_active(ENTITY_IS_CREATURE); }
    inline bool is_robot() { return this->flag_active(ENTITY_IS_ROBOT); }
    inline bool is_control_panel() { return this->flag_active(ENTITY_IS_CONTROL_PANEL); }
    inline bool is_rc() { return this->flag_active(ENTITY_IS_CONTROL_PANEL); }
    inline bool is_edevice() { return this->flag_active(ENTITY_IS_EDEVICE); }
    inline bool is_interactive() { return this->flag_active(ENTITY_IS_INTERACTIVE); }
    inline bool is_composable() { return this->flag_active(ENTITY_IS_COMPOSABLE); }
    inline bool is_wheel() { return (this->type == ENTITY_WHEEL || this->type == ENTITY_GEAR); }
    inline bool is_gearbox() { return (this->g_id == O_GEARBOX); }
    inline bool allow_connections() { return this->flag_active(ENTITY_ALLOW_CONNECTIONS); }
    inline bool has_tracker() { return this->flag_active(ENTITY_HAS_TRACKER); }
    virtual bool is_locked() { return this->flag_active(ENTITY_IS_LOCKED); }

    /* If this function returns true, then when the entity is to be deleted by
     * pressing Delete or the "remove entity" widget, a confirmation will be shown. */
    virtual bool requires_delete_confirmation() { return false; }
    bool is_motor();
    bool is_high_prio();

    bool interacted_with; /* if the player has interacted with this entity
                             allows us to bypass auto protect and platform protect */

    entity();
    virtual ~entity();

    inline int sublayer_dist(entity *e)
    {
        int l0 = this->get_layer()*4;
        int l1 = e->get_layer()*4;

        if (this->layer_mask & 1)
            ;
        else if (this->layer_mask & 2)
            l0 += 1;
        else if (this->layer_mask & 4)
            l0 += 2;
        else if (this->layer_mask & 8)
            l0 += 3;

        if (e->layer_mask & 1)
            ;
        else if (e->layer_mask & 2)
            l1 += 1;
        else if (e->layer_mask & 4)
            l1 += 2;
        else if (e->layer_mask & 8)
            l1 += 3;

        return abs(l0-l1);
    }

    virtual void prepare_fadeout();

    virtual void ghost_update(){update();}

    virtual void construct(){}

    virtual edevice *get_edevice(){return 0;}
    virtual base_prompt *get_base_prompt(){return 0;}
    virtual activator *get_activator() { return 0; }
    void sidecheck(connection *c);
    void sidecheck4(connection *cc);

    void get_chunk_intersections(std::set<chunk_pos> *chunks);

    virtual void remove_connection(connection *cr);
    virtual void destroy_connection(connection *cr);
    virtual void disconnect_all();

    b2Joint*
    find_pivot(int dir, bool recursive)
    {
        if (this->is_wheel()) {
            connection *c = this->conn_ll;

            while (c) {
                if ((dir == 0 && c->layer == this->get_layer()-1)
                     || (dir == 1 && c->layer == this->get_layer() && c->multilayer)) {
                    if (c->type == CONN_PIVOT || (c->type == CONN_CUSTOM && c->option == 1)) {
                        return c->j;
                    } else if (recursive) {
                        b2Joint *j = c->get_other(this)->find_pivot(dir, recursive);
                        if (j) return j;
                    }
                }
                c = c->next[c->e == this ? 0 : 1];
            }
        }

        return 0;
    };

    inline void
    add_connection(connection *c)
    {
        //tms_infof("%p add connection %p", this, c);
        connection *bkp = this->conn_ll;
        this->conn_ll = c;
        c->next[(this == c->e) ? 0:1] = bkp;
    }

    inline bool
    connected_to(entity *e)
    {
        if (!this->conn_ll) return false;

        connection *c = this->conn_ll;

        do {
            if (c->e == this) {
                if (c->o == e) return true;
            } else {
                if (c->e == e) return true;
            }
        } while ((c = c->next[this == c->e ? 0:1]));

        return false;
    }

    void recreate_all_connections();
    void gather_connections(std::set<connection*> *out, std::set<entity*> *entities=0);
    void gather_connected_entities(
            std::set<entity*> *entities,
            bool include_cables=false,
            bool include_custom_conns=true,
            bool include_static=false,
            bool select_through_layers=true,
            int layer=-1
            );

    inline m* get_material(void)
    {
        return (m *)this->material;
    }

    virtual b2PolygonShape* get_resizable_shape(){return 0;};
    virtual int get_resizable_vertices(int *out){return 0;};
    virtual int get_resizable_edges(int *out){return 0;};
    virtual bool on_resize_vertex(int v, b2Vec2 new_pos){return false;};
    virtual bool on_resize_edge(int e, float movement){return false;};

    virtual const char *get_slider_label(int s){return 0;};
    virtual const char *get_slider_tooltip(int s){return 0;};
    virtual float get_slider_snap(int s){return 0.f;};
    virtual float get_slider_value(int s){return 0.f;};
    virtual void on_slider_change(int s, float value){};

    virtual entity *get_property_entity(void){return this;};

    virtual void update_effects(){};
    virtual void update();
    virtual void step(){};
    virtual void mstep(){};
    virtual void tick(){};
    virtual void pre_step(){};
    virtual void set_layer(int z);
    void set_prio(int z);

    // this is only fired in play-mode
    virtual int handle_event(uint32_t type, uint64_t pointer_id, tvec2 pos){return EVENT_CONT;};

    virtual void set_position(float x, float y, uint8_t frame=0);
    inline void set_position(b2Vec2 p, uint8_t frame=0) { set_position(p.x, p.y, frame); }

    virtual b2Vec2 get_position();
    virtual b2Vec2 get_position(uint8_t frame){return local_to_world(b2Vec2(0.f,0.f), frame);};

    virtual void set_angle(float a);
    virtual void set_angle(float a,uint8_t frame){set_angle(a);};

    virtual float get_angle();
    virtual float get_angle(uint8_t frame){return get_angle();};

    virtual void add_to_world()=0;
    virtual void remove_from_world();
    virtual void connection_create_joint(connection *c){};
    virtual bool connection_destroy_joint(connection *c){return false;};

    /* state reading/writing */
    virtual void restore();
    virtual void write_state(lvlinfo *lvl, lvlbuf *lb);
    virtual void read_state(lvlinfo *lvl, lvlbuf *lb);

    virtual void toggle_axis_rot(){};
    virtual void set_locked(bool locked, bool immediate=true);
    virtual void load_flags(uint64_t f);
    virtual uint64_t save_flags();
    virtual bool get_axis_rot(){return this->flag_active(ENTITY_AXIS_ROT);};
    virtual const char *get_axis_rot_tooltip() { return "Toggle axis rotation"; }
    virtual struct tms_sprite * get_axis_rot_sprite(){return 0;};

    /* "editor" events */
    /* XXX */
    virtual void on_grab(game *g);
    virtual void on_release(game *g);

    virtual void on_grab_playing(){};
    virtual void on_release_playing(){};

    virtual void on_touch(b2Fixture *my, b2Fixture *other);
    virtual void on_untouch(b2Fixture *my, b2Fixture *other);

    virtual void on_paused_touch(b2Fixture *my, b2Fixture *other){};
    virtual void on_paused_untouch(b2Fixture *my, b2Fixture *other){};

    /*
    virtual void collision_begin(b2Contact *c, int order) = 0;
    virtual void collision_presolve(b2Contact *c, int order) = 0;
    virtual void collision_postsolve(b2Contact *c, int order) = 0;
    virtual void collision_end(b2Contact *c, int order) = 0;
    */

    inline int get_layer(){return this->prio;};

    virtual bool compatible_with(entity *o);

    virtual b2BodyType get_dynamic_type();

    virtual b2Vec2 local_to_world(b2Vec2 p, uint8_t frame);
    virtual b2Vec2 world_to_local(b2Vec2 p, uint8_t frame);
    virtual b2Vec2 world_to_body(b2Vec2 p, uint8_t frame);
    virtual b2Vec2 local_to_body(b2Vec2 p, uint8_t frame);
    virtual b2Vec2 local_vector_to_body(b2Vec2 p, uint8_t frame);

    virtual uint32_t get_num_bodies();
    virtual b2Body *get_body(uint8_t frame);
    virtual void find_pairs(){};

    virtual const char *get_name(void) = 0;

    virtual const char *get_real_name(void)
    {
        return this->get_name();
    }

    virtual void write_tooltip(char *out);
    virtual void write_quickinfo(char *out);
    virtual void write_object_id(char *out);

    virtual void pre_write(void);
    virtual void post_write(void){};
    virtual void on_load(bool created, bool has_state){};
    virtual connection* load_connection(connection &conn){return 0;};

    inline void initialize_interactive(void)
    {
        this->in_dragfield = 0;
        this->interactive_hp = 1.f;
    };

    /* Init is always called, regardless if the entity
     * has a state to be loaded or not. */
    virtual void init() {};

    /* Setup is called after init if the entity has no
     * state to load from */
    virtual void setup()
    {
        this->listeners.clear();
        this->subscriptions.clear();
    };

    virtual void on_pause()
    {
        this->subscriptions.clear();
        this->listeners.clear();
    };

    virtual void on_absorb() {};

    inline void on_entity_play()
    {
        this->interacted_with = false;
    }

    virtual bool allow_connection(entity *asker, uint8_t frame, b2Vec2 p){return true;};
    virtual bool enjoys_connection(uint32_t g_id) { return false; }

    /* Zappable stuff */
    float entity_health;

    struct worth worth;

    virtual void entity_damage(float dmg);
    virtual void drop_worth();

    void set_property(uint8_t id, float v);
    void set_property(uint8_t id, uint32_t v);
    void set_property(uint8_t id, const char *v);
    void set_propertyi8(uint8_t id, uint8_t v);
    inline void set_property(uint8_t id, uint8_t v){set_property(id, (uint32_t)v);};
    inline void set_property(uint8_t id, uint16_t v){set_property(id, (uint32_t)v);};
    void set_property(uint8_t id, const char *v, uint32_t len);

    void set_num_properties(uint8_t num);
    void reset_flags(void);

    /* this takes a value between 0.0 and 1.0 and converts it to the proper density scale value */
    void help_set_density_scale(float value)
    {
        this->set_density_scale(ENTITY_DENSITY_SCALE_MIN + value*(ENTITY_DENSITY_SCALE_MAX-ENTITY_DENSITY_SCALE_MIN));
    }

    static float distance_to_fixture(const b2Vec2 &query_point, b2Fixture *fx);
    static tvec2 get_nearest_point(const b2Vec2 &query_point, b2Fixture *fx);

    virtual void set_density_scale(float v) {}
    virtual float get_density_scale(){return 1.f;};

    virtual uint32_t get_sub_id() { return 0; }

    friend class group;
    friend class composable;
    friend class connection;
};

class entity_simpleconnect : public entity, public b2RayCastCallback
{
  public:
    b2Vec2 query_pt;
    b2Vec2 query_vec; /* Set in the constructor of the
                         object, direction of raycast */
    entity *query_result;
    b2Fixture *query_result_fx;
    uint8_t query_frame;
    connection c;

    entity_simpleconnect()
    {
        this->c.init_owned(0, this);
        this->c.type = CONN_GROUP;
        this->query_vec = b2Vec2(0.f, -.5f);
        this->query_pt = b2Vec2(0.f, 0.f);
    }

    virtual connection *load_connection(connection &conn);
    float32 ReportFixture(b2Fixture *f, const b2Vec2 &pt, const b2Vec2 &nor, float32 fraction);
    void find_pairs();
};

class entity_multiconnect : public entity
{
  public:
    connection c_side[4];

    entity_multiconnect()
    {
        this->c_side[0].init_owned(0, this); this->c_side[0].type = CONN_GROUP;
        this->c_side[1].init_owned(1, this); this->c_side[1].type = CONN_GROUP;
        this->c_side[2].init_owned(2, this); this->c_side[2].type = CONN_GROUP;
        this->c_side[3].init_owned(3, this); this->c_side[3].type = CONN_GROUP;
    }

    connection *load_connection(connection &conn);
    void find_pairs();
};

class ghost : public entity, public b2QueryCallback
{
  public:
    connection c;

    ghost();
    void update();
    const char *get_name(){return "Shape Extruder";};
    void init();
    bool ReportFixture(b2Fixture *f);
    void find_pairs();
    void add_to_world();
    connection* load_connection(connection &conn);
    void connection_create_joint(connection *c);
    bool connection_destroy_joint(connection *c);
};

void entity_fast_update(struct tms_entity *t);
