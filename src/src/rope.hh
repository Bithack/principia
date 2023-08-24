#pragma once

#include "entity.hh"

/* If rope length is changed, we need a version check for it. */
#define ROPE_LENGTH 12

class rope;

class rope_end : public entity
{
  private:
    rope *r;

  public:
    rope_end();
    const char *get_name() { return "Rope end"; }
    void add_to_world();

    friend class rope;
};

class rope : public entity,
             public b2RayCastCallback
{
  private:
    b2Body *rb[ROPE_LENGTH];
    rope_end *ends[2];
    connection end_conns[2];
    entity *query_result;
    b2Fixture *query_result_fx;
    b2Vec2 query_point;
    float query_fraction;
    uint8_t query_frame;
    int num; /* TODO: allocate slot in update every frame */

  public:
    rope();

    void add_to_world();
    void remove_from_world();
    void update(void);
    void refresh_predef_form();
    void ghost_update(void);
    void set_position(float x, float y, uint8_t frame=0);
    void set_angle(float a, uint8_t frame);
    float get_angle(uint8_t frame);
    static struct tms_entity* get_entity();

    void construct();
    const char* get_name(){return "Rope";};
    void pre_write();
    void set_layer(int l);

    static void reset_counter(void);
    static void upload_buffers(void);
    static void _init();

    uint32_t get_num_bodies();
    b2Body* get_body(uint8_t frame);
    b2Vec2 local_to_body(b2Vec2 p, uint8_t frame){return this->ends[frame]->local_to_body(p, frame);};
    b2Vec2 local_to_world(b2Vec2 p, uint8_t frame){return this->ends[frame]->local_to_world(p, frame);};
    b2Vec2 world_to_local(b2Vec2 p, uint8_t frame){return this->ends[frame]->world_to_local(p, frame);};
    b2Vec2 world_to_body(b2Vec2 p, uint8_t frame){return this->ends[frame]->world_to_body(p, frame);};

    connection *load_connection(connection &conn);
    void find_pairs();
    float32 ReportFixture(b2Fixture *f, const b2Vec2 &pt, const b2Vec2 &nor, float32 fraction);

    void write_state(lvlinfo *lvl, lvlbuf *lb);
    void read_state(lvlinfo *lvl, lvlbuf *lb);
    void restore();
    void step();

    /* (num_bodies + ROPE_LENGTH) * 3 */
    float state[(2 + ROPE_LENGTH) * 3];
};
