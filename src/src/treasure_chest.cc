#include "treasure_chest.hh"
#include "model.hh"
#include "material.hh"
#include "game.hh"
#include "item.hh"
#include "resource.hh"
#include "ui.hh"

#include <sstream>

#define QUALITY_COMMON   (1UL << 0)
#define QUALITY_UNCOMMON (1UL << 1)
#define QUALITY_RARE     (1UL << 2)
#define QUALITY_EPIC     (1UL << 3)

#define QUALITY_EPIC_UP     QUALITY_EPIC
#define QUALITY_RARE_UP     QUALITY_RARE | QUALITY_EPIC_UP
#define QUALITY_UNCOMMON_UP QUALITY_UNCOMMON | QUALITY_RARE_UP
#define QUALITY_COMMON_UP   QUALITY_COMMON | QUALITY_UNCOMMON_UP

#define QUALITY_ALL QUALITY_COMMON_UP

static const int MAX_ENTITY_COUNT = 15;

/**
 * Two properties:
 *
 * Property 0: A list of objects contained in the chest.
 * Format:
 * g_id:sub_id:count;
 * An empty list ("") means the loot will be randomized upon activation.
 *
 * Property 1: Quality bias
 * Default value: 0
 * Max value: 150
 * Increasing it by 150 would mean the chest is always of epic quality.
 **/
treasure_chest::treasure_chest()
    : activator(ATTACHMENT_NONE)
{
    this->set_flag(ENTITY_ALLOW_CONNECTIONS, false);
    this->set_flag(ENTITY_HAS_ACTIVATOR,    true);
#ifdef TMS_BACKEND_PC
    this->set_flag(ENTITY_HAS_CONFIG,        true);
#endif
    this->set_flag(ENTITY_DYNAMIC_UNLOADING, true);

    this->dialog_id = DIALOG_TREASURE_CHEST;

    this->set_mesh(mesh_factory::get_mesh(MODEL_TREASURE_CHEST));
    this->set_material(&m_chest_shiny);

    this->set_as_rect(1.55f/2.f, .90f/2.f);

    this->query_vec = b2Vec2(0, -.15f);
    this->query_pt = b2Vec2(0, -.45f);

    this->set_num_properties(2);
    this->properties[0].type = P_STR;
    this->set_property(0, "");//"210:1:20;186:21:2;210:3:453;210:5:10");
    this->properties[1].type = P_INT;
    this->properties[1].v.i = 0;
}

void
treasure_chest::activate(creature *by)
{
    this->disconnect_all();

    if (G->absorb(this)) {
        uint32_t quality = QUALITY_COMMON;

        if (this->properties[0].v.s.len == 0) {
            quality = this->randomize_loot();
        }

        this->emit_contents();

        switch (quality) {
            case QUALITY_RARE:
            case QUALITY_EPIC:
                G->play_sound(SND_CHEST_OPEN_RARE, this->get_position().x, this->get_position().y, 0, 1.f);
                break;

            default:
                G->play_sound(SND_CHEST_OPEN, this->get_position().x, this->get_position().y, 0, 1.f);
                break;
        }

        tms_infof("-------");

        tms_infof("successfully absorbed");
    } else {
        tms_infof("FAIL");
    }
}

static struct possible_loot {
    uint32_t g_id;
    uint32_t sub_id;

    int base_quantity;
    int max_quantity;

    int drop_chance;
    uint32_t quality_flags;
} possible_loots[] = {
    {
        O_ITEM,
        ITEM_RAILGUN,
        1, 1,
        70,
        QUALITY_UNCOMMON_UP
    },
    {
        O_ITEM,
        ITEM_SHOTGUN,
        1, 1,
        20,
        QUALITY_UNCOMMON_UP
    },
    {
        O_ITEM,
        ITEM_TESLAGUN,
        1, 1,
        5,
        QUALITY_RARE_UP,
    },
    {
        O_ITEM,
        ITEM_PLASMAGUN,
        1, 1,
        10,
        QUALITY_RARE_UP,
    },
    {
        O_ITEM,
        ITEM_BOMBER,
        1, 1,
        35,
        QUALITY_UNCOMMON_UP,
    },
    {
        O_ITEM,
        ITEM_ROCKET_LAUNCHER,
        1, 1,
        35,
        QUALITY_UNCOMMON_UP,
    },
    {
        O_ITEM,
        ITEM_MEGABUSTER,
        1, 1,
        5,
        QUALITY_EPIC_UP,
    },
    {
        O_ITEM,
        ITEM_FACTION_WAND,
        1, 1,
        35,
        QUALITY_EPIC_UP,
    },
    {
        O_ITEM,
        ITEM_ARMOUR_OIL,
        1, 1,
        35,
        QUALITY_UNCOMMON_UP,
    },
    {
        O_ITEM,
        ITEM_NINJAHELMET,
        1, 1,
        30,
        QUALITY_RARE_UP,
    },
    {
        O_ITEM,
        ITEM_WIZARDHAT,
        1, 1,
        50,
        QUALITY_RARE_UP,
    },
    {
        O_ITEM,
        ITEM_POLICEHAT,
        1, 1,
        40,
        QUALITY_RARE_UP,
    },
    {
        O_ITEM,
        ITEM_CONICALHAT,
        1, 1,
        25,
        QUALITY_RARE_UP,
    },

    {
        O_ITEM,
        ITEM_ZOMBIE_CIRCUIT,
        1, 1,
        10,
        QUALITY_EPIC_UP,
    },
    {
        O_ITEM,
        ITEM_REGENERATION_CIRCUIT,
        1, 1,
        20,
        QUALITY_RARE_UP,
    },
    {
        O_ITEM,
        ITEM_SOMERSAULT_CIRCUIT,
        1, 1,
        15,
        QUALITY_RARE_UP,
    },

    {
        O_ITEM,
        ITEM_BOLT_SET_WOOD,
        1, 1,
        40,
        QUALITY_UNCOMMON_UP,
    },
    {
        O_ITEM,
        ITEM_BOLT_SET_STEEL,
        1, 1,
        30,
        QUALITY_RARE_UP,
    },
    {
        O_ITEM,
        ITEM_BOLT_SET_SAPPHIRE,
        1, 1,
        40,
        QUALITY_RARE_UP,
    },
    {
        O_ITEM,
        ITEM_BOLT_SET_DIAMOND,
        1, 1,
        5,
        QUALITY_EPIC_UP,
    },

    {
        O_ITEM,
        ITEM_HAT,
        1, 1,
        10,
        QUALITY_EPIC_UP,
    },
    {
        O_ITEM,
        ITEM_BLACK_ROBOT_FRONT,
        1, 1,
        55,
        QUALITY_UNCOMMON_UP,
    },

    {
        O_ITEM,
        ITEM_JETPACK,
        1, 1,
        55,
        QUALITY_UNCOMMON,
    },
    {
        O_ITEM,
        ITEM_UPGRADED_JETPACK,
        1, 1,
        35,
        QUALITY_RARE,
    },
    {
        O_ITEM,
        ITEM_ADVANCED_JETPACK,
        1, 1,
        25,
        QUALITY_EPIC,
    },

    {
        O_RESOURCE,
        RESOURCE_RUBY,
        25, 75,
        10,
        QUALITY_ALL
    },
    {
        O_RESOURCE,
        RESOURCE_SAPPHIRE,
        25, 75,
        10,
        QUALITY_ALL
    },
    {
        O_RESOURCE,
        RESOURCE_EMERALD,
        25, 75,
        10,
        QUALITY_ALL
    },
    {
        O_RESOURCE,
        RESOURCE_TOPAZ,
        25, 75,
        10,
        QUALITY_ALL
    },
    {
        O_RESOURCE,
        RESOURCE_WOOD,
        35, 125,
        8,
        QUALITY_ALL
    },
    {
        O_RESOURCE,
        RESOURCE_COPPER,
        35, 125,
        8,
        QUALITY_ALL
    },
    {
        O_RESOURCE,
        RESOURCE_IRON,
        20, 40,
        12,
        QUALITY_ALL
    },
    {
        O_RESOURCE,
        RESOURCE_DIAMOND,
        5, 10,
        15,
        QUALITY_UNCOMMON_UP
    },
    {
        O_RESOURCE,
        RESOURCE_ALUMINIUM,
        20, 45,
        12,
        QUALITY_ALL
    },
    {
        O_ITEM,
        ITEM_BLACK_ROBOT_BACK,
        1, 1,
        55,
        QUALITY_UNCOMMON_UP,
    },
    {
        O_ITEM,
        ITEM_DUMMY_HEAD,
        1, 1,
        55,
        QUALITY_RARE_UP,
    },
    {
        O_ITEM,
        ITEM_KINGSCROWN,
        1, 1,
        20,
        QUALITY_EPIC_UP,
    },
    {
        O_ITEM,
        ITEM_JESTERHAT,
        1, 1,
        55,
        QUALITY_RARE_UP,
    },
    {
        O_ITEM,
        ITEM_WITCH_HAT,
        1, 1,
        50,
        QUALITY_RARE_UP,
    },
    {
        O_ITEM,
        ITEM_HAMMER,
        1, 1,
        20,
        QUALITY_UNCOMMON_UP
    },
    {
        O_ITEM,
        ITEM_CHAINSAW,
        1, 1,
        25,
        QUALITY_RARE_UP,
    },
    {
        O_ITEM,
        ITEM_SPIKED_CLUB,
        1, 1,
        25,
        QUALITY_UNCOMMON_UP
    },
    {
        O_ITEM,
        ITEM_STEEL_SWORD,
        1, 1,
        30,
        QUALITY_UNCOMMON_UP
    },
    {
        O_ITEM,
        ITEM_BASEBALLBAT,
        1, 1,
        30,
        QUALITY_UNCOMMON_UP
    },
    {
        O_ITEM,
        ITEM_WAR_AXE,
        1, 1,
        40,
        QUALITY_RARE_UP,
    },
    {
        O_ITEM,
        ITEM_PIXEL_SWORD,
        1, 1,
        25,
        QUALITY_RARE_UP,
    },
    {
        O_ITEM,
        ITEM_SERPENT_SWORD,
        1, 1,
        35,
        QUALITY_RARE_UP,
    },
    {
        O_ITEM,
        ITEM_HARD_HAT,
        1, 1,
        30,
        QUALITY_RARE_UP,
    },
    {
        O_ITEM,
        ITEM_VIKING_HELMET,
        1, 1,
        30,
        QUALITY_RARE_UP,
    },
    {
        O_ITEM,
        ITEM_PIONEER_FRONT,
        1, 1,
        40,
        QUALITY_RARE_UP,
    },
    {
        O_ITEM,
        ITEM_PIONEER_BACK,
        1, 1,
        40,
        QUALITY_RARE_UP,
    },
    {
        O_ITEM,
        ITEM_PICKAXE,
        1, 1,
        20,
        QUALITY_UNCOMMON_UP,
    },
};

static int num_possible_drops = sizeof(possible_loots) / sizeof(possible_loots[0]);

#define MAX_ITEMS_IN_CHEST 5

uint32_t
treasure_chest::randomize_loot()
{
    int qual_rand = 150;
    qual_rand -= this->properties[1].v.i;

    if (qual_rand < 1) {
        qual_rand = 1;
    }

    int rqual = rand()%qual_rand;

    uint32_t quality = QUALITY_COMMON;
    int min_items = 2;

    float quantity_mod = 1.f;


    if (rqual < 5) {
        quality = QUALITY_EPIC;
        quantity_mod = 3.f;
        min_items = 4;
        ui::message("Epic!");
    } else if (rqual < 25) {
        quality = QUALITY_RARE;
        quantity_mod = 2.f;
        min_items = 3;
        ui::message("Rare!");
    } else if (rqual < 85) {
        quality = QUALITY_UNCOMMON;
        quantity_mod = 1.5f;
        ui::message("Uncommon");
    } else {
        ui::message("Common...");
    }

    int num_items = rand()%MAX_ITEMS_IN_CHEST;

    if (num_items < min_items) {
        num_items = min_items;
    }

    int x = 0;
    std::stringstream ss;

    while (num_items) {
        int item_id = rand() % num_possible_drops;
        struct possible_loot *pl = &possible_loots[item_id];

        /* Is it possible for the item we randomized to drop in this chest quality? */
        if (pl->quality_flags & quality) {
            if (rand() % pl->drop_chance == 0) {
                if (x > 0) {
                    ss << ";";
                }

                int quantity = rand()%pl->base_quantity;

                quantity *= quantity_mod;

                if (quantity < 1) {
                    quantity = 1;
                } else if (quantity > pl->max_quantity) {
                    quantity = pl->max_quantity;
                }

                ss << pl->g_id << ":" << pl->sub_id << ":" << quantity;

                ++ x;
                -- num_items;
            }
        }
    }

    this->set_property(0, ss.str().c_str());

    return quality;
}

struct treasure_chest_item
treasure_chest::parse_item(char *str)
{
    char *sg_id;
    char *ssub_id;
    char *scount;
    int n = 0;

    char *ic = strchr(str, ':');
    while (ic != NULL) {
        *ic = '\0';

        switch (n) {
            case 0: sg_id = str; break;
            case 1: ssub_id = str; break;

            default:
                tms_errorf("invalid string from treasure cehst::emit_item");
                return treasure_chest_item(-1, -1, -1);
        }

        str = ic+1;

        ++ n;
        ic = strchr(str, ':');

        if (ic == NULL) {
            scount = str;
            break;
        }
    }

    if (n == 2) {
        int g_id = atoi(sg_id);
        int sub_id = atoi(ssub_id);
        int count = atoi(scount);

        return treasure_chest_item(g_id, sub_id, count);
    }

    return treasure_chest_item(-1, -1, -1);
}

std::vector<struct treasure_chest_item>
treasure_chest::parse_items(char *str)
{
    std::vector<struct treasure_chest_item> ret;
    int len = strlen(str);

    std::vector<char*> strings = p_split(str, len, ";");

    for (std::vector<char*>::iterator it = strings.begin();
            it != strings.end(); ++it) {
        ret.push_back(treasure_chest::parse_item(*it));
    }

    return ret;
}

void
treasure_chest::emit_item(struct treasure_chest_item &tci)
{
    if (tci.count < 0) {
        tms_warnf("Invalid count: %d", tci.count);
        return;
    }
#define rx p.x-.125f+(rand()%100)/100.f*.25f
#define ry p.y-.125f+(rand()%100)/100.f*.25f

    b2Vec2 p = this->get_position();

    switch (tci.g_id) {
        case O_ITEM:
            {
                if (tci.count > MAX_ENTITY_COUNT) {
                    tms_warnf("Count was set to %d, reducing it to %d", tci.count, MAX_ENTITY_COUNT);
                    tci.count = MAX_ENTITY_COUNT;
                }

                if (tci.sub_id < 0 || tci.sub_id > NUM_ITEMS) {
                    tms_warnf("Invalid item ID: %d", tci.sub_id);
                    return;
                }

                for (; tci.count > 0; --tci.count) {
                    item *i = static_cast<item*>(of::create(tci.g_id));
                    if (i) {
                        i->set_item_type(tci.sub_id);
                        i->set_position(rx, ry);
                        i->set_layer(this->get_layer());
                        G->emit(i, this, b2Vec2(0, 0));
                    } else {
                        break;
                    }
                }
            }
            break;

        case O_RESOURCE:
            {
                if (tci.sub_id < 0 || tci.sub_id > NUM_RESOURCES) {
                    tms_warnf("Invalid resource ID: %d", tci.sub_id);
                    return;
                }

                int split = 1;

                if (tci.count > 5) {
                    /* if the count is > 5, we split them up into chunks
                     * of <=10 per resource */
                    split = 10;
                }

                while (tci.count > 0) {
                    int out = std::min(split, tci.count);

                    resource *r = static_cast<resource*>(of::create(tci.g_id));
                    if (r) {
                        r->set_resource_type(tci.sub_id);
                        r->set_amount(out);
                        r->set_position(rx, ry);
                        r->set_layer(this->get_layer());
                        G->emit(r, this, b2Vec2(0, 0));
                    }

                    tci.count -= out;
                }
            }
            break;

        default:
            {
                if (tci.count > MAX_ENTITY_COUNT) {
                    tms_warnf("Count was set to %d, reducing it to %d", tci.count, MAX_ENTITY_COUNT);
                    tci.count = MAX_ENTITY_COUNT;
                }

                for (; tci.count > 0; --tci.count) {
                    entity *e = of::create(tci.g_id);
                    if (e) {
                        e->set_position(rx, ry);
                        e->set_layer(this->get_layer());
                        G->emit(e, this, b2Vec2(0, 0));
                    } else {
                        break;
                    }
                }
            }
            break;
    }
#undef rx
#undef ry
}

void
treasure_chest::emit_contents()
{
    tms_debugf("Emitting contents of treasure chest");

    char *str = strdup(this->properties[0].v.s.buf);

    std::vector<struct treasure_chest_item> items = treasure_chest::parse_items(str);

    free(str);

    G->lock();

    for (std::vector<struct treasure_chest_item>::iterator it = items.begin();
            it != items.end(); ++it) {
        this->emit_item(*it);
    }
    G->unlock();
}

void
treasure_chest::on_touch(b2Fixture *my, b2Fixture *other)
{
    if (my == this->fx_sensor) {
        this->activator_touched(other);
    }
}

void
treasure_chest::on_untouch(b2Fixture *my, b2Fixture *other)
{
    if (my == this->fx_sensor) {
        this->activator_untouched(other);
    }
}
