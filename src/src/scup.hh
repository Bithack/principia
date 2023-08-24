#pragma once

#include "edevice.hh"

#define SCUP_REACH .375f
#define SCUP_JOINT_FORCE 1500.f
#define SCUP_NUM_JOINTS  3
#define SCUP_WIDTH       .6f

class scup : public ecomp_multiconnect
{
  public:
    scup();
    const char *get_name() { return "Suction Cup"; };

    void setup();
    void restore();
    void step();

    edevice* solve_electronics();

    void write_state(lvlinfo *lvl, lvlbuf *lb);
    void read_state(lvlinfo *lvl, lvlbuf *lb);

    bool stuck;
    float strength_mod;
    b2Joint *j[SCUP_NUM_JOINTS];
    struct attachment {
        uint32_t id;
        uint8_t  frame;
        float    force;
        b2Vec2   body_pt;
    } a[SCUP_NUM_JOINTS];
    joint_info *ji;
};
