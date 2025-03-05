#pragma once

#include "composable.hh"

class pivot : public composable_simpleconnect
{
  //private:
  public:
    float dir;
    pivot();
    void on_grab(game *g);
    void on_release(game *g);
};

class pivot_1 : public pivot
{
  public:
    b2PivotJoint *joint; 
    connection dconn;

    pivot_1();

    const char* get_name(){return "Open Pivot";}
    void construct();
    connection *load_connection(connection &conn);
    void connection_create_joint(connection *c);
    void update_frame(bool hard);
    void set_layer(int z);
};

class pivot_2 : public pivot
{
  public:
    pivot_1 *p1;
    pivot_2();
    void update_frame(bool hard);

    entity *get_property_entity(){return p1?(entity*)p1:(entity*)this;};
    const char* get_name(){return "Open Pivot (part)";}
    void set_layer(int z);
};
