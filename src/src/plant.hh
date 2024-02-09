#pragma once

#include "entity.hh"
#include "ud2.hh"

#define PLANT_SEED_TIME 1.f
#define PLANT_MAX_SECTION_ANGLE (M_PI/2.f)

class plant_branch;

class plant_leaf : public tms::entity {
  public:
    b2Fixture *f;
    float growth;

    plant_leaf(int leaf_type);
    const char *get_name() { return "Leaf"; }
};

class plant_section : public ud2_info {
  public:
    b2Vec2        pos;
    float         angle;
    float         growth;
    float         width_growth;

    float         update_width; /* width at last update */
    float         update_growth; /* growth at last update */

    float         shift;
    int           stage; /* stage 0 = growing, stage 1 = still growing but there's another section after this one, stage 2 = finished growing */
    float         hp;
    float         damage_timer;
    plant_section *next;
    plant_branch  *branch; /* branch this section belongs to */
    plant_branch  *extension; /* if a branch extends from this section */
    b2Fixture    *f;

    plant_section() : ud2_info(UD2_PLANT_SECTION) { this->clear(); };
    void clear()
    {
        angle = 0.f;
        pos = b2Vec2(0.f, 0.f);
        growth = 0.f;
        width_growth = 0.f;
        shift = 0.f;
        next = 0;
        branch = 0;
        extension = 0;
        f = 0;
        stage = 0;
        hp = 1.f;
        damage_timer = 0.f;
        update_width = 0;
        update_growth = 0;
    };

    plant_section(plant_section *s);

    b2Vec2 get_end_point();
    b2Vec2 get_mid_point();
    inline b2Vec2 get_start_point() { return this->pos; };
    b2Vec2 get_vector();

    float get_width();
    float get_shift();
    float get_angle_displacement();
};

class plant_branch : public tms_entity
{
  public:
    plant_section *first;
    plant_section *last;
    plant_leaf    *leaf;
    plant_section *parent;

    int             dir;

    bool            needs_update;

    bool            dead; /* whether this branch can grow or not */

    bool            derived_body;
    b2Body          *b;
    b2RevoluteJoint *j; /* the joint between this branch and the parent branch */
    float            reference_angle;
    b2Vec2           reference_point;

    b2Body          *b_tmp; /* temporary body used for the outer-most sectino of the branch */
    b2Fixture       *f_tmp; /* temporary round fixture used for smooth growing around corners */
    b2RevoluteJoint *j_tmp;

    int             slot;
    struct tms_mesh _mesh;
    int           depth;
    int           dir_toggle;

    float         section_length;
    float         section_width;
    float         section_width_multiplier;
    int           sections_left;
    int           sections_done;
    int           extension_probability;

    plant_branch();

    void create_joint(b2Body *other, b2Vec2 point, bool collide);
};

enum {
    PLANT_TREE,
    PLANT_BUSH,
    PLANT_COLORFUL_BUSH,
    PLANT_BIG_TREE,
    PLANT_SAND_TREE,
    PLANT_ROUGH_TREE,
    PLANT_CLIMBER,

    NUM_PLANT_PREDEFS
};

enum {
    LEAF_DEFAULT,
    LEAF_SIMPLE,
    LEAF_CIRCLES,
    LEAF_CYLINDER,

    NUM_LEAF_TYPES
};

extern struct plant_predef {
    const char *name;
    tvec2 angle_jitter;
    tvec2 section_width;
    tvec2 gravity_influence;
    tvec2 sun_influence;
    int   num_sections;
    tvec2 section_length;
    tvec2 section_width_multiplier;
    tvec2 leaf_size;
    tvec3 leaf_color;
    int   leaf_type;
} plant_predefs[NUM_PLANT_PREDEFS];

class plant : public entity_simpleconnect
{
  public:
    b2Body       *bodies[64];
    int           num_bodies;

    b2Body *create_body(b2BodyDef *bd);
    void destroy_body(b2Body *b);

    b2Fixture *pending_fixture;
    float      pending_timer;
    b2Vec2     pending_normal;

    inline float rand_range(tvec2 v)
    {
        float r = (float)rand()/(float)RAND_MAX;
        return r*v.x + (1.f-r)*v.y;
    }

    void write_state(lvlinfo *lvl, lvlbuf *lb)
    {
        lb->w_s_float(this->pending_timer);
        /* XXX velocities are packed in the serialization */
    }

    void read_state(lvlinfo *lvl, lvlbuf *lb)
    {
        this->pending_timer = lb->r_float();
    }

    inline void set_from_predef(int p)
    {
        if (p >= NUM_PLANT_PREDEFS) {
            p = 0;
        }
        this->properties[1].v.f = rand_range(plant_predefs[p].angle_jitter);
        this->properties[2].v.f = rand_range(plant_predefs[p].section_width);
        this->properties[3].v.f = rand_range(plant_predefs[p].gravity_influence);
        this->properties[4].v.f = rand_range(plant_predefs[p].sun_influence);
        this->properties[5].v.i = plant_predefs[p].num_sections;
        this->properties[6].v.f = rand_range(plant_predefs[p].section_length);
        this->properties[7].v.f = rand_range(plant_predefs[p].section_width_multiplier);
        this->properties[8].v.f = rand_range(plant_predefs[p].leaf_size);

        this->properties[9].v.f = plant_predefs[p].leaf_color.r;
        this->properties[10].v.f = plant_predefs[p].leaf_color.g;
        this->properties[11].v.f = plant_predefs[p].leaf_color.b;
        this->properties[13].v.i8 = plant_predefs[p].leaf_type;

        this->root_branch.sections_left = this->get_num_sections();
        this->init_branch(&this->root_branch, 0);
    }

    plant();
    ~plant();

    static void upload_buffers();
    static void reset_counter();

    int get_branch_mesh_slot();

    void add_to_world();
    void remove_from_world();
    void clear_branch_slots(plant_branch *br);
    const char *get_name(){return "Plant";};
    void step(void);
    void tick(void);
    void init();
    void setup();
    void restore();
    void update(void);
    void update_effects();
    void render_damage(plant_branch *br);
    void pre_write();
    void on_load(bool created, bool has_state);

    void damage_section(plant_section *s, float damage, damage_type dmg_type);
    void break_branch(plant_branch *br, plant_section *s, bool create_resources);

    uint32_t get_num_bodies()
    {
        return this->num_bodies+1;
    }

    b2Body *get_body(uint8_t frame)
    {
        if (frame == 0) {
            return this->body;
        }
        return this->bodies[frame-1];
    }

    void set_position(float x, float y, uint8_t frame);
    
    connection *load_connection(connection &c);
    void connection_create_joint(connection *c);
    bool connection_destroy_joint(connection *c);

    void kill_branch(plant_branch *br);
    void grow_branch(plant_branch *br, float time);
    void adjust_branch_joint(plant_branch *br, bool recursive);

    void begin_section_fixture(plant_section *s, b2Vec2 b_tmp_vel, float b_tmp_avel);
    void update_section_fixture(plant_section *s, float time);
    void end_section_fixture(plant_section *s);

    void serialize_leaf(lvlbuf *lb, plant_leaf *l);
    void serialize_section(lvlbuf *lb, plant_section *s);
    void serialize_branch(lvlbuf *lb, plant_branch *br);
    void unserialize_branch(lvlbuf *lb, plant_section *parent_section, plant_branch *br);
    void unserialize_section(lvlbuf *lb, plant_section *s);
    void unserialize(lvlbuf *lb);

    plant_section *create_section(plant_section *s);
    plant_branch *create_branch(plant_section *s);
    void promote_branch(plant_branch *n, bool mesh_only=false);
    void init_branch(plant_branch *br, plant_section *parent_section);
    void init_section(plant_section *n);
    void update_leaf(plant_branch *br);
    plant_leaf *create_leaf(plant_branch *br);

    void update_meshes(plant_branch *br);
    int update_mesh(plant_section *s, struct vertex *v, int y, bool search_only);
    int mesh_add_pre_branch_sections(plant_branch *br, struct vertex *v, int y);
    int mesh_add_post_branch_sections(plant_branch *br, struct vertex *v, int y);
    int mesh_add_section(struct vertex *v, int y, b2Vec2 position, b2Vec2 axis, float width);

    void settle();

    float get_grow_strength(plant_branch *br)
    {
        return 100.f;
    }

    float get_angle_jitter(plant_branch *br)
    {
        return this->properties[1].v.f/(1.f+br->depth);
    }

    float get_leaf_size()
    {
        return this->properties[8].v.f;//*.25f;
    }

    float get_section_length(plant_branch *br)
    {
        return (this->properties[6].v.f/16.f) * std::max(this->get_num_sections(), 8);
    }

    float get_strength()
    {
        return 10*this->get_num_sections();
    }

    float get_section_width(plant_branch *br);

    float get_section_width_multiplier(plant_branch *br)
    {
        return this->properties[7].v.f;
    }

    float get_gravity_influence(plant_branch *br)
    {
        return this->properties[3].v.f;
    }

    float get_sun_influence(plant_branch *br)
    {
        return this->properties[4].v.f;
    }

    int get_num_sections()
    {
        return this->properties[5].v.i;
    }

    int get_max_depth()
    {
        return 2;
    }

    bool should_create_branch(plant_section *s)
    {
        return !s->branch->dead && s->branch->extension_probability >= (rand()%3)
            && s->branch->depth < this->get_max_depth()
            && (s->branch->sections_done > 2 || s->branch->sections_left < 6)
            && s != s->branch->first
            ;
    }

    plant_section root_section;
    plant_branch root_branch;
};
