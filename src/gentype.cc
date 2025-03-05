#include "gentype.hh"
#include "game.hh"
#include "object_factory.hh"
#include "world.hh"
#include "animal.hh"
#include "item.hh"
#include "faction.hh"
#include "robot.hh"
#include "noise.h"
#include "ladder.hh"
#include "tiles.hh"
#include "factory.hh"

/**
 * NOTES copies from terrain.cc
 *
 * random by chunk x:
 * faction bases
 * mines
 * nomad hideouts
 * master base
 * jail
 * house
 * special plants
 *
 * minor things possibly related to another generated thing:
 * - warning signs
 * - dead robots, loose parts
 * - wells, holes
 * - farms
 *
 * random by position:
 * perlin caves, material, height, roughness, etc
 *
 * treasures and stuff:
 * - in nomad hideouts:
 *   - gemstones
 *   - weapons
 *   - oils, barrels
 *   - useful objects
 * - in bases
 *   - weapons
 *   - scrap, robot parts
 *   - factories
 * - in mines
 *   - materials and oils
 *
 * entry types
 * an entry is an opening from the ground into a mine, underground base or hideout
 * - hatch
 * - overgrown opening
 * - buried opening
 * - opening
 * - opening with guard
 *
 * moar
 * - fireflies (glowing)
 * - butterflies
 * - robot eating enemies
 *   - small thing that looks like a stone
 *   - big robot eater
 *
 **/

uint32_t _gentype_id = 1;

gentype::~gentype()
{
#ifdef DEBUG
    tms_debugf("clearing those genslots for %p! (loaded:%d, count:%d, sorting:%d)", this, this->loaded, (int)this->genslots.size(), this->sorting);
#endif

#if 0
    {
        /* for easier debugging */
        std::sort(this->genslots.begin(), this->genslots.end());

    }
#endif

    for (std::vector<genslot>::iterator it = this->genslots.begin();
            it != this->genslots.end(); ++it) {
        genslot g = *it;

        level_chunk *c = W->cwindow->get_chunk(g.chunk_x, g.chunk_y, false, false);

#ifdef DEBUG_SPECIFIC_CHUNK
        if (g.chunk_x == DEBUG_CHUNK_X && g.chunk_y == DEBUG_CHUNK_Y) {
            tms_debugf("clearing genslot in %d %d", DEBUG_CHUNK_X, DEBUG_CHUNK_Y);
        }
#endif

        if (c) {
            if (c->genslots[g.slot_x][g.slot_y][g.sorting] == this) {
                tms_assertf(c->generate_phase < (5+g.sorting), "this should never happen!");
                c->genslots[g.slot_x][g.slot_y][g.sorting] = 0;
            }
        }
#ifdef DEBUG
        else {
            tms_warnf("could not find level chunk to remove my genslot!");
        }
#endif
    }

    if (!this->lock) {
        W->cwindow->preloader.gentypes.erase(this->id);
    } else {
        tms_debugf("i'm locked :(");
    }
}

const struct terrain_edit default_tileset[] =
{
    terrain_edit(0, 0), /* 0, do nothing! */

    terrain_edit(0, TERRAIN_EMPTY),
    terrain_edit(0, TERRAIN_GRASS),
    terrain_edit(0, TERRAIN_DIRT),
    terrain_edit(0, TERRAIN_STONE),
    terrain_edit(0, TERRAIN_HEAVY_STONE),
    terrain_edit(0, 0), /* 6, unused */
    terrain_edit(0, 0), /* 7, unused */
    terrain_edit(0, 0), /* 8, unused */

    terrain_edit(TERRAIN_EDIT_SOFT, TERRAIN_EMPTY),
    terrain_edit(TERRAIN_EDIT_SOFT, TERRAIN_GRASS),
    terrain_edit(TERRAIN_EDIT_SOFT, TERRAIN_DIRT),
    terrain_edit(TERRAIN_EDIT_SOFT, TERRAIN_STONE),
    terrain_edit(TERRAIN_EDIT_SOFT, TERRAIN_HEAVY_STONE),
    terrain_edit(0, 0), /* 14, unused */
    terrain_edit(0, 0), /* 15, unused */
    terrain_edit(0, 0), /* 16, unused */

    terrain_edit(TERRAIN_EDIT_HARDEN | TERRAIN_EDIT_NONEMPTY | TERRAIN_EDIT_INC, TERRAIN_EMPTY),
    terrain_edit(TERRAIN_EDIT_HARDEN | TERRAIN_EDIT_NONEMPTY | TERRAIN_EDIT_INC, TERRAIN_GRASS),
    terrain_edit(TERRAIN_EDIT_HARDEN | TERRAIN_EDIT_NONEMPTY | TERRAIN_EDIT_INC, TERRAIN_DIRT),
    terrain_edit(TERRAIN_EDIT_HARDEN | TERRAIN_EDIT_NONEMPTY | TERRAIN_EDIT_INC, TERRAIN_STONE),
    terrain_edit(TERRAIN_EDIT_HARDEN | TERRAIN_EDIT_NONEMPTY | TERRAIN_EDIT_INC, TERRAIN_HEAVY_STONE),
    terrain_edit(0, 0), /* 22, unused */
    terrain_edit(0, 0), /* 23, unused */
    terrain_edit(0, 0), /* 24, unused */

    terrain_edit(TERRAIN_EDIT_SOFTEN | TERRAIN_EDIT_NONEMPTY | TERRAIN_EDIT_DEC, TERRAIN_EMPTY),
    terrain_edit(TERRAIN_EDIT_SOFTEN | TERRAIN_EDIT_NONEMPTY | TERRAIN_EDIT_DEC, TERRAIN_GRASS),
    terrain_edit(TERRAIN_EDIT_SOFTEN | TERRAIN_EDIT_NONEMPTY | TERRAIN_EDIT_DEC, TERRAIN_DIRT),
    terrain_edit(TERRAIN_EDIT_SOFTEN | TERRAIN_EDIT_NONEMPTY | TERRAIN_EDIT_DEC, TERRAIN_STONE),
    terrain_edit(TERRAIN_EDIT_SOFTEN | TERRAIN_EDIT_NONEMPTY | TERRAIN_EDIT_DEC, TERRAIN_HEAVY_STONE),
    terrain_edit(0, 0), /* 30, unused */
    terrain_edit(0, 0), /* 31, unused */
    terrain_edit(0, 0), /* 32, unused */

    terrain_edit(TERRAIN_EDIT_NONEMPTY, TERRAIN_EMPTY),
    terrain_edit(TERRAIN_EDIT_NONEMPTY, TERRAIN_GRASS),
    terrain_edit(TERRAIN_EDIT_NONEMPTY, TERRAIN_DIRT),
    terrain_edit(TERRAIN_EDIT_NONEMPTY, TERRAIN_STONE),
    terrain_edit(TERRAIN_EDIT_NONEMPTY, TERRAIN_HEAVY_STONE),
    terrain_edit(0, 0), /* 14, unused */
    terrain_edit(0, 0), /* 15, unused */
    terrain_edit(0, 0), /* 16, unused */
};

const int NUM_DEFAULT_TILES = sizeof(default_tileset)/sizeof(default_tileset[0]);

static inline bool is_surface_level(int pos_y, float *heights)
{
    return ((pos_y+1)*8.f > heights[7] && (pos_y)*8.f < heights[7]);
}

static inline bool check_depth_range(int chunk_y, float *heights, float min_y, float max_y)
{
    float h = heights[7];
    float b = chunk_y*8.f;
    float t = (chunk_y+1)*8.f;

    min_y += h;
    max_y += h;

    return (b > min_y && t < max_y);
}

struct enemy_random_equipment
{
    uint32_t item_id;
    int prob;
};

static struct enemy_random_equipment enemy_weapons[] = {
    {
        ITEM_ARM_CANNON,
        1,
    }, {
        ITEM_SHOTGUN,
        7,
    }, {
        ITEM_RAILGUN,
        25,
    }, {
        ITEM_ROCKET_LAUNCHER,
        15,
    }, {
        ITEM_BOMBER,
        14,
    }, {
        ITEM_TESLAGUN,
        20,
    }, {
        ITEM_PLASMAGUN,
        25,
    }, {
        ITEM_MEGABUSTER,
        40,
    }, {
        ITEM_TRAINING_SWORD,
        7,
    }, {
        ITEM_HAMMER,
        15,
    }, {
        ITEM_SIMPLE_AXE,
        10,
    }, {
        ITEM_SPIKED_CLUB,
        15,
    }, {
        ITEM_STEEL_SWORD,
        15,
    }, {
        ITEM_BASEBALLBAT,
        12,
    }, {
        ITEM_WAR_AXE,
        35,
    }, {
        ITEM_PIXEL_SWORD,
        30,
    }, {
        ITEM_SERPENT_SWORD,
        30,
    }, {
        ITEM_PICKAXE,
        10,
    }
};

static struct enemy_random_equipment enemy_tools[] = {
    {
        ITEM_ZAPPER,
        1,
    }, {
        ITEM_BUILDER,
        10,
    }, {
        ITEM_FACTION_WAND,
        60,
    }
};

static struct enemy_random_equipment enemy_front_equipments[] = {
    {
        FRONT_EQUIPMENT_ROBOT_FRONT,
        1,
    }, {
        FRONT_EQUIPMENT_BLACK_ROBOT_FRONT,
        5,
    }, {
        FRONT_EQUIPMENT_PIONEER_FRONT,
        10,
    }
};

static struct enemy_random_equipment enemy_back_equipments[] = {
    {
        BACK_EQUIPMENT_ROBOT_BACK,
        1,
    }, {
        BACK_EQUIPMENT_BLACK_ROBOT_BACK,
        5,
    }, {
        BACK_EQUIPMENT_JETPACK,
        30,
    }, {
        BACK_EQUIPMENT_UPGRADED_JETPACK,
        60,
    }, {
        BACK_EQUIPMENT_ADVANCED_JETPACK,
        150,
    }, {
        BACK_EQUIPMENT_PIONEER_BACK,
        10,
    }
};

static struct enemy_random_equipment enemy_heads[] = {
    {
        HEAD_ROBOT,
        1,
    }, {
        HEAD_ROBOT_UNCOVERED,
        10,
    }, {
        HEAD_DUMMY,
        30,
    }, {
        HEAD_OSTRICH,
        200,
    }
};

static struct enemy_random_equipment enemy_head_equipments[] = {
    {
        HEAD_EQUIPMENT_NULL,
        1,
    }, {
        HEAD_EQUIPMENT_NINJAHELMET,
        20,
    }, {
        HEAD_EQUIPMENT_HEISENBERG,
        25,
    }, {
        HEAD_EQUIPMENT_WIZARDHAT,
        40,
    }, {
        HEAD_EQUIPMENT_CONICALHAT,
        30,
    }, {
        HEAD_EQUIPMENT_POLICEHAT,
        30,
    }, {
        HEAD_EQUIPMENT_KINGSCROWN,
        100,
    }, {
        HEAD_EQUIPMENT_WITCH_HAT,
        40,
    }, {
        HEAD_EQUIPMENT_JESTERHAT,
        60,
    }, {
        HEAD_EQUIPMENT_HARD_HAT,
        30,
    }, {
        HEAD_EQUIPMENT_VIKING_HELMET,
        30,
    }
};

static struct enemy_random_equipment enemy_bolt_sets[] = {
    {
        BOLT_SET_STEEL,
        1,
    }, {
        BOLT_SET_WOOD,
        40,
    }, {
        BOLT_SET_SAPPHIRE,
        90,
    }, {
        BOLT_SET_DIAMOND,
        150,
    }
};

static struct enemy_random_equipment enemy_feet[] = {
    {
        FEET_BIPED,
        1,
    }, {
        FEET_MINIWHEELS,
        8,
    }, {
        FEET_MONOWHEEL,
        7,
    }
};

static const int NUM_ENEMY_WEAPONS = sizeof(enemy_weapons) / sizeof(enemy_weapons[0]);
static const int NUM_ENEMY_TOOLS = sizeof(enemy_tools) / sizeof(enemy_tools[0]);
static const int NUM_ENEMY_FRONT_EQUIPMENTS = sizeof(enemy_front_equipments) / sizeof(enemy_front_equipments[0]);
static const int NUM_ENEMY_BACK_EQUIPMENTS = sizeof(enemy_back_equipments) / sizeof(enemy_back_equipments[0]);
static const int NUM_ENEMY_HEADS = sizeof(enemy_heads) / sizeof(enemy_heads[0]);
static const int NUM_ENEMY_HEAD_EQUIPMENTS = sizeof(enemy_head_equipments) / sizeof(enemy_head_equipments[0]);
static const int NUM_ENEMY_BOLT_SETS = sizeof(enemy_bolt_sets) / sizeof(enemy_bolt_sets[0]);
static const int NUM_ENEMY_FEET = sizeof(enemy_feet) / sizeof(enemy_feet[0]);

static int
randomize_equipment(struct enemy_random_equipment *equipments, int num_equipments)
{
    int index = 0;
    for (int n = num_equipments-1; n != 1; --n) {
        if (rand()%equipments[n].prob == 0) {
            index = n;
            break;
        }
    }

    return equipments[index].item_id;
}

entity*
gentype::create_enemy(int robot_type, b2Vec2 pos, int layer, int faction)
{
    robot_base *e;

    switch (robot_type) {
        case ROBOT_TYPE_ROBOT:
            {
                char tmp[128];
                e = static_cast<robot*>(of::create(O_ROBOT));

                int weapon_id = randomize_equipment(enemy_weapons, NUM_ENEMY_WEAPONS);
                int tool_id = randomize_equipment(enemy_tools, NUM_ENEMY_TOOLS);

                snprintf(tmp, 127, "%d;%d", tool_id, weapon_id);
                e->set_property(7, tmp);

                {
                    int id = randomize_equipment(enemy_front_equipments, NUM_ENEMY_FRONT_EQUIPMENTS);
                    e->properties[ROBOT_PROPERTY_FRONT].v.i8 = id;
                }
                {
                    int id = randomize_equipment(enemy_back_equipments, NUM_ENEMY_BACK_EQUIPMENTS);
                    e->properties[ROBOT_PROPERTY_BACK].v.i8 = id;
                }
                {
                    int id = randomize_equipment(enemy_heads, NUM_ENEMY_HEADS);
                    e->properties[ROBOT_PROPERTY_HEAD].v.i8 = id;
                }
                {
                    int id = randomize_equipment(enemy_head_equipments, NUM_ENEMY_HEAD_EQUIPMENTS);
                    e->properties[ROBOT_PROPERTY_HEAD_EQUIPMENT].v.i8 = id;
                }
                {
                    int id = randomize_equipment(enemy_bolt_sets, NUM_ENEMY_BOLT_SETS);
                    e->properties[ROBOT_PROPERTY_BOLT_SET].v.i8 = id;
                }
                {
                    int id = randomize_equipment(enemy_feet, NUM_ENEMY_FEET);
                    e->properties[ROBOT_PROPERTY_FEET].v.i8 = id;
                }
            }
            break;

        case ROBOT_TYPE_SPIKEBOT: e = static_cast<robot*>(of::create(O_SPIKEBOT)); break;
        case ROBOT_TYPE_COMPANION: e = static_cast<robot*>(of::create(O_COMPANION)); break;
        case ROBOT_TYPE_BOMBER: e = static_cast<robot*>(of::create(O_BOMBER)); break;
        case ROBOT_TYPE_LOBBER: e = static_cast<robot*>(of::create(O_LOBBER)); break;
        case ROBOT_TYPE_MINIBOT: e = static_cast<robot*>(of::create(O_MINIBOT)); break;
        default:return 0;
    }

    e->set_layer(1);
    e->_pos = pos;
    e->_angle = 0.f;
    e->set_faction(faction);
    e->properties[ROBOT_PROPERTY_ROAMING].v.i8 = true;

    this->entities.insert(std::make_pair(e->id, e));
    return e;
}

class nomad_hideout : public gentype
{
  private:
    float variant;
    terrain_coord entity_coord;

    nomad_hideout(terrain_coord coord, float priority)
        : gentype(coord, priority)
    {
        this->variant = std::abs(_noise2(coord.get_world_x(), coord.get_world_y()));
    }

    nomad_hideout() : gentype()
    {

    }

    const tilemap* get_tilemap() const
    {
        int tile_id;

        if (this->variant < 0.2f) {
            tile_id = TILE_NOMAD_HIDEOUT_FULL;
        } else {
            tile_id = TILE_NOMAD_HIDEOUT_BROKEN;
        }

        return tile_factory::get_tilemap(tile_id);
    }

  public:
    void write_state(lvlinfo *lvl, lvlbuf *lb){tms_fatalf("unimplemented");};
    void read_state(lvlinfo *lvl, lvlbuf *lb){tms_fatalf("unimplemented");};
    static gentype* allocate(){return new nomad_hideout();};

    static gentype*
    occupy(struct gentype_data data)
    {
        if (true) {
            float wx = data.coord.get_world_x();
            float wy = data.coord.get_world_y();
            nomad_hideout *gt = new nomad_hideout(data.coord, _noise2(wx, wy));

            terrain_edit  clear(TERRAIN_EDIT_LAYER1, TERRAIN_EMPTY);
            terrain_edit  grass(TERRAIN_EDIT_LAYER1, TERRAIN_GRASS);

            /* Harden the material by one step, to a maximum material of stone */
            terrain_edit inc(
                    TERRAIN_EDIT_LAYER1
                   |TERRAIN_EDIT_HARDEN
                   |TERRAIN_EDIT_NONEMPTY
                   |TERRAIN_EDIT_INC,
                    TERRAIN_STONE);

            const tilemap *tm = gt->get_tilemap();
            if (!tm) return 0;

            const int ox = -tm->width/2.f;
            const int oy = 0;

            struct terrain_coord c = data.coord;

            c.step(ox, oy);

            gt->entity_coord = c;

            terrain_coord cc = c;

            for (int z=0; z<3; ++z) {
                const int layer_mask = TERRAIN_EDIT_LAYER0<<z;

                if (!tm->layers[z]) continue;

                for (int y=0; y<tm->height; y++) {
                    for (int x=0; x<tm->width; x++) {
                        int v = tm->layers[z][y*tm->width+x];
                        switch (v) {
                            case 0:
                                /* do nothing */
                                break;
                            default:
                                if (v > 0 && v < NUM_DEFAULT_TILES) {
                                    gt->transaction.add(c, terrain_edit(layer_mask | default_tileset[v].flags, default_tileset[v].data));
                                } else {
                                    tms_warnf("unknown tile id (%d)", v);
                                }
                                break;
                        }
                        c.step(1, 0);
                    }
                    c.step(-tm->width, -1);
                }

                c = cc;
            }

            return gt;
        }

        return 0;
    }

    void create_entities()
    {
        const tilemap *tm = this->get_tilemap();
        if (!tm) return;

        float wx = this->entity_coord.get_world_x();
        float wy = this->entity_coord.get_world_y();

        if (!tm->entities.empty()) {
            for (std::deque<struct tile_object>::const_iterator it = tm->entities.begin();
                    it != tm->entities.end(); ++it) {
                const struct tile_object &ed = *it;

                entity *e = of::create(ed.id);
                e->set_layer(1);
                e->_pos = b2Vec2(wx+(ed.position.x/2.0f), wy-(ed.position.y/2.0f));
                e->_angle = 0.f;
                e->set_color(TV_BLACK);
                this->entities.insert(std::make_pair(e->id, e));

                tms_infof("Adding entity with g_id %d at %.2f/%.2f (file coordinates: %.2f/%.2f)",
                        ed.id,
                        e->_pos.x, e->_pos.y,
                        ed.position.x, ed.position.y);
            }
        }

        /*
        if (this->variant < .2f) {
            {
                entity *e = of::create(O_PLASTIC_BOX);
                e->set_layer(1);
                e->_pos = b2Vec2(wx+13.f, wy-14.f);
                e->_angle = 0.f;
                e->set_color(TV_BLACK);
                this->entities.insert(std::make_pair(e->id, e));
            }

            {
                spikebot *r = static_cast<spikebot*>(of::create(O_SPIKEBOT));
                r->set_layer(1);
                r->_pos = b2Vec2(wx+11.f, wy-14.f);
                r->_angle = 0.f;
                r->set_color(TV_BLACK);
                this->entities.insert(std::make_pair(r->id, r));
            }
        } else {
            {
                entity *e = of::create(O_PLASTIC_BOX);
                e->set_layer(1);
                e->_pos = b2Vec2(wx, wy-12.f);
                e->_angle = 0.f;
                e->set_color(TV_WHITE);
                this->entities.insert(std::make_pair(e->id, e));
            }
        }
        */
    }
};

static struct atype {
    int animal_type;
    int min_count;
    int max_count;
    int prob;
} atypes[NUM_ANIMAL_TYPES] = {
    {
        ANIMAL_TYPE_COW,
        1, 7,
        2,
    }, {
        ANIMAL_TYPE_PIG,
        2, 8,
        3,
    }, {
        ANIMAL_TYPE_OSTRICH,
        1, 2,
        2,
    },
};

class animals : public gentype
{
  private:
    animals(terrain_coord coord, float priority)
        : gentype(coord, priority)
    {
        this->sorting = 1; /* apply this gentype after other gentypes */
    }
    animals(){};

    int animal_type;
    int count;
    int spacing;

  public:
    void write_state(lvlinfo *lvl, lvlbuf *lb)
    {
        lb->w_s_int32(animal_type);
        lb->w_s_int32(count);
        lb->w_s_int32(spacing);
    };

    void read_state(lvlinfo *lvl, lvlbuf *lb)
    {
        this->animal_type = lb->r_int32();
        this->count = lb->r_int32();
        this->spacing = lb->r_int32();
    };

    static gentype* allocate(){return new animals();};
    static gentype*
    occupy(struct gentype_data data)
    {
        int animal_type = -1;

        for (int x=0; x<NUM_ANIMAL_TYPES; x++) {
            if (atypes[x].prob && rand()%atypes[x].prob == 0) {
                animal_type = atypes[x].animal_type;
                break;
            }
        }

        if (animal_type != -1) {
            animals *gt = new animals(data.coord, 1.f);

            gt->animal_type = animal_type;
            gt->count = atypes[gt->animal_type].min_count + rand()%(atypes[gt->animal_type].max_count-atypes[gt->animal_type].min_count);

            gt->spacing = 3;
            terrain_edit  e(TERRAIN_EDIT_TOUCH, 0);
            terrain_coord c = data.coord;
            c.step(-(gt->count*gt->spacing*2)/2, 0);

            for (int x=0; x<gt->count; x++) {
                gt->transaction.add(c, e);
                c.step(gt->spacing*2, 0);
            }

            return gt;
        }

        return 0;
    }

    void create_entities()
    {
        for (int x=0; x<this->count; x++) {
            animal *e = static_cast<animal*>(of::create(O_ANIMAL));
            e->set_layer(1);

            terrain_coord c = this->coord;
            c.step(-(count*spacing*2)/2, 0);
            c.step(x*spacing*2, 0);
            terrain_coord ground;
            level_chunk *chunk = W->cwindow->get_chunk(c.chunk_x, c.chunk_y);

            if (chunk->find_ground(&c, 1, &ground, 0, 1)) {
                e->_pos = b2Vec2(ground.get_world_x(), ground.get_world_y()+1.f);
                e->_angle = 0.f;
                e->set_animal_type(this->animal_type);
                e->properties[1].v.f = .35f+(rand()%100/100.f)*.65f;

                this->entities.insert(std::make_pair(e->id, e));
            }
        }
    }
};

static struct etype {
    uint16_t enemy_type;
    int min_count;
    int max_count;
    int prob;
} etypes[] = {
    {
        ROBOT_TYPE_ROBOT,
        1, 7,
        4,
    }, {
        ROBOT_TYPE_SPIKEBOT,
        2, 4,
        7,
    }, {
        ROBOT_TYPE_SPIKEBOT,
        4, 8,
        22,
    }, {
        ROBOT_TYPE_COMPANION,
        7, 10,
        100,
    }, {
        ROBOT_TYPE_BOMBER,
        1, 2,
        10,
    }, {
        ROBOT_TYPE_LOBBER,
        1, 2,
        9,
    }, {
        ROBOT_TYPE_MINIBOT,
        2, 4,
        75,
    },
};

static int NUM_ENEMY_TYPES = sizeof(etypes) / sizeof(etypes[0]);

class enemies : public gentype
{
  private:
    enemies(terrain_coord coord, float priority)
        : gentype(coord, priority)
    {
        this->sorting = 1; /* apply this gentype after other gentypes */
    }
    enemies(){};

    uint16_t enemy_type;
    int count;
    int spacing;

  public:
    void write_state(lvlinfo *lvl, lvlbuf *lb)
    {
        lb->w_s_uint16(enemy_type);
        lb->w_s_int32(count);
        lb->w_s_int32(spacing);
    };

    void read_state(lvlinfo *lvl, lvlbuf *lb)
    {
        this->enemy_type = lb->r_uint16();
        this->count = lb->r_int32();
        this->spacing = lb->r_int32();
    };

    static gentype* allocate(){return new enemies();};
    static gentype*
    occupy(struct gentype_data data)
    {
        if (abs(data.coord.chunk_x) < 2) {
            return 0;
        }

        int enemy_index = -1;

        for (int x=0; x<NUM_ENEMY_TYPES; x++) {
            if (etypes[x].prob && rand()%etypes[x].prob == 0) {
                enemy_index = x;
                break;
            }
        }

        if (enemy_index != -1) {
            enemies *gt = new enemies(data.coord, 1.f);

            struct etype &et = etypes[enemy_index];

            gt->enemy_type = et.enemy_type;
            gt->count = et.min_count + rand()%(et.max_count-et.min_count);

            gt->spacing = 3;
            terrain_edit  e(TERRAIN_EDIT_TOUCH, 0);
            terrain_coord c = data.coord;
            c.step(-(gt->count*gt->spacing*2)/2, 0);

            for (int x=0; x<gt->count; x++) {
#ifdef DEBUG_PRELOADER_SANITY
                tms_assertf(abs(c.chunk_y) < 1000.f && abs(c.chunk_x) < 1000.f,
                        "suspicious chunk pos");
#endif
                gt->transaction.add(c, e);
                c.step(gt->spacing*2, 0);
            }

            return gt;
        }

        return 0;
    }

    void create_entities()
    {
        for (int x=0; x<this->count; x++) {
            /* XXX: use create_enemy here */

            terrain_coord c = this->coord;
            c.step(-(count*spacing*2)/2, 0);
            c.step(x*spacing*2, 0);
            terrain_coord ground;
            level_chunk *chunk = W->cwindow->get_chunk(c.chunk_x, c.chunk_y);

            if (chunk->find_ground(&c, 1, &ground, 0, 1)) {
                b2Vec2 pos(ground.get_world_x(), ground.get_world_y()+1.f);
                this->create_enemy(this->enemy_type, pos, 1, FACTION_ENEMY);
            }
        }
    }
};

class linegen : public gentype
{
  protected:
    linegen(terrain_coord coord, float priority)
        : gentype(coord, priority)
    {
    };
    linegen(){};

    void write_pixel(int x, int y, int d)
    {
        terrain_coord c;
        terrain_edit e(TERRAIN_EDIT_LAYER1, TERRAIN_EMPTY);
        c.set_from_global(x,y);
        this->transaction.add(c, e);
    }

    void draw_line(int x0, int y0, int x1, int y1, float w)
    {
        terrain_coord c;

        int dx = abs(x1-x0), sx = x0 < x1 ? 1 : -1;
        int dy = abs(y1-y0), sy = y0 < y1 ? 1 : -1;
        int err = dx-dy, e2, x2, y2;
        float ed = dx+dy == 0 ? 1 : sqrt((float)dx*dx+(float)dy*dy);

        for (w = (w+1)/2; ; ) {
            this->write_pixel(x0,y0, std::max(0.f,255*(abs(err-dx+dy)/ed-w+1)));
            e2 = err;
            x2 = x0;

            if (2*e2 >= -dx) {
                for (e2 += dy, y2 = y0; e2 < ed*w && (y1 != y2 || dx > dy); e2 += dx) {
                    this->write_pixel(x0, y2 += sy, std::max(0.f,255*(abs(e2)/ed-w+1)));
                }
                if (x0 == x1) {
                    break;
                }
                e2 = err;
                err -= dy;
                x0 += sx;
            }

            if (2*e2 <= dy) {
                for (e2 = dx-e2; e2 < ed*w && (x1 != x2 || dx < dy); e2 += dy) {
                    this->write_pixel(x2 += sx, y0, std::max(0.f,255*(abs(e2)/ed-w+1)));
                }
                if (y0 == y1) {
                    break;
                }
                err += dx;
                y0 += sy;
            }
        }
    }

  public:
    void create_entities()=0;
};

class mine : public linegen
{
  private:
    mine(terrain_coord coord, float priority)
        : linegen(coord, priority)
    {
    }
    mine(){};

    std::vector<b2Vec2> ladders;
    std::vector<b2Vec2> storages;
    std::vector<b2Vec2> enemies;

  public:
    void write_state(lvlinfo *lvl, lvlbuf *lb)
    {
        lb->w_s_uint32(this->ladders.size());
        lb->w_s_uint32(this->storages.size());
        lb->w_s_uint32(this->enemies.size());

        for (std::vector<b2Vec2>::iterator i = ladders.begin(); i != ladders.end(); i++) {
            b2Vec2 v = (*i);
            lb->w_s_float(v.x);
            lb->w_s_float(v.y);
        }

        for (std::vector<b2Vec2>::iterator i = storages.begin(); i != storages.end(); i++) {
            b2Vec2 v = (*i);
            lb->w_s_float(v.x);
            lb->w_s_float(v.y);
        }

        for (std::vector<b2Vec2>::iterator i = enemies.begin(); i != enemies.end(); i++) {
            b2Vec2 v = (*i);
            lb->w_s_float(v.x);
            lb->w_s_float(v.y);
        }
    };

    void read_state(lvlinfo *lvl, lvlbuf *lb)
    {
        uint32_t num_ladders, num_storages, num_enemies;

        num_ladders = lb->r_uint32();
        num_storages = lb->r_uint32();
        num_enemies = lb->r_uint32();

        for (int x=0; x<num_ladders; x++) {
            b2Vec2 p;
            p.x = lb->r_float();
            p.y = lb->r_float();
            this->ladders.push_back(p);
        }

        for (int x=0; x<num_storages; x++) {
            b2Vec2 p;
            p.x = lb->r_float();
            p.y = lb->r_float();
            this->storages.push_back(p);
        }

        for (int x=0; x<num_enemies; x++) {
            b2Vec2 p;
            p.x = lb->r_float();
            p.y = lb->r_float();
            this->enemies.push_back(p);
        }
    };

    static gentype* allocate(){return new mine();};
    static gentype*
    occupy(struct gentype_data data)
    {
        if (abs(data.coord.chunk_x) < 2) {
            return 0;
        }

        mine *gt = new mine(data.coord, 1.f);

        int num_steps = 12 + _noise1(data.coord.get_world_x())*5;
        terrain_coord coord = data.coord;

        for (int x=0; x<num_steps; x++) {
            if (gt->transaction.reached_limit) {
                break;
            }
            int num_ladders = (1+(int)roundf(fabsf(_noise1(coord.get_world_y())*7.f)));
            int height = -1+6*num_ladders;
            int width = 20+roundf(_noise1(coord.get_world_x()*13.45332f)*17.f);

            int dir = (_noise2(coord.get_world_x(), coord.get_world_y()) > 0.f);
            bool storage = _noise2(coord.get_world_x()*.1f, coord.get_world_y()*.1f) > .3f;

            if (storage) {
                num_ladders ++;
            }

            for (int y=0; y<num_ladders; y++) {
                gt->ladders.push_back(b2Vec2(coord.get_world_x()-.25f, coord.get_world_y()-2.5f-3.f*y));
            }
            gt->draw_line(
                    coord.get_global_x(), coord.get_global_y(),
                    coord.get_global_x(), coord.get_global_y()-height,
                    3
                    );
            coord.step(0, -height);

            int sh = 5;
            int sw = 10;
            if (storage) {
                coord.step(0, -sh);
                for (int y=sh; y>=-sh; y--) {
                    for (int x=-1-dir*sw; x<=sw-dir*sw; x++) {
                        terrain_coord c = coord;
                        c.step(x,y);
                        gt->transaction.add(c, terrain_edit(TERRAIN_EDIT_LAYER1, TERRAIN_EMPTY));
                    }
                }

                int start = dir ? sw : -sw;

                coord.step(-start/2, -(sh-3));

                if (gt->transaction.reached_limit) {
                    break;
                }
                gt->storages.push_back(b2Vec2(b2Vec2(coord.get_world_x(), coord.get_world_y()-.25f)));

                coord.step(start/2, 0);
            }

            if (dir) {
                gt->draw_line(
                        coord.get_global_x()-1, coord.get_global_y(),
                        coord.get_global_x()+width, coord.get_global_y(),
                        6
                        );
                coord.step(width, 0);
            } else {
                gt->draw_line(
                        coord.get_global_x()-width, coord.get_global_y(),
                        coord.get_global_x(), coord.get_global_y(),
                        6
                        );
                coord.step(-width, 0);
            }

            if (gt->transaction.reached_limit) {
                break;
            }

            if (x == num_steps - 1) {
                gt->storages.push_back(b2Vec2(b2Vec2(coord.get_world_x(), coord.get_world_y()-.25f)));
            }

            if (rand()%2 == 0) {
                terrain_coord c = coord;
                c.step(dir ? -width/2 : width/2, 0);
                gt->enemies.push_back(b2Vec2(c.get_world_x(), c.get_world_y()-.75f));
            }
        }

        return gt;
    }

    void create_entities()
    {
        for (int y=0; y<this->ladders.size(); y++) {
            ladder *l = static_cast<ladder*>(of::create(O_LADDER));
            l->_pos = this->ladders[y];
            l->set_layer(1);
            l->_angle = 0;
            this->entities.insert(std::make_pair(l->id, l));
        }

        for (int y=0; y<this->storages.size(); y++) {
            entity *l = of::create(O_TREASURE_CHEST);
            l->set_property(0, "");
            l->properties[1].v.i = 0;
            l->_pos = this->storages[y];
            l->set_layer(1);
            l->_angle = 0;
            this->entities.insert(std::make_pair(l->id, l));
        }

        for (int y=0; y<this->enemies.size(); y++) {
            this->create_enemy(ROBOT_TYPE_ROBOT, this->enemies[y]);
        }
    };
};

class hidden_factory : public linegen
{
  private:
    hidden_factory(terrain_coord coord, float priority)
        : linegen(coord, priority)
    {
    }

    hidden_factory(){};

    std::vector<b2Vec2> ladders;

    bool include_storage;
    b2Vec2 storage_pos;

    std::vector<std::pair<b2Vec2, int> > enemies;

    uint32_t factory_type;
    b2Vec2 factory_pos;

  public:
    void write_state(lvlinfo *lvl, lvlbuf *lb)
    {
        lb->w_s_uint32(this->ladders.size());
        lb->w_s_uint32(this->enemies.size());

        for (std::vector<b2Vec2>::iterator i = this->ladders.begin();
                i != this->ladders.end();
                i++) {
            b2Vec2 v = *i;
            lb->w_s_float(v.x);
            lb->w_s_float(v.y);
        }

        for (int x=0; x<this->enemies.size(); x++) {
            b2Vec2 v = this->enemies[x].first;
            int t = this->enemies[x].second;
            lb->w_s_float(v.x);
            lb->w_s_float(v.y);
            lb->w_s_int32(t);
        }


        lb->w_s_bool(this->include_storage);
        if (this->include_storage) {
            lb->w_s_float(this->storage_pos.x);
            lb->w_s_float(this->storage_pos.y);
        }

        lb->w_s_uint32(this->factory_type);
        lb->w_s_float(this->factory_pos.x);
        lb->w_s_float(this->factory_pos.y);
    }

    void read_state(lvlinfo *lvl, lvlbuf *lb)
    {
        uint32_t num_ladders = lb->r_uint32();
        uint32_t num_enemies = lb->r_uint32();

        for (int x=0; x<num_ladders; x++) {
            b2Vec2 p;
            p.x = lb->r_float();
            p.y = lb->r_float();
            this->ladders.push_back(p);
        }

        for (int x=0; x<num_enemies; x++) {
            b2Vec2 p;
            p.x = lb->r_float();
            p.y = lb->r_float();
            int t = lb->r_int32();

            this->enemies.push_back(std::make_pair(p, t));
        }

        this->include_storage = lb->r_bool();

        if (this->include_storage) {
            this->storage_pos.x = lb->r_float();
            this->storage_pos.y = lb->r_float();
        }

        this->factory_type = lb->r_uint32();
        this->factory_pos.x = lb->r_float();
        this->factory_pos.y = lb->r_float();
    }

    static gentype* allocate(){return new hidden_factory();};
    static gentype*
    occupy(struct gentype_data data)
    {
        hidden_factory *gt = new hidden_factory(data.coord, 1.f);

        terrain_coord coord = data.coord;

        gt->factory_type = (int)roundf(fabsf(_noise1(coord.get_world_y())*NUM_FACTORIES));

        int num_ladders = 3;
        int height = -1+4*num_ladders;
        int width = 25+roundf(_noise1(coord.get_world_x()*10.f));

        int dir = (_noise2(coord.get_world_x(), coord.get_world_y()) > 0.f);
        gt->include_storage = _noise2(coord.get_world_x()*.1f, coord.get_world_y()*.1f) > .1f;

        for (int y=0; y<num_ladders; y++) {
            gt->ladders.push_back(b2Vec2(coord.get_world_x()-.25f, coord.get_world_y()-2.5f-3.f*y));
        }
        gt->draw_line(
                coord.get_global_x(), coord.get_global_y(),
                coord.get_global_x(), coord.get_global_y()-height,
                3
                );
        coord.step(0, -height);

        int sh = 5;
        int sw = 10;

        coord.step(0, -sh);
        for (int y=sh; y>=-sh; y--) {
            for (int x=-1-dir*sw; x<=sw-dir*sw; x++) {
                terrain_coord c = coord;
                c.step(x,y);
                gt->transaction.add(c, terrain_edit(TERRAIN_EDIT_LAYER1, TERRAIN_EMPTY));
            }
        }

        int start = dir ? sw : -sw;

        coord.step(-start/2, -(sh-3));

        gt->factory_pos = b2Vec2(b2Vec2(coord.get_world_x(), coord.get_world_y()-.25f));

        coord.step(start/2, 0);

        if (dir) {
            gt->draw_line(
                    coord.get_global_x()-1, coord.get_global_y(),
                    coord.get_global_x()+width, coord.get_global_y(),
                    6
                    );
            coord.step(width, 0);
        } else {
            gt->draw_line(
                    coord.get_global_x()-width, coord.get_global_y(),
                    coord.get_global_x(), coord.get_global_y(),
                    6
                    );
            coord.step(-width, 0);

        }

        gt->storage_pos = b2Vec2(b2Vec2(coord.get_world_x(), coord.get_world_y()-.25f));

        int num_enemies = 1+(int)roundf(fabsf(_noise1(coord.get_world_y())*3));

        for (int x=0; x<num_enemies; ++x) {
            //terrain_coord c = coord;
            coord.step(dir ? -width/2 : width/2, 0);
            b2Vec2 enemy_pos = b2Vec2(coord.get_world_x(), coord.get_world_y()-.75f);
            int robot_type = (int)roundf(fabsf(_noise1(coord.get_world_y()+(x*0.115f))*NUM_ROBOT_TYPES));
            gt->enemies.push_back(std::make_pair(enemy_pos, robot_type));
        }

        return gt;
    }

    void create_entities()
    {
        for (int y=0; y<this->ladders.size(); y++) {
            ladder *l = static_cast<ladder*>(of::create(O_LADDER));
            l->_pos = this->ladders[y];
            l->set_layer(1);
            l->_angle = 0;
            this->entities.insert(std::make_pair(l->id, l));
        }

        if (this->include_storage) {
            entity *l = of::create(O_TREASURE_CHEST);
            l->set_property(0, "");
            l->_pos = this->storage_pos;
            l->set_layer(1);
            l->_angle = 0;
            this->entities.insert(std::make_pair(l->id, l));
        }

        for (int y=0; y<this->enemies.size(); y++) {
            this->create_enemy(this->enemies[y].second, this->enemies[y].first);
        }

        {
            uint8_t g_id = 0;
            factory *f = 0;

            switch (this->factory_type) {
                case FACTORY_ROBOT: g_id = O_ROBOT_FACTORY; break;
                case FACTORY_ARMORY: g_id = O_ARMORY; break;
                case FACTORY_OIL_MIXER: g_id = O_OIL_MIXER; break;
                default:
                case FACTORY_GENERIC: g_id = O_FACTORY; break;
            }

            f = static_cast<factory*>(of::create(g_id));

            f->_pos = factory_pos;
            f->set_layer(1);
            f->_angle = 0.f;

            this->entities.insert(std::make_pair(f->id, f));
        }
    }
};

class mineral : public gentype
{
  private:
    mineral(terrain_coord coord, float priority)
        : gentype(coord, priority)
    {
        this->sorting = 1; /* apply this gentype after other gentypes */
    }

    mineral(){};

    int width;
    int height;
    int num_ores;
    int ore_type;

  public:
    void write_state(lvlinfo *lvl, lvlbuf *lb)
    {
        lb->w_s_int32(width);
        lb->w_s_int32(height);
        lb->w_s_int32(num_ores);
        lb->w_s_int32(ore_type);
    }

    void read_state(lvlinfo *lvl, lvlbuf *lb)
    {
        width = lb->r_int32();
        height = lb->r_int32();
        num_ores = lb->r_int32();
        ore_type = lb->r_int32();
    }

    static gentype* allocate(){return new mineral();};
    static gentype*
    occupy(struct gentype_data data)
    {
        mineral *gt = new mineral(data.coord, .1f);

        gt->ore_type = TPIXEL_MATERIAL_COPPER_ORE;

        struct oretype {
            int tpixel_type;
            int max_size;
            uint32_t rnd;
            float depth_influence;
        } oretypes[] = {
            {
                TPIXEL_MATERIAL_DIAMOND_ORE,
                18,
                10,
                .01f,
            }, {
                TPIXEL_MATERIAL_RUBY_ORE,
                18,
                10,
                .001f,
            }, {
                TPIXEL_MATERIAL_SAPPHIRE_ORE,
                18,
                10,
                .001f,
            }, {
                TPIXEL_MATERIAL_EMERALD_ORE,
                18,
                10,
                .001f,
            }, {
                TPIXEL_MATERIAL_TOPAZ_ORE,
                18,
                10,
                .001f,
            }, {
                TPIXEL_MATERIAL_IRON_ORE,
                30,
                10,
                .001f,
            }, {
                TPIXEL_MATERIAL_ALUMINIUM_ORE,
                18,
                5,
                .001f,
            }, {
                TPIXEL_MATERIAL_COPPER_ORE,
                18,
                1,
                .001f,
            },
        };

        int num_oretypes = sizeof(oretypes)/sizeof(struct oretype);

        for (int x=0; x<num_oretypes; x++) {
            struct oretype o = oretypes[x];
            /* TODO depth influence */
            if ((rand()%o.rnd) == 0 || x == num_oretypes-1) {
                gt->width = 3+rand()%o.max_size;
                gt->height = 3+rand()%o.max_size;
                gt->ore_type = o.tpixel_type;
                break;
            }
        }

        int size = gt->width*gt->height;

        gt->num_ores = 1+rand()%(size/8);

        data.coord.step(-gt->width/2, gt->height/2);

        for (int y=0; y<gt->height; y++) {
            for (int x=0; x<gt->width; x++) {
                data.coord.step(1,0);
                gt->transaction.add(data.coord, terrain_edit(TERRAIN_EDIT_TOUCH | TERRAIN_EDIT_LAYER1, TERRAIN_EMPTY));
            }
            data.coord.step(-gt->width,-1);
        }

        gt->width-=2;
        gt->height-=2;

        return gt;
    }

    void create_entities()
    {
        b2Vec2 pos = b2Vec2(this->coord.get_world_x(), this->coord.get_world_y());

        for (int x=0; x<num_ores; x++) {
            /* only place ores if there is some terrain around here */

            b2Vec2 op = pos+b2Vec2(.5f+-this->width/4.f,this->height/4.f);
            op.x += (rand()%20)/20.f * this->width/2.f;
            op.y += (rand()%20)/20.f * this->height/2.f;

            //float _angle = rand()%360 * (180/M_PI);
            float _angle = 0.f;
            bool valid = false;
            terrain_coord coord(op.x, op.y);

            if (W->cwindow->get_pixel(coord.get_global_x(), coord.get_global_y(), 1)) {
                valid = true;
            }
            if (valid) {
                tpixel *tp = static_cast<tpixel*>(of::create(O_TPIXEL));
                tp->set_block_type(this->ore_type);
                tp->_pos = b2Vec2(op.x, op.y);
                tp->properties[0].v.i8 = 0;
                tp->set_layer(1);
                tp->_angle = _angle;
                this->entities.insert(std::make_pair(tp->id, tp));
            }
        }
    };
};

class cave : public linegen
{
  private:
    cave(terrain_coord coord, float priority)
        : linegen(coord, priority)
    {
    }
    cave(){};

    int lx, ly;

    std::vector<b2Vec2> spikebots;
    std::vector<b2Vec2> chests;

  public:
    void write_state(lvlinfo *lvl, lvlbuf *lb)
    {
        lb->w_s_int32(lx);
        lb->w_s_int32(ly);
        lb->w_s_int32(this->spikebots.size());
        lb->w_s_int32(this->chests.size());

        for (int x=0; x<this->spikebots.size(); x++) {
            lb->w_s_float(spikebots[x].x);
            lb->w_s_float(spikebots[x].y);
        }

        for (int x=0; x<this->chests.size(); x++) {
            lb->w_s_float(chests[x].x);
            lb->w_s_float(chests[x].y);
        }
    };

    void read_state(lvlinfo *lvl, lvlbuf *lb)
    {
        lx = lb->r_int32();
        ly = lb->r_int32();
        int num_spikebots = lb->r_int32();
        int num_chests = lb->r_int32();

        for (int x=0; x<num_spikebots; x++) {
            b2Vec2 s;
            s.x = lb->r_float();
            s.y = lb->r_float();

            this->spikebots.push_back(s);
        }

        for (int x=0; x<num_chests; x++) {
            b2Vec2 s;
            s.x = lb->r_float();
            s.y = lb->r_float();

            this->chests.push_back(s);
        }
    };

    static gentype* allocate(){return new cave();};
    static gentype*
    occupy(struct gentype_data data)
    {
        cave *gt = new cave(data.coord, 2.f);

        gt->branch(
                data.coord.get_global_x(),
                data.coord.get_global_y(),
                -M_PI/2.f, 0);

        return gt;
    }

    void branch(int sx, int sy, float angle, int depth)
    {
        int num_sections = 200/(depth+1);
        static float section_length = 2.f;
        static float quirkiness = .5f;

        float prev_noise = 0;
        int ex, ey;

        for (int x=0; x<num_sections; x++) {
            //float a = roundf(angle/(M_PI/4.f))*(M_PI/4.f);
            //
            if (fabsf(tmath_adist(angle, M_PI/2.f)) < .01f) {
                angle = -angle;
            }

            float a = angle;//roundf(angle/(M_PI/8.f))*(M_PI/8.f);
            float cs = cosf(a);
            float sn = sinf(a);

            ex = sx + cs*section_length;
            ey = sy + sn*section_length;

            sx -= 2*cs;
            sy -= 2*sn;

            if (fabsf(tmath_adist(a, 0)) < M_PI/4.f
                    ||fabsf(tmath_adist(a, M_PI)) < M_PI/4.f) {
                int r = rand()%100;
                if (r < 10) {
                    spikebots.push_back(b2Vec2((sx+ex)*.25f, (sy+ey)*.25f));
                } else if (r < 13) {
                    chests.push_back(b2Vec2((sx+ex)*.25f, (sy+ey)*.25f));
                }
            }

            this->draw_line(
                sx, sy,
                ex, ey,
                (8)+roundf(_noise2(sx*2.f, sy*2.f))*2
                );

            sx = ex;
            sy = ey;

            float quirk_mod = 1.f;
            if (depth == 0) {
                if (fabsf(tmath_adist(angle, -M_PI/2.f)) < .2f) {
                    quirk_mod = 3.f;
                }
            }
            angle += _noise2(sx*.9f, sy*.9f)*quirkiness*quirk_mod;
            float offs = (ex-this->coord.get_global_x());
            float downoffs = tmath_adist(angle, -M_PI/2.f);
            float blend = tclampf(fabsf(.25f*(float)offs / (float)(GENTYPE_MAX_REACH_X*16-16)), 0.f, 1.f);

            if (copysignf(1.f, offs) != copysignf(1.f, downoffs)) {
                angle = (1.f-blend)*angle + blend*(angle+downoffs);
            }

            float noise = _noise2(sx*.01f, sy*.01f);
            if (prev_noise >= .2f && noise < .2f && depth < 1) {
                this->branch(sx, sy, angle - M_PI/4.f, depth+1);
            }
            prev_noise = noise;
        }

        this->lx = (ex+sx)/2;
        this->ly = (ey+sy)/2;
    }

    void create_entities()
    {
        terrain_coord last;
        last.set_from_global(this->lx, this->ly);

        float x = last.get_world_x();
        float y = last.get_world_y();

        //this->create_enemy(ROBOT_TYPE_ROBOT, b2Vec2(x,y), 1, FACTION_CHAOTIC);
        entity *l = of::create(O_TREASURE_CHEST);
        l->set_property(0, "");
        l->_pos = b2Vec2(x,y);
        l->set_layer(1);
        l->_angle = 0;
        this->entities.insert(std::make_pair(l->id, l));

        for (int x=0; x<this->spikebots.size(); x++) {
            this->create_enemy(ROBOT_TYPE_SPIKEBOT, this->spikebots[x], 1, FACTION_ENEMY);
        }

        for (int x=0; x<this->chests.size(); x++) {
            entity *l = of::create(O_TREASURE_CHEST);
            l->set_property(0, "");
            l->_pos = this->chests[x];
            l->set_layer(1);
            l->_angle = 0;
            this->entities.insert(std::make_pair(l->id, l));
        }
    };
};

class deep_cave : public linegen
{
  private:
    deep_cave(terrain_coord coord, float priority)
        : linegen(coord, priority)
    {
    }
    deep_cave(){};

    int lx, ly;
    int depth;

  public:
    void write_state(lvlinfo *lvl, lvlbuf *lb)
    {
        lb->w_s_int32(lx);
        lb->w_s_int32(ly);
        lb->w_s_int32(depth);
    };

    void read_state(lvlinfo *lvl, lvlbuf *lb)
    {
        lx = lb->r_int32();
        ly = lb->r_int32();
        depth = lb->r_int32();
    };

    static gentype* allocate(){return new deep_cave();};
    static gentype*
    occupy(struct gentype_data data)
    {
        deep_cave *gt = new deep_cave(data.coord, 1.f);
        gt->depth = roundf(data.heights[7] - data.coord.get_global_y());

        gt->branch(
                data.coord.get_global_x(),
                data.coord.get_global_y(),
                0, 0);

        return gt;
    }

    void branch(int sx, int sy, float angle, int depth)
    {
        int num_sections = 40;
        static float section_length = 2.f;
        static float quirkiness = 3.f;

        int ex, ey;

        for (int x=0; x<num_sections; x++) {
            float a = roundf(angle/(M_PI/8.f))*(M_PI/8.f);
            float cs = cosf(a);
            float sn = sinf(a);

            ex = sx + cs*section_length;
            ey = sy + sn*section_length;

            sx -= 2*cs;
            sy -= 2*sn;

            this->draw_line(
                sx, sy,
                ex, ey,
                (12)+roundf(_noise2(sx*2.f, sy*2.f))*2
                );

            sx = ex;
            sy = ey;

            float quirk_mod = 1.f;
            angle += _noise2(sx*.9f, sy*.9f)*quirkiness*quirk_mod;
            float offs = (ex-this->coord.get_global_x());
            float downoffs = tmath_adist(angle, -M_PI/2.f);
            float blend = tclampf(fabsf(.25f*(float)offs / (float)(GENTYPE_MAX_REACH_X*16-16)), 0.f, 1.f);

            if (copysignf(1.f, offs) != copysignf(1.f, downoffs)) {
                angle = (1.f-blend)*angle + blend*(angle+downoffs);
            }
        }

        this->lx = (ex+sx)/2;
        this->ly = (ey+sy)/2;
    }

    void create_entities()
    {
        terrain_coord last;
        last.set_from_global(this->lx, this->ly);

        float x = last.get_world_x();
        float y = last.get_world_y();

        //this->create_enemy(ROBOT_TYPE_ROBOT, b2Vec2(x,y), 1, FACTION_CHAOTIC);
        entity *l = of::create(O_TREASURE_CHEST);
        l->set_property(0, "");
        int quality_bias = this->depth;
        if (quality_bias < 100) {
            quality_bias = 100;
        } else if (quality_bias > 800) {
            quality_bias = 800;
        }
        uint32_t q = 20+(rand()%(int)roundf((35*(quality_bias/800.f))));
        l->properties[1].v.i = q;
        l->_pos = b2Vec2(x,y);
        l->set_layer(1);
        l->_angle = 0;
        this->entities.insert(std::make_pair(l->id, l));

        this->create_enemy(ROBOT_TYPE_SPIKEBOT, b2Vec2(this->coord.get_world_x(),this->coord.get_world_y()), 1, FACTION_CHAOTIC);
    };
};

class testtest : public gentype
{
  private:
    testtest(terrain_coord coord, float priority)
        : gentype(coord, priority)
    {
    }

    testtest(){};

  public:
    void write_state(lvlinfo *lvl, lvlbuf *lb){tms_fatalf("unimplemented");};
    void read_state(lvlinfo *lvl, lvlbuf *lb){tms_fatalf("unimplemented");};
    static gentype* allocate(){return new testtest();};
    static gentype*
    occupy(struct gentype_data data)
    {
        testtest *gt = new testtest(data.coord, _noise2(data.base_x, data.base_y));

        terrain_edit  e(TERRAIN_EDIT_LAYER0, TERRAIN_DIRT);
        gt->transaction.add(data.coord, e);
        data.coord.step(1, 0);
        gt->transaction.add(data.coord, e);
        data.coord.step(-2, 0);
        gt->transaction.add(data.coord, e);
        data.coord.step(1, 1);
        gt->transaction.add(data.coord, e);
        data.coord.step(0, -2);
        gt->transaction.add(data.coord, e);

        return gt;
    }

    void create_entities()
    {
    }
};

class hole : public gentype
{
  private:
    hole(terrain_coord coord, float priority)
        : gentype(coord, priority)
    {
    }
    hole(){};

  public:
    void write_state(lvlinfo *lvl, lvlbuf *lb){};
    void read_state(lvlinfo *lvl, lvlbuf *lb){};
    static gentype* allocate(){return new hole();};
    static gentype*
    occupy(struct gentype_data data)
    {
        int length = (int)tclampf(20.f+(_noise1(data.coord.get_world_x()*1.32154325f)*170.f), 1.f, 170.f);
        int base_width = 5.f + (int)tclampf((_noise1(data.coord.get_world_x()*.25f)*30.f), 1.f, 30.f);

        terrain_coord c = data.coord;

        gentype *nh = new hole(data.coord, 2.f);

        for (int y=0; y<length; y++) {
            int nw = _noise2(c.get_world_x()*.2f, c.get_world_y()*.2f)*3;
            int width = ((float)base_width*(1.f-(float)y/((float)length+5.f)))+nw;
            if (y%2==0) {
                c.step(-floor((float)width/2.f), 0);
            } else {
                c.step(-ceil((float)width/2.f), 0);
            }

            for (int x=0; x<width; x++) {
                c.step(1, 0);
                if (base_width < 7) {
                    nh->transaction.add(c, terrain_edit((
                                    TERRAIN_EDIT_LAYER1
                                    ), TERRAIN_EMPTY));
                } else {
                    nh->transaction.add(c, terrain_edit((
                                    ((std::abs(x-width/2)+nw*2 < width/2) * TERRAIN_EDIT_LAYER2)
                                    | ((std::abs(x-width/2) < width/2)*TERRAIN_EDIT_LAYER1)
                                    | ((std::abs(x-width/2)+y/6 < width/4)*TERRAIN_EDIT_LAYER0)
                                    ), TERRAIN_EMPTY));
                }
            }

            if (y%2==0) {
                c.step(-floor((float)width/2.f), -1);
            } else {
                c.step(-ceil((float)width/2.f), -1);
            }
        }

        return nh;
    }

    void create_entities()
    {
    }
};

class gravething : public gentype
{
  private:
    gravething(struct terrain_coord coord, float priority)
        : gentype(coord, priority)
    {
    }
    gravething(){};

  public:
    void write_state(lvlinfo *lvl, lvlbuf *lb){};
    void read_state(lvlinfo *lvl, lvlbuf *lb){};
    static gentype* allocate(){return new gravething();};
    static gentype*
    occupy(struct gentype_data data)
    {
        static const char *terrain[3][14] =
        { {
            "  044034  ",
            " 33333323 ",
            "2333333232",
            "4232333324",
            "4233333334",
            "4333233334",
            "2333333333",
            " 332333333",
            " 333233333",
            " 333233333",
            " 333233333",
            " 333233333",
            " 333333333",
            " 333333333",
        }, {
            " 032004400",
            " 323003200",
            " 333003233",
            "2300000323",
            "4320000034",
            "4320000204",
            "3300000004",
            "2340043233",
            "23 0004434",
            "2300004443",
            "23 0000033",
            "23     433",
            "2222244222",
            "2222222222",
        }, {
            " 000000400",
            " 444000200",
            " 333004244",
            " 433333334",
            " 433333334",
            " 433333334",
            " 432333334",
            " 433333234",
            " 433233333",
            " 333233333",
            " 333233333",
            " 333233333",
            " 333333333",
            " 333333333",
        } };
        static const int num_y = 14;
        static const int width = strlen(terrain[0][0]);

        static const int ox = -width/2;
        static const int oy = num_y/2;

        struct terrain_coord c = data.coord;

        c.step(ox, oy);

        float wx = data.coord.get_world_x();

        gentype *gt = new gravething(data.coord, 1.f);

        terrain_coord cc = c;

        for (int z=0; z<3; z++) {
            for (int y=0; y<num_y; y++) {
                for (int x=0; x<width; x++) {
                    switch (terrain[z][y][x]) {
                        case '0': gt->transaction.add(c, terrain_edit((TERRAIN_EDIT_LAYER0<<z), TERRAIN_EMPTY)); break;
                        case '2': gt->transaction.add(c, terrain_edit((TERRAIN_EDIT_LAYER0<<z) | TERRAIN_EDIT_INC | TERRAIN_EDIT_HARDEN, TERRAIN_DIRT)); break;
                        case '3': gt->transaction.add(c, terrain_edit((TERRAIN_EDIT_LAYER0<<z) | TERRAIN_EDIT_INC | TERRAIN_EDIT_HARDEN, TERRAIN_STONE)); break;
                        case '4': {
                                      if (_noise2(wx+x, y) > 0.f) {
                                          gt->transaction.add(c, terrain_edit((TERRAIN_EDIT_LAYER0<<z) | TERRAIN_EDIT_INC | TERRAIN_EDIT_HARDEN, TERRAIN_DIRT));
                                      }
                                  }
                                  break;
                    }
                    c.step(1, 0);
                }
                c.step(-width, -1);
            }

            c = cc;
        }

        return gt;
    }

    void create_entities()
    {
        float wx = this->coord.get_world_x();
        float wy = this->coord.get_world_y();

        if (0) {
            robot *e = static_cast<robot*>(of::create(O_ROBOT));
            e->set_layer(1);
            e->_pos = b2Vec2(wx/*-count*3+x*3*/, wy);
            e->_angle = 0.f;

            e->properties[ROBOT_PROPERTY_STATE].v.i = CREATURE_DEAD;
            e->properties[ROBOT_PROPERTY_HEAD].v.i = 0;
            e->properties[ROBOT_PROPERTY_BACK].v.i = 0;
            e->properties[ROBOT_PROPERTY_FRONT].v.i = 0;

            this->entities.insert(std::make_pair(e->id, e));
        }

        {
            item *e = of::create_item(ITEM_ROBOT_HEAD);
            e->set_layer(1);
            e->_pos = b2Vec2(wx/*-count*3+x*3*/, wy-1.f);
            e->_angle = 0.f;

            this->entities.insert(std::make_pair(e->id, e));
        }

        {
            item *e = of::create_item(ITEM_PIONEER_FRONT);
            e->set_layer(1);
            e->_pos = b2Vec2(wx/*-count*3+x*3*/, wy-1.f);
            e->_angle = 0.f;

            this->entities.insert(std::make_pair(e->id, e));
        }

        {
            item *e = of::create_item(ITEM_PIONEER_BACK);
            e->set_layer(1);
            e->_pos = b2Vec2(wx/*-count*3+x*3*/, wy-1.f);
            e->_angle = 0.f;

            this->entities.insert(std::make_pair(e->id, e));
        }
    }
};

/**
 * Things that can be generated
 **/
struct gentype_generator gentype::gentypes[NUM_GENTYPES] = {
    {
        "Nomad Hideout",
        false,
        0.2f,
        0.f,
        0.f, 0.f,
        &nomad_hideout::occupy,
        &nomad_hideout::allocate,
    }, {
        "Animals",
        true,
        0.1f,
        0.f,
        0.f, 0.f,
        &animals::occupy,
        &animals::allocate,
    }, {
        "Gravethingy",
        true,
        0.02f,
        0.f,
        0.f, 0.f,
        &gravething::occupy,
        &gravething::allocate,
    }, {
        "Hole",
        true,
        0.12f,
        0.f,
        0.f, 0.f,
        &hole::occupy,
        &hole::allocate,
    }, {
        "test",
        false,
        1.0f,
        1.0f,
        10.f, 50.f,
        &testtest::occupy,
        &testtest::allocate,
    }, {
        "Mine",
        true,
        .25f,
        0.f,
        0.f, 0.f,
        &mine::occupy,
        &mine::allocate,
    }, {
        "Minerals",
        true,
        .2f,
        .2f,
        -100.f, -2.f,
        &mineral::occupy,
        &mineral::allocate,
    }, {
        "Cave",
        true,
        .25f,
        .0f,
        0.f, 0.f,
        &cave::occupy,
        &cave::allocate,
    }, {
        "Hidden factory",
        true,
        .125f,
        .0f,
        0.f, 0.f,
        &hidden_factory::occupy,
        &hidden_factory::allocate,
    }, {
        "Deep cave",
        true,
        .05f,
        .05f,
        -500.f, -30.f,
        &deep_cave::occupy,
        &deep_cave::allocate,
    }, {
        "Enemies",
        true,
        0.2f,
        0.f,
        0.f, 0.f,
        &enemies::occupy,
        &enemies::allocate,
    },
};

bool
gentype::post_occupy()
{
    this->transaction.occupy(this);

    switch (this->transaction.state) {
        case TERRAIN_TRANSACTION_OCCUPIED:
            return !this->genslots.empty();
            break;

        default:
            tms_debugf("terrain transaction failed");
            return false;
    }
}

void
gentype::apply()
{
    if (this->applied) {
        return;
    }

    switch (this->transaction.state) {
        case TERRAIN_TRANSACTION_OCCUPIED:
            if (this->sorting == 0) {
                this->transaction.apply();
            } else {
                this->transaction.state = TERRAIN_TRANSACTION_APPLIED;
            }
            break;

        default:
            tms_debugf("can not generate, transaction state is not occupied");
            return;
    }

    if (this->transaction.state == TERRAIN_TRANSACTION_APPLIED) {
        this->create_entities();
        this->add_to_world();
    }

    this->applied = true;
}

void
gentype::add_to_world()
{
    if (this->transaction.state != TERRAIN_TRANSACTION_APPLIED) {
        return;
    }

    /* XXX TODO this does NOT handle groups correctly! */

    for (std::map<uint32_t, entity*>::iterator i = this->entities.begin(); i!= this->entities.end();) {
        entity *e = i->second;
        e->on_load(false, false);

        terrain_coord c(e->_pos.x, e->_pos.y);
        level_chunk *chunk = W->cwindow->get_chunk(c.chunk_x, c.chunk_y);

        if (chunk->load_phase >= 2 && chunk->body) {
            /* immediately add the entitity to the world */
            W->add(e);
            i++;
            continue;
        }

        e->pre_write();
        of::write(&W->cwindow->preloader.heap, W->level.version, e, 0, b2Vec2(0.f, 0.f), false);
        e->post_write();
        preload_info info(e->write_ptr, e->write_size, true);

        W->cwindow->preloader.entities.insert(std::make_pair(e->id, info));
        W->cwindow->preloader.entities_by_chunk.insert(std::make_pair(chunk_pos(c.chunk_x, c.chunk_y), e->id));

        /* unload the entity for later loading */
        this->entities.erase(i++);
        delete e;
    }

    W->init_level_entities(&this->entities, &this->groups);
    G->add_entities(&entities, &this->groups, &this->connections, &this->cables);
}

#define GENTYPE_THRESHOLD    .1f
#define GENTYPE_NOISE_Y_BIAS 3.321785f

/**
 * occupy the given chunk with one or several
 * gentypes that should be generated
 **/
void
gentype::generate(level_chunk *c)
{
    /**
     * gentype slots
     *
     * allocate slots above ground, slot below ground
     *
     * above ground
     * under surface
     * deep
     **/

    float *heights = W->cwindow->get_heights(c->pos_x, true);

#ifdef DEBUG_PRELOADER_SANITY
    tms_assertf(heights[0] > -1000.f, "height should have been initialized properly here!");
#endif

    gentype_data data;
    data.heights = heights;
    data.c = c;

#ifdef DEBUG_PRELOADER_SANITY
    tms_assertf(abs(c->pos_x) < 1000 && abs(c->pos_y) < 1000, "suspicious chunk pos %d,%d received by gentype::generate", c->pos_x, c->pos_y);
#endif

    for (int t=0; t<NUM_GENTYPES; t++) {
        if (!gentypes[t].enabled) {
            continue;
        }

        if (gentypes[t].y_prevalence == 0.f) {
            /* generate at surface level only */
            if (!is_surface_level(c->pos_y, heights)) {
                continue;
            }
        } else {
            if (!check_depth_range(c->pos_y, heights, gentypes[t].y_min, gentypes[t].y_max)) {
                continue;
            }
        }

        float last_v, v;
        for (int x=-1; x<4; x++) {
            v = _noise2(
                    (c->pos_x*4.f + x)*gentypes[t].x_prevalence,
                    GENTYPE_NOISE_Y_BIAS*(t+1) + ((c->pos_y*4.f+x) * gentypes[t].y_prevalence)
                    );

            if (x >= 0) {
                if (last_v >= GENTYPE_THRESHOLD && v < GENTYPE_THRESHOLD) {
                    data.base_x = c->pos_x * 8.f + (x*(8.f/4.f) + 8.f/8.f);
                    data.base_y = c->pos_y * 8.f + 4.f;
                    data.update_coord();

                    if (gentypes[t].y_prevalence == 0.f) {
#ifdef DEBUG_PRELOADER_SANITY
                        int previous_x = data.coord.chunk_x;
                        int previous_y = data.coord.chunk_y;
#endif
                        data.move_to_surface(0.f);
#ifdef DEBUG_PRELOADER_SANITY
                        tms_assertf(abs(data.coord.chunk_x) < 1000 && abs(data.coord.chunk_y) < 1000, "suspicious chunk pos %d,%d after move_to_surface, previous was %d,%d", data.coord.chunk_x, data.coord.chunk_y, previous_x, previous_y);
#endif
                    }

                    gentype *gt;

                    if ((gt = gentypes[t].occupy(data))) {
                        gt->type = t;
                        tms_debugf("%p occupied %s successfully", gt, gentypes[t].name);
                        if (!gt->post_occupy()) {
                            tms_debugf("%p Nevermind, post-occupy failed. We might have been invalidated or the gentype did not actually occupy any genslots (which is required).", gt);
                            delete gt;
                        } else {

                            W->cwindow->preloader.gentypes.insert(std::make_pair(gt->id, gt));
                            /**
                             * Make sure all neighbours are loaded
                             **/

                            /* why???? */

                            /*
                            level_chunk *neighbours[8];
                            W->cwindow->preloader.get_chunk_neighbours(c, neighbours, true);

                            for (int x=0; x<8; x++) {
                                neighbours[x]->generate(W->cwindow, 3);
                            }
                            */
                        }
                        break;
                    }
                }
                break;
            }
            last_v = v;
        }
    }
}
