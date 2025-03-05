#pragma once

#include <map>
#include <stdint.h>
#include "types.hh"
#include <cmath>


class gentype;

#define GENTYPE_MAX_REACH_X 4
#define GENTYPE_MAX_REACH_Y 18

#define TERRAIN_EDIT_LAYER0   (1<<0)
#define TERRAIN_EDIT_LAYER1   (1<<1)
#define TERRAIN_EDIT_LAYER2   (1<<2)
#define TERRAIN_EDIT_TOUCH    (1<<3)  /* only touch/occupy the pixel, do not modify it */
#define TERRAIN_EDIT_SOFT     (1<<4)  /* soft modify, only set the pixel if it is empty, do not modify already existing materials*/
#define TERRAIN_EDIT_INC      (1<<5)  /* increase the pixel material only */
#define TERRAIN_EDIT_DEC      (1<<6)  /* decrease the pixel material only */
#define TERRAIN_EDIT_SOFTEN   (1<<7)  /* decrease the pixel material by one step */
#define TERRAIN_EDIT_HARDEN   (1<<8)  /* increase the pixel material by one step */
#define TERRAIN_EDIT_NONEMPTY (1<<9)  /* only set the pixel if it is NOT empty */

struct terrain_edit {
    uint32_t flags;
    int      data;

    terrain_edit(uint32_t flags, int data)
    {
        this->flags = flags;
        this->data = data;
    }

    terrain_edit(){};
};

struct terrain_coord {
    int chunk_x;
    int chunk_y;
    uint8_t _xy;

    terrain_coord(){};

    terrain_coord(float world_x, float world_y)
    {
        this->set_from_world(world_x, world_y);
    }

    inline int get_global_x()
    {
        return chunk_x*16+this->get_local_x();
    }

    inline int get_global_y()
    {
        return chunk_y*16+this->get_local_y();
    }

    inline float get_world_x()
    {
        return this->chunk_x * 8.f + this->get_local_x()*.5f;
    }

    inline float get_world_y()
    {
        return this->chunk_y * 8.f + this->get_local_y()*.5f;
    }

    inline void set_from_world(float x, float y)
    {
        int gx = (int)roundf(x * 2.f);
        int gy = (int)roundf(y * 2.f);
        this->set_from_global(gx, gy);
    }

    inline void set_from_global(int gx, int gy)
    {
        this->chunk_x = (int)floor(gx/16.f);
        this->chunk_y = (int)floor(gy/16.f);

        int local_x = (int)(gx-this->chunk_x*16) & 0x0f;
        int local_y = (int)(gy-this->chunk_y*16) & 0x0f;

        this->_xy = (local_y << 4) | local_x;
    }

    void step(int x, int y)
    {
        int gx = this->chunk_x * 16 + this->get_local_x();
        int gy = this->chunk_y * 16 + this->get_local_y();

        gx += x;
        gy += y;

        this->set_from_global(gx, gy);
    }

    chunk_pos get_chunk_pos()
    {
        return chunk_pos(chunk_x, chunk_y);
    }

    int get_local_x()
    {
        return (_xy) & 0xf;
    }

    int get_local_y()
    {
        return (_xy >> 4) & 0xf;
    }
};

enum {
    TERRAIN_TRANSACTION_EMPTY        = 0,
    TERRAIN_TRANSACTION_READY        = 1,
    TERRAIN_TRANSACTION_OCCUPYING    = 2,
    TERRAIN_TRANSACTION_OCCUPIED     = 3,
    //TERRAIN_TRANSACTION_FAILED       = 4,
    TERRAIN_TRANSACTION_APPLIED      = 5,
    TERRAIN_TRANSACTION_INVALIDATED  = 6,
};

struct terrain_transaction
{
    /* multimap sorted by chunk (see < operator of terrain_coord) */
    std::multimap<terrain_coord, terrain_edit> modifications;
    int                                        state;
    int                                        start_x;
    int                                        start_y;
    bool                                       reached_limit;

    terrain_transaction()
    {
        this->state = TERRAIN_TRANSACTION_EMPTY;
        this->start_x = 0;
        this->start_y = 0;
        this->reached_limit = false;
    }

    void add(terrain_coord coord, terrain_edit edit)
    {
        if (std::abs(coord.chunk_x - start_x) > GENTYPE_MAX_REACH_X) {
            this->reached_limit = true;
            return;
        }
        if (std::abs(coord.chunk_y - start_y) > GENTYPE_MAX_REACH_Y) {
            this->reached_limit = true;
            return;
        }

#ifdef DEBUG_PRELOADER_SANITY
        tms_assertf(std::abs(coord.chunk_x) < 1000 && std::abs(coord.chunk_y) < 1000, "suspicious transaction chunks %d,%d", coord.chunk_x, coord.chunk_y);
#endif

        this->state = TERRAIN_TRANSACTION_READY;

        modifications.insert(std::make_pair(coord, edit));
    }

    void occupy(gentype *gt);
    void apply();
};

/**
 * Comparison of terrain_coords are done by comparing the chunk positions,
 * this allows us to easily sort terrain_coords by chunk and quickly
 * apply a transaction
 * */
bool operator <(const terrain_coord& lhs, const terrain_coord &rhs);
