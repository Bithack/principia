#pragma once

#include "entity.hh"
#include "group.hh"

class world;

class composable : public entity
{
  public:
    b2FixtureDef fd;
    b2Fixture   *fx_sensor;

    /**
     * C++ doesn't allow classes with constructors in unions,
     * otherwise we would have used union here
     **/
    struct /*union */{
        struct { b2PolygonShape shape; } poly;
        struct { b2CircleShape  shape; } circle;
    } orig;

    struct /*union */{
        struct { b2PolygonShape shape; } poly;
        struct { b2CircleShape  shape; } circle;
    } active;

    composable()
    {
        this->set_flag(ENTITY_IS_COMPOSABLE, true);
        this->fd.shape = 0;
        this->fx_sensor = 0;
    }
    ~composable()
    {
    }

    void grouped_update();

    virtual void set_position(float x, float y, uint8_t frame=0);
    void set_angle(float a);
    b2Vec2 get_position(void);
    float get_angle(void);
    b2Vec2 local_to_world(b2Vec2 p, uint8_t frame);
    b2Vec2 world_to_local(b2Vec2 p, uint8_t frame);
    b2Vec2 local_to_body(b2Vec2 p, uint8_t frame);
    b2Vec2 local_vector_to_body(b2Vec2 p, uint8_t frame);
    b2Body *get_body(uint8_t frame);
    void set_as_circle(float r);
    virtual void set_as_rect(float width, float height);
    void set_as_tri(float width, float height);
    void set_as_poly(b2Vec2 *verts, int num_verts);
    void recreate_shape(void);
    void update_shape(b2Vec2 local_pos, float local_angle);
    virtual void update_frame(bool hard){};

    void refresh_poly_shape();
    void refresh_circle_shape();
    void recreate_fixtures(bool initial);
    void add_to_world();
    void remove_from_world();

    virtual void create_sensor();
    virtual float get_sensor_radius() { return 0.f; };
    virtual b2Vec2 get_sensor_offset() { return b2Vec2(0.f, 0.f); };

    friend class group;
};

class composable_simpleconnect : public composable, public b2RayCastCallback
{
  public:
    b2Vec2 query_pt;
    b2Vec2 query_vec; /* Set in the constructor of the
                         object, direction of raycast */
    entity *query_result;
    b2Fixture *query_fx;
    uint8_t query_frame;
    connection c;

    composable_simpleconnect()
    {
        this->c.init_owned(0, this);
        this->c.type = CONN_GROUP;
        this->query_vec = b2Vec2(0.f, -.5f);
        this->query_pt = b2Vec2(0.f, 0.f);
    }

    connection *load_connection(connection &conn);
    float32 ReportFixture(b2Fixture *f, const b2Vec2 &pt, const b2Vec2 &nor, float32 fraction);
    void find_pairs();
};

class composable_multiconnect : public composable
{
  public:
    connection c_side[4];

    composable_multiconnect()
    {
        this->c_side[0].init_owned(0, this); this->c_side[0].type = CONN_GROUP;
        this->c_side[1].init_owned(1, this); this->c_side[1].type = CONN_GROUP;
        this->c_side[2].init_owned(2, this); this->c_side[2].type = CONN_GROUP;
        this->c_side[3].init_owned(3, this); this->c_side[3].type = CONN_GROUP;
    }

    connection *load_connection(connection &conn);
    void find_pairs();
    void set_as_rect(float width, float height);
};

void composable_fast_update(struct tms_entity *t);

inline void composable::grouped_update()
{
    b2Vec2 p = this->gr->body->GetWorldPoint(this->_pos);
    float a = this->gr->body->GetAngle()+this->_angle;

    float c,s;
    tmath_sincos(a, &s, &c);

    this->M[0] = c;
    this->M[1] = s;
    this->M[4] = -s;
    this->M[5] = c;
    this->M[12] = p.x;
    this->M[13] = p.y;
    this->M[14] = this->prio * LAYER_DEPTH;

    tmat3_copy_mat4_sub3x3(this->N, this->M);
}
