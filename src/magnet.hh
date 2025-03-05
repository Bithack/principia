#pragma once

#include "edevice.hh"

#include <map>

typedef std::multimap<entity*, b2Fixture*> object_map;
typedef object_map::iterator object_map_iter;

/* XXX: simpleconnect? */
class magnet : public edev
{
  private:
    object_map objects;
    b2Fixture *sensor;

    int  type;
    bool active;
    float strength;
    float strength_mul;

  public:
    magnet(int type);
    const char* get_name(){
        switch (this->type) {
            case 0: return "Magnet";
            case 1: return "Electromagnet";
        }
        return "";
    };

    void recreate_shape();
    void add_to_world();
    void step(void);

    void on_touch(b2Fixture *my, b2Fixture *other);
    void on_untouch(b2Fixture *my, b2Fixture *other);
    edevice* solve_electronics();

    void apply_magnetism(b2Fixture *f, float dist, float multiplier);
};
