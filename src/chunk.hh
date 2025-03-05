#pragma once

#include "entity.hh"
#include "types.hh"
#include "tpixel.hh"
#include <map>
#include <vector>

#define MAX_CHUNKS (14*12)
#define MAX_VERTS_PER_CHUNK 13824 /* (3*16*16*24) * (3/4) */
#define MAX_GRASS_VERTS_PER_CHUNK MAX_GRASS_PER_CHUNK*4
#define MAX_GRASS_PER_CHUNK (16*NUM_GRASS_PER_PIXEL*3)*2
#define NUM_GRASS_PER_PIXEL 3

#define CAVEVIEW_SIZE 64

#define GENSLOT_SIZE_X 4
#define GENSLOT_SIZE_Y 4

#ifdef DEBUG
/* define these in config.sh or go script */
//#define DEBUG_PRELOADER_SANITY
//#define DEBUG_SPECIFIC_CHUNK
//#define DEBUG_SPECIFIC_CHUNK_PIXELS
#define DEBUG_CHUNK_X 1
#define DEBUG_CHUNK_Y 1
#endif

enum {
    CHUNK_LOAD_MERGED = 0,
    CHUNK_LOAD_PIXELS = 1,
    CHUNK_LOAD_EMPTY  = 2,
};

class world;

class chunk_window;
class cable;
class gentype;
class level_chunk;
struct terrain_coord;

typedef std::map<chunk_pos, level_chunk*> chunk_map;
typedef chunk_map::iterator chunk_iterator;

class level_chunk : public entity
{
  public:
    int  generate_phase;
    int  load_phase; /* 0 = unloaded, 1 = sensor + terrain loaded, 2 = fully loaded */
    bool loaded_neighbours;
    bool modified;
    int pos_x, pos_y;
    int slot;
    uint8_t pixels[3][16][16];

    level_chunk *neighbours[8];

    gentype *genslots[GENSLOT_SIZE_X][GENSLOT_SIZE_Y][2]; /* first slot phase 4, other phase 5*/

    tpixel_desc merged[3][16*16];
    uint8_t num_merged[3];
    int min_merged[3];

    struct tms_entity layer_entities[3];
    struct tms_entity grass_entity[2]; /* first entity is for layer 0+1, second entity layer 2 */

    int num_fixtures, num_dyn_fixtures, num_non_unloadable;
    int unload_ticks;
    int unload_ticks_max;
    bool garbage; /* marked as garbage, can be unloaded */
    int waste_retry_wait; /* if unloading of this chunk failed, we wait a few frames before attemping to unload it again */

    void add_fixture(b2Fixture *fx, entity *owner);
    void remove_fixture(b2Fixture *fx, entity *owner);

    void init_chunk_neighbours();
    void clear_chunk_neighbours();

    const char *get_name(void) { return "Chunk"; }

    void load_neighbours();

    /* TODO: need alternative constructor that is fast and used when we load chunks */
    level_chunk(int x, int y)
        : entity()
    {
        this->unload_ticks_max = 8;
        this->unload_ticks = this->unload_ticks_max;
        this->num_fixtures = 0;
        this->num_dyn_fixtures = 0;
        this->num_non_unloadable = 0;
        this->load_phase = 0;
        this->set_flag(ENTITY_ALLOW_CONNECTIONS, true);
        this->set_flag(ENTITY_IS_STATIC, true);
        this->set_flag(ENTITY_IS_ZAPPABLE, true);
        this->g_id = O_CHUNK;
        this->set_mesh((struct tms_mesh*)0);
        this->set_material(&m_tpixel);
        this->generate_phase = 0;
        this->pos_x = x;
        this->pos_y = y;
        this->slot = -1;
        this->garbage = false;
        this->loaded_neighbours = false;
        memset(this->neighbours, 0, sizeof(this->neighbours));

        for (int x=0; x<3; x++) {
            tms_entity_init(&this->layer_entities[x]);
            tms_entity_set_mesh(&this->layer_entities[x], tms_mesh_alloc(0, 0));
            tms_entity_set_material(&this->layer_entities[x], &m_tpixel);
            tmat3_load_identity(this->layer_entities[x].N);
            this->layer_entities[x].prio = x;
            this->num_merged[x] = 0;
            this->min_merged[x] = 0;
        }
        for (int x=0; x<2; x++) {
            tms_entity_init(&this->grass_entity[x]);
            tms_entity_set_material(&this->grass_entity[x], &m_grass);
            tms_entity_set_mesh(&this->grass_entity[x], tms_mesh_alloc(0,0));
            this->grass_entity[x].prio = x*2;
            tms_entity_add_child(&this->layer_entities[x*2], &this->grass_entity[x]);
        }

        memset(pixels, 0, sizeof(pixels));
        //memset(merged, 0, sizeof(merged));
        memset(this->genslots, 0, sizeof(this->genslots));
    }

    ~level_chunk();

    bool find_ground(terrain_coord *in, int layer, terrain_coord *out, float *heights=0, int tolerance=1);

    bool is_focused();

    inline chunk_pos get_chunk_pos()
    {
        return chunk_pos(this->pos_x, this->pos_y);
    }

    void recreate_fixtures(bool initial);

    bool occupy_pixel(int local_x, int local_y, gentype *gt);

    void set_pixel(int x, int y, int z, int material)
    {
        this->modified = true;
        this->pixels[z][y][x] = material;
    }

    int get_pixel(int x, int y, int z)
    {
        return this->pixels[z][y][x];
    }

    uint32_t get_fixture_connection_data(b2Fixture *f);

    void on_touch(b2Fixture *my, b2Fixture *other);
    void on_untouch(b2Fixture *my, b2Fixture *other);
    void on_paused_touch(b2Fixture *my, b2Fixture *other);
    void on_paused_untouch(b2Fixture *my, b2Fixture *other);

    void make_smooth(chunk_window *win);

    void generate(chunk_window *win, int up_to_phase=5);
    void generate_phase1(chunk_window *win);
    void generate_phase2(chunk_window *win);
    void generate_phase3(chunk_window *win);
    void generate_phase4(chunk_window *win);
    void generate_phase5(chunk_window *win);
    void generate_phase6(chunk_window *win);

    /* phase 3 */
    void generate_vegetation(float *heights);
    void generate_caves(float *heights);

    /* phase 4+5 */
    void apply_gentypes(int sorting);

    void merge(int _x, int _y, int _z, int _w, int _h, int _d);
    void remerge();

    void update_pixel_buffer();
    void update_heights();

    void add_to_world();
    void remove_from_world();
};

struct preload_info
{
    bool   heap;
    size_t ptr;
    size_t size;
    preload_info(size_t p, size_t s, bool heap=false){
        this->ptr = p;
        this->size = s;
        this->heap = heap;
    };
};

class chunk_preloader
{
  protected:
    lvlbuf w_lb; /* buffer of current level */
    lvlbuf heap;
    int version; /* version of current level */
    uint32_t adventure_id;
    uint64_t flags;

    std::multimap<chunk_pos, uint32_t>          entities_by_chunk;
    std::map<uint32_t,      preload_info>       groups;
    std::map<uint32_t,      preload_info>       entities;
    std::map<uint32_t,      preload_info>       cables;
    std::map<chunk_pos,     preload_info>       chunks;
    std::multimap<uint32_t, uint32_t>           cable_rels; /* e_id -> cable_id */
    std::map<size_t,        preload_info>       connections; /* ptr -> conn */
    std::multimap<uint32_t, size_t>             connection_rels; /* e_id/o_id -> ptr */
    std::map<uint32_t,      gentype*>           gentypes;

    /* temporary buffers until push_loaded() is called */
    std::map<uint32_t, entity*> loaded_entities;
    std::map<uint32_t, group*>  loaded_groups;
    std::set<connection*>       loaded_connections;
    std::set<cable*>            loaded_cables;
    std::vector<chunk_pos>      affected_chunks; /* list of chunks that have been affected and must be loaded */

    chunk_map active_chunks; /* chunks loaded and being simulated, either for being inside the viewport or surrounding an active chunk */
    chunk_map wastebin; /* chunks to be unloaded */

    level_chunk* get_chunk(int x, int y, bool soft/*=false*/, bool load=true);

    /* keep these in sync with of::read */
    void preload_group();
    void preload_entity();
    void preload_cable();
    void preload_connection();
    group *read_group(preload_info i);
    entity *read_entity(preload_info i);
    cable *read_cable(preload_info i);
    connection *read_connection(size_t conn_id);
    level_chunk *read_chunk(preload_info i);

    void write_chunk(lvlinfo *lvl, lvlbuf *lb, level_chunk *c);

  public:
    chunk_preloader();
    ~chunk_preloader();
    void reset();
    void clear_chunks();
    bool preload(lvlinfo *lvl, lvlbuf *lb);

    void require_chunk_neighbours(level_chunk *c);

    void prepare_write();

    void    unwaste_chunk(level_chunk *c);
    bool    unload(level_chunk *c);
    void    load(level_chunk *c, int phase);
    void    push_loaded();
    group  *load_group(uint32_t id);
    entity *load_entity(uint32_t id);
    cable *load_cable(uint32_t id);

    /* write unloaded stuff */
    void write_groups(lvlinfo *lvl, lvlbuf *lb);
    void write_entities(lvlinfo *lvl, lvlbuf *lb);
    void write_cables(lvlinfo *lvl, lvlbuf *lb);
    void write_connections(lvlinfo *lvl, lvlbuf *lb);
    void write_chunks(lvlinfo *lvl, lvlbuf *lb);
    void write_gentypes(lvlinfo *lvl, lvlbuf *lb);

    void read_chunks(lvlinfo *lvl, lvlbuf *lb);
    void read_gentypes(lvlinfo *lvl, lvlbuf *lb);

    const std::map<chunk_pos, level_chunk*>& get_active_chunks();

    friend class world;
    friend class chunk_window;
    friend class game;
    friend class gentype;
    friend class level_chunk;
};


class chunk_window : public tms::entity
{
  public:
    bool isset;
    int x, y, w, h;
    uint64_t seed;
    struct tms_texture               *caveview;
    chunk_preloader                   preloader;
    level_chunk                      *slots[MAX_CHUNKS];
    std::map<int, float*>             heightmap;

    chunk_window();
    const char *get_name() { return "Chunk window"; }
    void reset();
    void step();
    inline level_chunk *get_chunk(int x, int y, bool soft=false, bool load=true)
    {
        return this->preloader.get_chunk(x,y,soft,load);
    }
    level_chunk *set_pixel(int gx, int gy, int z, int material);
    int get_pixel(int gx, int gy, int z);

    float get_height(float x);
    float *get_heights(int x, bool must_be_valid=false);
    float *generate_heightmap(int chunk_x, bool search=true);
    void set_seed(uint64_t seed);
    int find_slot(){for (int x=0; x<MAX_CHUNKS; x++) {if (!this->slots[x]) return x;}; return -1;};
    void load_slot(int s, level_chunk *c);
    void unload_slot(int s);
    void set(b2Vec2 lower, b2Vec2 upper);

    void recreate_caveview_texture(float px, float py, float ax, float ay);
};
