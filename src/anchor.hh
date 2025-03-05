#pragma once

#include "edevice.hh"

class faction_info;

// anchor types
enum {
    ANCHOR_GUARDPOINT,
};

class anchor : public ecomp_multiconnect
{
  public:
    int anchor_type;
    bool active;
    faction_info *faction;

    anchor(int anchor_type);
    const char *get_name()
    {
        switch (this->anchor_type) {
            case ANCHOR_GUARDPOINT: return "Guard point";
        }

        return "Unknown Anchor";
    }

    void on_load(bool created, bool has_state);

    faction_info* set_faction(uint8_t faction_id);
    faction_info* set_faction(faction_info *faction);

    edevice* solve_electronics();

    inline bool is_active() { return this->active; }

    void write_state(lvlinfo *lvl, lvlbuf *lb)
    {
        entity::write_state(lvl, lb);

        lb->w_s_bool(this->active);
    }

    void read_state(lvlinfo *lvl, lvlbuf *lb)
    {
        entity::read_state(lvl, lb);

        this->active = lb->r_bool();
    }
};
