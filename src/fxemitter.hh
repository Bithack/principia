#pragma once

#include "i1o1gate.hh"

#include <set>

#define FX_EXPLOSION   0
#define FX_HIGHLIGHT   1
#define FX_DESTROYCONN 2
#define FX_SMOKE       3
#define FX_MAGIC       4
#define FX_BREAK       5

#define FX_INVALID 0xdeadbeef

#define FXEMITTER_NUM_EFFECTS 4

#define DISCHARGE_MAX_POINTS 10

#define NUM_SPARKS 3

#define SPARK_DECAY_RATE 2.0f

#define NUM_FIRES 20
#define EXPLOSION_FIRE_DECAY_RATE 2.5f
#define EXPLOSION_DECAY_RATE 4.0f

#define NUM_SMOKE_PARTICLES 3

#define NUM_BREAK_PARTICLES 3
#define BREAK_DECAY_RATE 2.5f

#define NUM_FLAMES 30

class base_effect : public entity
{
  public:
    const char *get_name() { return "Base effect"; }
    base_effect() : entity()
    {
        this->set_flag(ENTITY_IS_OWNED,             true);
        this->set_flag(ENTITY_DO_MSTEP,             true);
        this->set_flag(ENTITY_DO_UPDATE_EFFECTS,    true);
        this->set_flag(ENTITY_FADE_ON_ABSORB,       false);
    }

    void add_to_world(){};
};

class fxemitter : public i1o1gate_fifo
{
    bool     activated;
    bool     completed;
    uint64_t time;

    int      num_emitted[4];
    uint64_t next[4];

    std::set<connection *> *conns;

    uint64_t get_next_time()
    {
        if (this->properties[2].v.f > 0.f) {
            return ((uint64_t)rand())%(uint64_t)(this->properties[2].v.f * 1000000.f);
        }

        return 0llu;
    };

  public:
    edevice* solve_electronics();
    const char* get_name(){return "FX Emitter";}
    void update_effects(void);

    void step();
    void setup();
    void on_pause();

    fxemitter();

    void write_state(lvlinfo *lvl, lvlbuf *lb)
    {
        entity::write_state(lvl, lb);

        lb->w_s_uint8(this->activated ? 1 : 0);
        lb->w_s_uint8(this->completed ? 1 : 0);
        lb->w_s_uint64(this->time);

        for (int n=0; n<4; ++n) {
            lb->w_s_uint32(this->num_emitted[n]);
            lb->w_s_uint64(this->next[n]);
        }
    }

    void read_state(lvlinfo *lvl, lvlbuf *lb)
    {
        entity::read_state(lvl, lb);

        this->activated = (lb->r_uint8() != 0);
        this->completed = (lb->r_uint8() != 0);
        this->time = lb->r_uint64();

        for (int n=0; n<4; ++n) {
            this->num_emitted[n] = (int)lb->r_uint32();
            this->next[n] = lb->r_uint64();
        }
    }

    void restore()
    {
        entity::restore();

        this->conns = 0;
    }
};

struct particle {
    float x;
    float y;
    float z;
    float life;
    float s;
    float a;
};

struct piece {
    float x; float y;
    float vx; float vy;
    float life;
};

struct flame {
    float x;
    float y;
    float vx;
    float vy;
    float life;
    float s;
    float a;
};

class debris : public entity
{
  private:
    b2Vec2 initial_force;
    float life;
  public:
    debris(b2Vec2 force, b2Vec2 pos);
    const char *get_name() { return "Debris"; }
    void add_to_world();
    void step(void);
};

class explosion_effect : public base_effect
{
  private:
    float scale;
    float       life;
    struct particle particles[NUM_FIRES];
    /*struct piece pieces[5];*/
    b2Vec2      trigger_point;
    bool   played_sound;

  public:
    explosion_effect(b2Vec2 pos, int layer, bool with_debris=true, float scale=1.f);
    void mstep();
    void update_effects();
};

class spark_effect : public base_effect
{
  private:
    float life;
    struct piece pieces[NUM_SPARKS];
    b2Vec2      trigger_point;
    bool   played_sound;

  public:
    spark_effect(b2Vec2 pos, int layer);
    void mstep();
    void update_effects();
};

class smoke_effect : public base_effect
{
  private:
    struct particle particles[3];
    b2Vec2      trigger_point;
    float col;
    float scale;

  public:
    smoke_effect(b2Vec2 pos, int layer, float col=0.f, float scale = 1.f);
    void mstep();
    void update_effects();
};

class magic_effect : public base_effect
{
  private:
    struct particle *particles;
    int num_particles;
    void create_particle(int slot);

  public:
    magic_effect(b2Vec2 pos, int layer, int num_particles=3);
    ~magic_effect();
    void mstep();
    void update_effects();
    void update_pos(b2Vec2 pos, int layer);

    bool continuous;
    tvec4 color;
    float speed_mod;
};

class break_effect : public base_effect
{
  private:
    struct piece pieces[3];
    b2Vec2 trigger_point;

  public:
    break_effect(b2Vec2 pos, int layer);
    void mstep();
    void update_effects();
};

class discharge_effect : public base_effect
{
  private:
    b2Vec2 p[2];
    float  life;
    float  start_z;
    float  end_z;
    float  shift_dir;
    float  displ[DISCHARGE_MAX_POINTS];
    int    num_points;

  public:
    float  line_width;

    discharge_effect(b2Vec2 start, b2Vec2 end, float start_z, float end_z, int num_points, float life);
    void set_points(b2Vec2 start, b2Vec2 end, float start_z, float end_z);
    void mstep();
    void update_effects();
};

class flame_effect : public base_effect
{
  private:
      struct flame flames[NUM_FLAMES];
      b2Vec2 v;
      int flame_n;
      int f_type; //0 = thruster, 1 = rocket
      float sep;
      float thrustmul;
      float z_offset;
      bool disable_sound;

  public:
      bool done;

      flame_effect(b2Vec2 pos, int layer, int f_type, bool disable_sound=false);
      void update_pos(b2Vec2 pos, b2Vec2 v);
      void step();
      void update_effects();
      void set_thrustmul(float thrustmul);
      void set_z_offset(float z_offset);
};

#define TESLA_NUM_RAYS 16
#define TESLA_MAX_PATHS 10

class tesla_effect : public base_effect, public b2RayCastCallback
{
    int               rnd[TESLA_NUM_RAYS];
    discharge_effect *paths[TESLA_MAX_PATHS];
    int               num_paths;

    std::set<entity*> charged_entities;


    entity           *ignore;
    uint32_t ignore_id;

    float             range;

    /* results from reportfixture */
    b2Fixture        *res_fx;
    b2Vec2            res_pt;

    void search(b2Vec2 pt);

  public:
    float             min_angle;
    float             max_angle;
    tesla_effect(entity *source, b2Vec2 pos, int layer);
    ~tesla_effect();

    int get_num_paths(){return this->num_paths;};
    void    update_effects(void);
    void    step();
    float32 ReportFixture(b2Fixture *f, const b2Vec2 &pt, const b2Vec2 &nor, float32 fraction);
};

class plasma_explosion_effect : public base_effect
{
  private:
    struct particle particle;
    b2Vec2          trigger_point;
    float col;
    float scale;

  public:
    plasma_explosion_effect(b2Vec2 pos, int layer, float col=0.f, float scale = 1.f);
    void mstep();
    void update_effects();
};

