#pragma once

#include "entity.hh"

#define RAIL_STRAIGHT 0
#define RAIL_SKEWED   1
#define RAIL_45DEG    2
#define RAIL_SKEWED2  3

class rail : public entity
{
  private:
      int railtype;
  public:
      rail(int type);
      const char *get_name(){
          switch (this->railtype) {
              case RAIL_STRAIGHT: return "Rail";
              case RAIL_SKEWED: return "Rail (up)";
              case RAIL_SKEWED2: return "Rail (down)";
              case RAIL_45DEG: return "Rail (turn)";
          }
          return "Rail";
      }
      void add_to_world();
      void set_angle(float a);
      void set_position(float x, float y, uint8_t frame=0);
};
