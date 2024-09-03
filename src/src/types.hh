#pragma once

#include <stdint.h>

class entity;

struct chunk_pos {
    int x, y;

    chunk_pos(int x, int y) {
        this->x = x;
        this->y = y;
    };
};

struct chunk_intersection {
    int x;
    int y;
    int num_fixtures;
};

struct genslot
{
    int chunk_x;
    int chunk_y;
    uint8_t slot_x;
    uint8_t slot_y;
    uint8_t sorting;

    genslot(){};
    genslot(int a, int b, int c, int d, int e)
    {
        chunk_x = a; chunk_y = b;
        slot_x = (uint8_t)c; slot_y = (uint8_t)d;
        sorting = (uint8_t)e;
    }
};

bool operator <(const chunk_pos& lhs, const chunk_pos &rhs);
bool operator <(const genslot& lhs, const genslot &rhs);
bool operator ==(const genslot& lhs, const genslot &rhs);

typedef ::entity p_entity;
typedef uint32_t p_id;
typedef uint8_t p_gid;

enum {
    DAMAGE_SOURCE_WORLD,
    DAMAGE_SOURCE_BULLET
};

enum damage_type {
    DAMAGE_TYPE_FORCE,
    DAMAGE_TYPE_PLASMA,
    DAMAGE_TYPE_ELECTRICITY,
    DAMAGE_TYPE_OTHER,
    DAMAGE_TYPE_BLUNT, /* hammers */
    DAMAGE_TYPE_SHARP, /* swords */
    DAMAGE_TYPE_HEAVY_SHARP, /* axes */

    NUM_DAMAGE_TYPES
};

enum {
    TERRAIN_EMPTY           = 0,
    TERRAIN_GRASS           = 1,
    TERRAIN_DIRT            = 2,
    TERRAIN_STONE           = 3,
    TERRAIN_HEAVY_STONE     = 4,

    NUM_TERRAIN_MATERIALS
};
