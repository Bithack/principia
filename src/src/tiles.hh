#pragma once

#include <cstring>
#include <inttypes.h>
#include <deque>
#include <tms/math/vector.h>

enum {
    TILE_NOMAD_HIDEOUT_FULL,
    TILE_NOMAD_HIDEOUT_BROKEN,

    NUM_TILES
};

struct tile_object
{
  public:
    int id;
    tvec2 position;
};

class tilemap
{
  public:
    int width;
    int height;
    int *layers[3];

    std::deque<struct tile_object> entities;

    tilemap()
    {
        for (int x=0; x<3; ++x) {
            this->layers[x] = 0;
        }
        //memset(this->layers, 0, sizeof(int*)*3);
    }
};

struct tile_load_data {
    const char *path;
    tilemap *tm;
};

class tile_factory
{
  public:
    static struct tile_load_data tiles[NUM_TILES];

    static inline tilemap *get_tilemap(int tile_id)
    {
        return tile_factory::tiles[tile_id].tm;
    }

    static void init(void);
};
