#pragma once

#include <vector>
#include <set>

#include "entity.hh"

class composable;

class group : public entity
{
  private:
    std::vector<composable*> entities;
    std::vector<connection*> connections;

    void recreate_all_entity_joints(bool hard);
    void recreate_entity_joints(composable *e, bool hard);
    void add_entity(composable *e);
    void merge(group *g, connection *c);
    void make_group(composable *e, std::set<composable *> *pending, std::set<composable *> *found, std::set<connection *> *found_conn);
    void reset_origo(bool hard_recreate);

    /* for grouping the nails and shit */
    tms_varray *va;
    tms_gbuffer *vbuf;
    tms_gbuffer *ibuf;

    /* for grouping wooden objects */
    tms_varray wooden_va;
    tms_gbuffer wooden_vbuf;
    tms_gbuffer wooden_ibuf;
    tms_mesh wooden_mesh[3];
    tms_entity wooden_entity[3];
    int wooden_count[3];

    /* for grouping plastic objects */
    tms_varray plastic_va;
    tms_gbuffer plastic_vbuf;
    tms_gbuffer plastic_ibuf;
    tms_mesh plastic_mesh[3];
    tms_entity plastic_entity[3];
    int plastic_count[3];

    b2Vec2 origo_offset;

  public:
    virtual const char *get_name() { return "Group"; }
    group();
    ~group();

    void update(void);
    void create_mesh(void);
    void finalize(void);
    void add(connection *c);
    void remove_entity(composable *e);
    virtual void add_to_world();
    virtual void remove_from_world();
    void dangle();

    void push_entity(composable *e, b2Vec2 p, float angle);
    void push_connection(connection *c);
    virtual void on_load(bool created, bool has_state);

    void rebuild();

    bool is_locked();

    friend class entity;
    friend class composable;
    friend class of;
    friend class world;
    friend class game;
    friend class selection_handler;
};
