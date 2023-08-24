#pragma once

#include "composable.hh"

class corner_ray_cb;

class corner : public composable
{
  private:
    connection c[3];
    void create();

  public:
    corner();

    virtual const char* get_name(){
        return "Corner";
    }
    virtual void find_pairs();
    connection* load_connection(connection &conn);

    void on_load(bool created, bool has_state);

    friend class corner_ray_cb;
};
