#include "repair_station.hh"
#include "model.hh"
#include "world.hh"
#include "object_factory.hh"
#include "creature.hh"
#include "fxemitter.hh"
#include "settings.hh"
#include "ui.hh"
#include "game.hh"
#include "item.hh"
#include "robot_parts.hh"

struct eq_slot {
    int x;
    int y;
    uint32_t category;
    bool compat; /* if this slot is compatible with the current creature */
    struct rs_item *item;
};

static struct eq_slot eq_slots[] = {
    { 1, 0, ITEM_CATEGORY_HEAD }, /* DO NOT CHANGE ORDER WITHOUT FIXING UNEQUIP OF HEAD BELOW BLAH JSKDLA AND OTHER MISC SHKFJLA */
    { 1, 1, ITEM_CATEGORY_LOOSE_HEAD },
    { 1, 2, ITEM_CATEGORY_BACK },
    { 1, 3, ITEM_CATEGORY_FRONT },
    { 1, 4, ITEM_CATEGORY_FEET },

    { 2, 0, ITEM_CATEGORY_BOLT_SET },

    { 3, 0, ITEM_CATEGORY_CIRCUIT },
    { 3, 1, ITEM_CATEGORY_CIRCUIT },
    { 3, 2, ITEM_CATEGORY_CIRCUIT },
    { 3, 3, ITEM_CATEGORY_CIRCUIT },
};

#define NUM_EQ_SLOTS (sizeof(eq_slots)/sizeof(struct eq_slot))

static int inventory_top = 0;
static int inventory_width = 0;
static int screen_padding = 0;
static bool down[MAX_P];
static bool dragging[MAX_P];
static bool sliding[MAX_P];
static tvec2 start[MAX_P];
static int begin_offset = 0;
static int dragging_id = -1;
static int highlight_id = -1;
static int hover_x = -1;
static int hover_y = -1;
static int inventory_row_h;
static int inventory_item_w;
static int inventory_item_h;
static int padding_x;
static int padding_y;
static int inner_padding_x;
static int inner_padding_y;

struct rs_item *rs_item_alloc(uint32_t gid, uint32_t item_type, uint32_t category)
{
    struct rs_item *i = (struct rs_item*)calloc(1, sizeof(struct rs_item));

    i->g_id = gid;
    i->item_type = item_type;
    i->category = category;

    return i;
}

static void
refresh_highlight(tvec2 sp)
{
    if (sp.x < inventory_width) {
        highlight_id = floorf((_tms.window_height + inventory_top - padding_y - sp.y) / (inventory_row_h+padding_y));
    } else {
        highlight_id = -1;
    }
}

static void
equip(int slot_id, repair_station *rs, int inventory_id)
{
    if (slot_id == 0) { /* equipping headwear, require head */
        if (!eq_slots[1].item) {
            return;
        }
    }
    if (eq_slots[slot_id].item) {
        std::swap(eq_slots[slot_id].item, rs->inventory[inventory_id]);
    } else {
        eq_slots[slot_id].item = (struct rs_item*)malloc(sizeof(struct rs_item));
        memcpy(eq_slots[slot_id].item, rs->inventory[inventory_id], sizeof(struct rs_item));
        rs->inventory.erase(rs->inventory.begin() + inventory_id);
    }
}

repair_station::repair_station()
    : activator(ATTACHMENT_NONE)
    , sensor_radius(1.25f)
    , sensor_offset(-0.25f, 2.75f)
    , ladder_ud2(UD2_CLIMBABLE)
    , ladder_step_ud2(UD2_LADDER_STEP)
{
    this->set_flag(ENTITY_IS_BETA,              true);
    this->set_flag(ENTITY_IS_LOW_PRIO,          true);
    this->set_flag(ENTITY_ALLOW_CONNECTIONS,    false);
    this->set_flag(ENTITY_DO_STEP,              true);
    this->set_flag(ENTITY_DO_UPDATE_EFFECTS,    true);
    this->set_flag(ENTITY_HAS_ACTIVATOR,        true);
    this->set_flag(ENTITY_CAN_BE_GRABBED,       false);
    this->set_flag(ENTITY_DYNAMIC_UNLOADING,    true);

    this->menu_scale = 0.3f;

    this->update_method = ENTITY_UPDATE_CUSTOM;

    this->set_mesh(mesh_factory::get_mesh(MODEL_REPAIR_STATION));
    this->set_material(&m_repairstation);

    this->width  = 2.35f;
    this->height = 1.45f;

    this->top_size.Set(this->width-.50f, .18f);
    this->top_offset.Set(.45f, 1.85f);
    this->bottom_size.Set(this->width, .175f);
    this->bottom_offset.Set(0.f, 0.f);

    for (int x=0; x<2; ++x) {
        this->side_doors[x] = tms_entity_alloc();
        this->front_doors[x] = tms_entity_alloc();
        tms_entity_set_mesh(this->side_doors[x], const_cast<struct tms_mesh*>(tms_meshfactory_get_cube()));
        tms_entity_set_mesh(this->front_doors[x], const_cast<struct tms_mesh*>(tms_meshfactory_get_cube()));
        tms_entity_set_material(this->side_doors[x],  &m_edev);
        tms_entity_set_material(this->front_doors[x], &m_pv_rgba);

        tms_entity_add_child(this, this->side_doors[x]);
        tms_entity_add_child(this, this->front_doors[x]);
    }

    tms_entity_set_uniform4f(this->front_doors[0], "~color", 1.0f, 1.0f, 1.0f, .3f);
    tms_entity_set_uniform4f(this->front_doors[1], "~color", 1.0f, 1.0f, 1.0f, .3f);

    this->side_door_state = 0.;
    this->front_door_state = 0.f;
    this->repair_state = RS_PENDING;
    this->repair_target_id = 0;
    this->down_pid = 0;
    this->down_step = 0;

    /*
    this->s_in[0].lpos = b2Vec2(-.6f, this->height-.225f);
    this->s_in[1].lpos = b2Vec2(-.3f, this->height-.225f);
    this->s_in[2].lpos = b2Vec2( .0f, this->height-.225f);
    this->s_in[3].lpos = b2Vec2( .3f, this->height-.225f);
    this->s_in[4].lpos = b2Vec2( .6f, this->height-.225f);

    this->s_out[0].lpos = b2Vec2(-.6f, this->height-.525f);
    this->s_out[1].lpos = b2Vec2(-.3f, this->height-.525f);
    this->s_out[2].lpos = b2Vec2( .0f, this->height-.525f);
    this->s_out[3].lpos = b2Vec2( .3f, this->height-.525f);
    this->s_out[4].lpos = b2Vec2( .6f, this->height-.525f);
    */

}

#define X_DISPL      .450f
#define X_OFFSET     1.800f
#define Y_SCALE      2.75f
#define Y_TRANSLATE  0.275f
#define Z_COOL       0.350f

void
repair_station::update()
{
    //tmat4_load_identity(this->M);
    entity_fast_update(static_cast<struct tms_entity*>(this));

    // Side doors
    for (int x=0; x<2; ++x) {
        tmat4_copy(this->side_doors[x]->M, this->M);
        tmat3_copy_mat4_sub3x3(this->side_doors[x]->N,  this->side_doors[x]->M);

        tmat4_translate(this->side_doors[x]->M,
                (x==0?-1.f:1.f) * X_OFFSET + X_DISPL,
                Y_TRANSLATE,
                -.2f + (((Z_COOL+.175f)/2.f) * this->side_door_state));

        tmat4_scale(this->side_doors[x]->M,
                .1f,
                Y_SCALE,
                .05f + (Z_COOL+.175f) * this->side_door_state);
    }

    // Front doors
    if (this->front_door_state > .0f) {
        for (int x=0; x<2; ++x) {
            tmat4_copy(this->front_doors[x]->M, this->M);
            tmat3_copy_mat4_sub3x3(this->front_doors[x]->N, this->front_doors[x]->M);

            tmat4_translate(this->front_doors[x]->M,
                    X_DISPL + (x==0?-1.f:1.f) * (X_OFFSET-.05f) * (1.f - (this->front_door_state/2.f)),
                    Y_TRANSLATE,
                    Z_COOL);

            tmat4_scale(this->front_doors[x]->M,
                    ((X_OFFSET-.05f)*2.f) * (this->front_door_state/2.f),
                    Y_SCALE,
                    .1f);
        }
    } else {
        for (int x=0; x<2; ++x) {
            tmat4_scale(this->front_doors[x]->M, 0.f, 0.f, 0.f);
        }
    }
}

void
repair_station::update_effects()
{

}

#undef X_OFFSET
#undef Y_SCALE
#undef Y_TRANSLATE

void
repair_station::add_to_world()
{
    this->bottom = 0;

    b2PolygonShape box;
    b2BodyDef bd;
    b2FixtureDef fd;

    bd.type = b2_dynamicBody;
    bd.position = _pos;
    bd.angle = _angle;

    this->body = W->b2->CreateBody(&bd);

    fd.shape = &box;
    fd.density = 1.0f;
    fd.friction = .5f;
    fd.restitution = .3f;
    fd.filter = world::get_filter_for_layer(this->get_layer(), 15);

    b2Vec2 verts[4] = {
        b2Vec2( (this->width+.75f), -this->height),
        b2Vec2(-(this->width+.75f), -this->height),
        b2Vec2( (this->width),      -this->height+(this->bottom_size.y*2.f)),
        b2Vec2(-(this->width),      -this->height+(this->bottom_size.y*2.f)),
    };

    /* bottom */
    box.Set(verts, 4);
    (this->bottom = (this->body->CreateFixture(&fd)))->SetUserData(this);

    /* top */
    box.SetAsBox(this->top_size.x, this->top_size.y,
            b2Vec2(this->top_offset.x, this->top_offset.y), 0);
    (this->body->CreateFixture(&fd))->SetUserData(this);

    /* ladder */
    box.SetAsBox(.35f, 1.85f, b2Vec2(-1.80f, 0.75f), 0);
    fd.filter = world::get_filter_for_layer(this->get_layer(), 1);
    (this->fx_ladder = this->body->CreateFixture(&fd))->SetUserData(this);
    this->fx_ladder->SetUserData2(&this->ladder_ud2);

    /* ladder step */
    box.SetAsBox(.35f, 0.1f, b2Vec2(-1.80f, 1.95f), 0);
    fd.filter = world::get_filter_for_layer(this->get_layer(), 1);
    (this->fx_ladder_step = this->body->CreateFixture(&fd))->SetUserData(this);
    this->fx_ladder_step->SetUserData2(&this->ladder_step_ud2);

    /* "control panel" */
    box.SetAsBox(.12f, 0.55f, b2Vec2(-.3f, top_offset.y+.75f), 0);
    fd.filter = world::get_filter_for_layer(this->get_layer(), 15);
    (this->fx_control = this->body->CreateFixture(&fd))->SetUserData(this);

    /* detection sensor */
    fd.isSensor = true;
    fd.filter = world::get_filter_for_layer(this->get_layer(), 15);
    box.SetAsBox(this->width-.25f, this->height-this->bottom_size.y-this->top_size.y, b2Vec2(0,(this->bottom_size.y-this->top_size.y)), 0);
    (this->detection_sensor = this->body->CreateFixture(&fd))->SetUserData(this);

    /* absorb sensor */
    box.SetAsBox(top_size.x-.5f, .25f, b2Vec2(top_offset.x+.5f, top_offset.y+.25f), 0);
    (this->absorb_sensor = this->body->CreateFixture(&fd))->SetUserData(this);

    if (W->is_playing()) {
        b2CircleShape circle;
        circle.m_radius = this->sensor_radius;
        circle.m_p = this->local_to_body(this->sensor_offset, 0);

        b2FixtureDef sensor_fd;
        sensor_fd.isSensor = true;
        sensor_fd.restitution = 0.f;
        sensor_fd.friction = FLT_EPSILON;
        sensor_fd.density = 0.00001f;
        sensor_fd.filter = world::get_filter_for_layer(this->get_layer(), 15);
        sensor_fd.shape = &circle;

        (this->activator_sensor = this->body->CreateFixture(&sensor_fd))->SetUserData(this);
    }

    this->body->SetSleepingAllowed(false);
}

void
repair_station::init()
{
    this->targets.clear();
}

void
repair_station::read_state(lvlinfo *lvl, lvlbuf *lb)
{
    this->repair_state = lb->r_uint8();
    this->repair_target_id = lb->r_id();
    this->side_door_state = lb->r_float();
    this->front_door_state = lb->r_float();

    /* read inventory */
    uint32_t num_inv = lb->r_uint32();
    for (uint32_t x=0; x<num_inv; x++) {
        struct rs_item *i = (struct rs_item*)calloc(1, sizeof(struct rs_item));

        i->g_id = lb->r_uint32();
        i->item_type = lb->r_uint32();
        i->category = lb->r_uint32();

        this->inventory.push_back(i);
    }
}

void
repair_station::write_state(lvlinfo *lvl, lvlbuf *lb)
{
    lb->w_s_uint8(this->repair_state);
    lb->w_s_id(this->repair_target_id);
    lb->w_s_float(this->side_door_state);
    lb->w_s_float(this->front_door_state);

    /* write inventory */
    uint32_t num_inv = (uint32_t)this->inventory.size();
    lb->w_s_uint32(num_inv);

    lb->ensure(num_inv*3*sizeof(uint32_t));
    for (uint32_t x=0; x<num_inv; x++) {
        lb->w_uint32(this->inventory[x]->g_id);
        lb->w_uint32(this->inventory[x]->item_type);
        lb->w_uint32(this->inventory[x]->category);
    }
}


void
repair_station::restore()
{
    entity::restore();
}

void
repair_station::setup()
{
    this->repair_state = RS_PENDING;
    this->repair_target_id = 0;
    this->side_door_state = 0.f;
    this->front_door_state = 0.f;
}

void
repair_station::on_pause()
{
    this->setup();
}

void
repair_station::on_touch(b2Fixture *my, b2Fixture *other)
{
    if (other->IsSensor()) return;

    entity *e;

    if (my == this->activator_sensor) {
        this->activator_touched(other);
    } else if (my == this->absorb_sensor) {
        if ((e = static_cast<entity*>(other->GetUserData()))) {
            switch (e->g_id) {
                case O_ITEM:
                    {
                        uint32_t item_type = ((item*)e)->get_item_type();
                        uint32_t item_category = ((item*)e)->item_category;
                        struct rs_item *i;
                        if (!(i = (struct rs_item*)calloc(1, sizeof(struct rs_item)))) {
                            return;
                        }

                        if (item_category != ITEM_CATEGORY_HEAD
                            && item_category != ITEM_CATEGORY_BACK
                            && item_category != ITEM_CATEGORY_FEET
                            && item_category != ITEM_CATEGORY_CIRCUIT
                            && item_category != ITEM_CATEGORY_LOOSE_HEAD
                            && item_category != ITEM_CATEGORY_FRONT
                            && item_category != ITEM_CATEGORY_BOLT_SET
                            ) {
                            return;
                        }

                        if (G->absorb(e)) {
                            i->g_id = e->g_id;
                            i->item_type = item_type;
                            i->category = item_category;

                            G->finished_tt(TUTORIAL_REPAIR_STATION_DROP);
                            G->close_tt(TUTORIAL_TEXT_REPAIR_STATION_DROP);

                            this->inventory.push_back(i);
                        }
                    }
                    break;
            }
        }
    } else if (my == this->detection_sensor) {
        if ((e = static_cast<entity*>(other->GetUserData()))) {
            if (e->flag_active(ENTITY_IS_CREATURE) && (other == static_cast<creature*>(e)->get_body_fixture())
                && e->id != this->repair_target_id) {
                this->targets.insert(e->id);
            }
        }
    }
}

void
repair_station::on_untouch(b2Fixture *my, b2Fixture *other)
{
    if (other->IsSensor()) return;

    if (my == this->activator_sensor) {
        this->activator_untouched(other);
    } else if (my == this->detection_sensor) {
        entity *e;

        if ((e = static_cast<entity*>(other->GetUserData()))) {
            if (e->flag_active(ENTITY_IS_CREATURE) && (other == static_cast<creature*>(e)->get_body_fixture())) {
                this->targets.erase(e->id);
            }
        }
    }
}

void
repair_station::on_target_disappear()
{
    tms_debugf("target absorbed?");
    this->repair_target_id = 0;
    this->repair_state = RS_OPEN_DOORS;
}

void
repair_station::step()
{
    creature *repair_target = 0;

    if (this->repair_target_id != 0) {
        repair_target = static_cast<creature*>(W->get_entity_by_id(this->repair_target_id));
    }

    if (!repair_target) {
        if (this->repair_target_id != 0) {
            this->on_target_disappear();
        } else {
            this->repair_target_id = 0;
            this->repair_state = RS_PENDING;
        }
    }

    switch (this->repair_state) {
        case RS_PENDING:
            this->repair_target_id = 0;
            /* Pending, wait for a creature to step into the detection sensor */
            for (std::set<uint32_t>::iterator it = this->targets.begin();
                    it != this->targets.end(); ++it) {
                creature *t = static_cast<creature*>(W->get_entity_by_id(*it));

                if (!t) {
                    this->targets.erase(it);
                    break;
                }
                float tangent_dist = t->get_tangent_distance(this->get_position());

                if (std::abs(tangent_dist) < .3f && (!t->is_player() || (!t->is_moving_right() && !t->is_moving_left()))) {
                    tms_debugf("Begin repairing %p [%s], tdist: %.2f", t, t->get_name(), tangent_dist);
                    this->repair_state ++;
                    this->repair_target_id = t->id;
                    this->targets.erase(it);
                    break;
                }
            }
            break;

        case RS_PRE_REPAIR:
            if (true/* || repair_target->needs_repair*/) {
                tms_debugf("Pre-repair %p [%s]", repair_target, repair_target->get_name());
                repair_target->stop();

                if (repair_target->flag_active(ENTITY_IS_ROBOT)) {
                    repair_target->look(DIR_FORWARD);
                }

                repair_target->set_creature_flag(CREATURE_FROZEN, true);
                repair_target->set_creature_flag(CREATURE_MOVING_LEFT, false);
                repair_target->set_creature_flag(CREATURE_MOVING_RIGHT, false);
                repair_target->set_creature_flag(CREATURE_MOVING_UP, false);
                repair_target->set_creature_flag(CREATURE_MOVING_DOWN, false);
                this->repair_state ++;
            } else {
                this->repair_state = RS_PENDING;
            }
            break;

        case RS_WAIT_FOR_ROBOT:
            {
                float tangent_dist = repair_target->get_tangent_distance(this->local_to_world(b2Vec2(.45f, .275f), 0));
                if (std::abs(tangent_dist) > .02f) {
                    repair_target->apply_effect(SLOWDOWN_ID, *robot_base::slowdown_effect);
                    if (tangent_dist < 0.f) {
                        repair_target->move(DIR_LEFT, true);
                    } else if (tangent_dist) {
                        repair_target->move(DIR_RIGHT, true);
                    }
                } else {
                    repair_target->stop();
                    if (repair_target->flag_active(ENTITY_IS_ROBOT)) {
                        repair_target->look(DIR_FORWARD);
                        if (std::abs(repair_target->i_dir) < .0125f) {
                            this->repair_state ++;
                        }
                    } else {
                        this->repair_state ++;
                    }
                }
            }
            break;

        case RS_CLOSE_DOORS:
            if (this->side_door_state < 1.f) {
                this->side_door_state += 0.0175f;
            } else if (this->side_door_state >= 1.f) {
                this->side_door_state = 1.f;
                if (this->front_door_state < 1.f) {
                    this->front_door_state += 0.0175f;
                } else if (this->front_door_state >= 1.f) {
                    this->front_door_state = 1.f;
                    this->repair_state ++;
                }
            }
            break;

        case RS_CHECK_COSTS:
            {
                /* automatically open the repair dialog for the player */
                bool do_open = (W->is_adventure() && repair_target->is_player());

                if (do_open) {
                    if (G->get_mode() == GAME_MODE_REPAIR_STATION) {
                        tms_debugf("another repair station is already open! :(");
                    } else {
                        this->load_equipments(repair_target);
                        G->sel_p_ent = this;
                        G->set_mode(GAME_MODE_REPAIR_STATION);
                    }
                }

                this->repair_time = 200;
                this->repair_elapsed = 0;
                this->repair_state ++;
            }

            /* TODO: Check if the creature can be repaired? */
#if 0
            if (false/* ||  not enough materials in repair station */) {
                this->repair_state = RS_OPEN_DOORS;
            } else {
                // we have enough materials to begin repairing
            }
#endif
            break;

        case RS_BEGIN_REPAIRS:
            if ((G->get_mode() != GAME_MODE_REPAIR_STATION || G->sel_p_ent != this)
                 && this->repair_elapsed >= this->repair_time) {
                this->repair_state ++;
            } else {
                this->repair_elapsed ++;
                if (rand()%(int)(60.f/G->get_time_mul()) == 0) {
                    G->emit(new spark_effect(
                                this->local_to_world(b2Vec2(-.5f + (rand()%100) / 100.f, -.5f + (rand()%100) / 100.f), 0),
                                this->get_layer()
                                ), 0);
                    G->emit(new smoke_effect(
                                this->local_to_world(b2Vec2(-.5f + (rand()%100) / 100.f, -.5f + (rand()%100) / 100.f), 0),
                                this->get_layer(),
                                .5f,
                                .5f
                                ), 0);
                }
            }
            break;

        case RS_OPEN_DOORS:
            if (this->front_door_state > 0.f) {
                this->front_door_state -= .0175f;
            } else if (this->front_door_state <= 0.f) {
                this->front_door_state = 0.f;
                if (this->side_door_state > 0.f) {
                    this->side_door_state -= .0175f;
                } else if (this->side_door_state <= 0.f) {
                    this->side_door_state = 0.f;
                    this->repair_state ++;
                }
            }
            break;

        case RS_EJECT_ROBOT:
            if (repair_target) {
                repair_target->set_creature_flag(CREATURE_FROZEN, false);
                repair_target->set_creature_flag(CREATURE_MOVING_LEFT, false);
                repair_target->set_creature_flag(CREATURE_MOVING_RIGHT, false);
                repair_target->set_creature_flag(CREATURE_MOVING_UP, false);
                repair_target->set_creature_flag(CREATURE_MOVING_DOWN, false);
                this->repair_state ++;
            }
            break;

        case RS_END:
            this->repair_state = RS_PENDING;
            this->repair_target_id = 0;
            break;

        default:
            tms_debugf("Unhandled repair state: %d", this->repair_state);
            break;
    }
}

static void
swapify(struct rs_item **a, struct rs_item **b)
{
    struct rs_item *tmp = *a;
    *a = *b;
    *b = tmp;
}

void
game::render_repair_station(void)
{
    if (!this->sel_p_ent || this->sel_p_ent->g_id != O_REPAIR_STATION) {
        this->set_mode(GAME_MODE_DEFAULT);
        return;
    }

    repair_station *rs = static_cast<repair_station*>(this->sel_p_ent);

    inventory_row_h  = _tms.yppcm * .65f;
    inner_padding_x  = _tms.xppcm * .1f;
    inner_padding_y  = _tms.yppcm * .1f;
    inventory_item_w = _tms.xppcm * .65f - inner_padding_x;
    inventory_item_h = _tms.yppcm * .65f - inner_padding_y;
    padding_x = _tms.xppcm * 0.15f;
    padding_y = _tms.yppcm * 0.15f;

    int num_items = rs->inventory.size();
    int max_y = ((inventory_row_h+padding_y) * (num_items)) - _tms.window_height + padding_y;
    if (max_y < 0) max_y = 0;

    if (inventory_top < 0) inventory_top = 0;
    else if (inventory_top > max_y) inventory_top = max_y;

    inventory_width = _tms.xppcm * 4.f;
    screen_padding = _tms.xppcm/2.5f;
    // rest of screen, i.e. the equipment "boxes" will take window_width-inventory_width-padding

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    glActiveTexture(GL_TEXTURE0);

    tms_ddraw_set_color(this->get_surface()->ddraw, .0f, 0.f, .0f, .75f);
    tms_ddraw_square(this->get_surface()->ddraw, _tms.window_width/2.f, _tms.window_height/2.f,
            _tms.window_width, _tms.window_height);

    /* draw inventory background */
    tms_ddraw_set_color(this->get_surface()->ddraw, .1f, .1f, .1f, .75f);
    tms_ddraw_square(this->get_surface()->ddraw, inventory_width/2.f, _tms.window_height/2.f,
            inventory_width, _tms.window_height);

    /* draw "rest of screen" background */
    tms_ddraw_set_color(this->get_surface()->ddraw, .1f, .1f, .1f, .75f);
    tms_ddraw_square(this->get_surface()->ddraw, (_tms.window_width+inventory_width+screen_padding)/2.f, _tms.window_height/2.f,
            (_tms.window_width-inventory_width-screen_padding), _tms.window_height);

    /* draw eq slot flat stuff */
    for (int i=0; i<NUM_EQ_SLOTS; ++i) {
        float base_x = inventory_width + screen_padding*2.f;
        float base_y = _tms.window_height - (inventory_row_h/2.f) - padding_y;

        struct rs_item *rsi = 0;
        if (!eq_slots[i].compat) {
            tms_ddraw_set_color(this->get_surface()->ddraw, MENU_GRAY_FI, .5f);
        } else {
            if (dragging_id >= 0 && dragging_id < rs->inventory.size()) {
                if ((rsi = rs->inventory.at(dragging_id)) && rsi->category == eq_slots[i].category) {
                    tms_ddraw_set_color(this->get_surface()->ddraw, .7f, 1.f, .7f, .5f);
                } else {
                    tms_ddraw_set_color(this->get_surface()->ddraw, 1.0f, .7f, .7f, .5f);
                }
            } else {
                tms_ddraw_set_color(this->get_surface()->ddraw, MENU_WHITE_FI, .5f);
            }
        }

        tms_ddraw_square(this->get_surface()->ddraw,
                base_x + ((inventory_item_w + padding_x) * eq_slots[i].x),
                base_y - ((inventory_item_h + padding_y) * eq_slots[i].y),
                inventory_item_w,
                inventory_item_h);
    }

    float y = _tms.window_height - (inventory_row_h/2.f) - padding_y + inventory_top;
    /* draw flat shit */
    int n = 0;
    for (std::vector<struct rs_item*>::iterator i = rs->inventory.begin();
            i != rs->inventory.end(); ++i, ++n) {
        if (dragging_id == n) {
            tms_ddraw_set_color(this->get_surface()->ddraw, .0f, .0f, .0f, .80f);
            tms_ddraw_square(this->get_surface()->ddraw,
                    inventory_width/2.f,
                    y,
                    (inventory_width) - padding_x,
                    inventory_row_h);
        } else {
            if (highlight_id == n) {
                if (down[0] || down[1]) {
                    tms_ddraw_set_color(this->get_surface()->ddraw, .65f, .65f, .65f, .85f);
                } else {
                    tms_ddraw_set_color(this->get_surface()->ddraw, .6f, .6f, .6f, .80f);
                }
                tms_ddraw_square(this->get_surface()->ddraw,
                        inventory_width/2.f,
                        y,
                        (inventory_width) - padding_x,
                        inventory_row_h);
            }

            // item box
            tms_ddraw_set_color(this->get_surface()->ddraw, 1.f, 1.f, 1.f, .5f);
            tms_ddraw_square(this->get_surface()->ddraw,
                    (inventory_item_w/2.f) + (padding_x/2.f) + inner_padding_x,
                    y,
                    inventory_item_w,
                    inventory_item_h);
        }

        y -= inventory_row_h + padding_y;
    }

    tms_ddraw_set_color(this->get_surface()->ddraw, 1.f, 1.f, 1.f, 1.f);

    y = _tms.window_height - (inventory_row_h/2.f) - padding_y + inventory_top;
    int last_bind = -1;
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    /* draw eq sprites */
    for (int i=0; i<NUM_EQ_SLOTS; ++i) {
        if (!eq_slots[i].item) continue;
        float base_x = inventory_width + screen_padding*2.f;
        float base_y = _tms.window_height - (inventory_row_h/2.f) - padding_y;

        struct rs_item *rsi = eq_slots[i].item;
        struct tms_sprite *img = 0;
        GLuint gltex;

        if (rsi->g_id == O_ITEM) {
            img = &item_options[rsi->item_type].image;
            gltex = this->get_item_texture()->gl_texture;
        }

        if (img) {
            if (gltex != last_bind) {
                last_bind = gltex;
                glBindTexture(GL_TEXTURE_2D, gltex);
            }

            tms_ddraw_sprite(this->get_surface()->ddraw, img,
                    base_x + ((inventory_item_w + padding_x) * eq_slots[i].x),
                    base_y - ((inventory_item_h + padding_y) * eq_slots[i].y),
                    inventory_item_w,
                    inventory_item_h);
        }
    }

    /* draw item sprite */
    for (std::vector<struct rs_item*>::iterator i = rs->inventory.begin();
            i != rs->inventory.end(); ++i) {
        struct rs_item *rsi = *i;
        struct tms_sprite *img = 0;
        GLuint gltex;

        if (rsi->g_id == O_ITEM) {
            img = &item_options[rsi->item_type].image;
            gltex = this->get_item_texture()->gl_texture;
        }

        if (img) {
            if (gltex != last_bind) {
                last_bind = gltex;
                glBindTexture(GL_TEXTURE_2D, gltex);
            }

            if (rsi->dragging) {
                tms_ddraw_sprite(this->get_surface()->ddraw, img,
                        rsi->pos.x,
                        rsi->pos.y,
                        inventory_item_w,
                        inventory_item_h);
            } else {
                tms_ddraw_sprite(this->get_surface()->ddraw, img,
                        (inventory_item_w/2.f) + padding_x,
                        y,
                        inventory_item_w,
                        inventory_item_h);
            }

        }

        y -= inventory_row_h + padding_y;
    }

    y = _tms.window_height - (inventory_row_h/2.f) - padding_y + inventory_top;
    glBindTexture(GL_TEXTURE_2D, this->texts->texture.gl_texture);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    /* draw item name sprite */
    for (std::vector<struct rs_item*>::iterator i = rs->inventory.begin();
            i != rs->inventory.end(); ++i) {
        struct tms_sprite *img = 0;
        struct rs_item *rsi = *i;

        if (rsi->g_id == O_ITEM) {
            img = item_options[rsi->item_type].name_spr;
        }

        if (img) {
            tms_ddraw_sprite(this->get_surface()->ddraw, img,
                    inventory_item_w + padding_x + (img->width / 2.f) + inner_padding_x,
                    y,
                    img->width,
                    img->height);
        }

        y -= inventory_row_h + padding_y;
    }

    glDisable(GL_BLEND);
}

void
repair_station::clear_slots()
{
    for (int x=0; x<NUM_EQ_SLOTS; x++) {
        if (eq_slots[x].item) {
            free(eq_slots[x].item);
            eq_slots[x].item = 0;
        }
    }
}

void
repair_station::load_equipments(creature *repair_target)
{
    this->clear_slots();

    uint32_t circuit_counter = 0;

    for (int x=0; x<NUM_EQ_SLOTS; x++) {
        eq_slots[x].compat = false;

        uint32_t item_id = ITEM_INVALID;

        switch (eq_slots[x].category) {
            case ITEM_CATEGORY_LOOSE_HEAD:
                eq_slots[x].compat = (repair_target->has_feature(CREATURE_FEATURE_HEAD));
                if (repair_target->equipments[EQUIPMENT_HEAD]) {
                    item_id = repair_target->equipments[EQUIPMENT_HEAD]->get_item_id();
                }
                break;

            case ITEM_CATEGORY_HEAD:
                eq_slots[x].compat = (repair_target->has_feature(CREATURE_FEATURE_HEAD));
                if (repair_target->equipments[EQUIPMENT_HEADWEAR]) {
                    item_id = repair_target->equipments[EQUIPMENT_HEADWEAR]->get_item_id();
                }
                break;

            case ITEM_CATEGORY_BACK:
                eq_slots[x].compat = (repair_target->has_feature(CREATURE_FEATURE_BACK_EQUIPMENT));
                if (repair_target->equipments[EQUIPMENT_BACK]) {
                    item_id = repair_target->equipments[EQUIPMENT_BACK]->get_item_id();
                }
                break;

            case ITEM_CATEGORY_FRONT:
                eq_slots[x].compat = (repair_target->has_feature(CREATURE_FEATURE_FRONT_EQUIPMENT));
                if (repair_target->equipments[EQUIPMENT_FRONT]) {
                    item_id = repair_target->equipments[EQUIPMENT_FRONT]->get_item_id();
                }
                break;

            case ITEM_CATEGORY_FEET:
                eq_slots[x].compat = true;

                if (repair_target->equipments[EQUIPMENT_FEET]) {
                    item_id = repair_target->equipments[EQUIPMENT_FEET]->get_item_id();
                }
                break;

            case ITEM_CATEGORY_CIRCUIT:
                while (circuit_counter < 31) {
                    uint32_t flag = (1<<circuit_counter);
                    circuit_counter ++;
                    if (repair_target->circuits & flag) {
                        item_id = _circuit_flag_to_item(flag);
                        break;
                    }
                }
                eq_slots[x].compat = true;
                break;

            case ITEM_CATEGORY_BOLT_SET:
                eq_slots[x].compat = true;
                item_id = _bolt_to_item[repair_target->bolt_set];
                break;
        }

        if (item_id != ITEM_INVALID) {
            item_option *i = &item_options[item_id];

            eq_slots[x].item = rs_item_alloc(
                    O_ITEM,
                    item_id,
                    i->category
                    );
        }
    }
}

void
repair_station::apply_equipments(creature *repair_target)
{
    /* Remove any movement flags from the robot when exiting the repair station */
    uint64_t mask = ~(
              (this->id == G->state.adventure_id ? CREATURE_MOVING_LEFT : 0)
            | (this->id == G->state.adventure_id ? CREATURE_MOVING_RIGHT : 0)
            | (this->id == G->state.adventure_id ? CREATURE_MOVING_UP : 0)
            | (this->id == G->state.adventure_id ? CREATURE_MOVING_DOWN : 0)
            );
    repair_target->creature_flags &= mask;

    repair_target->circuits = 0llu;

    for (int x=0; x<NUM_EQ_SLOTS; x++) {
        if (!eq_slots[x].compat) continue;

        uint32_t item_id = ITEM_INVALID;
        struct item_option *i = 0;

        tms_debugf("processing slot %d", x);

        if (eq_slots[x].item) {
            item_id = eq_slots[x].item->item_type;

            i = &item_options[item_id];
            tms_debugf("in slot: %s, item type %d", i->name, item_id);
        } else {
            tms_debugf("no item");
        }

        switch (eq_slots[x].category) {
            case ITEM_CATEGORY_LOOSE_HEAD:
                if (item_id == ITEM_INVALID) {
                    repair_target->set_equipment(EQUIPMENT_HEAD, 0);
                } else {
                    repair_target->set_equipment(EQUIPMENT_HEAD, i->data_id);
                }
                break;

            case ITEM_CATEGORY_HEAD:
                if (item_id == ITEM_INVALID) {
                    repair_target->set_equipment(EQUIPMENT_HEADWEAR, HEAD_EQUIPMENT_NULL);
                } else {
                    repair_target->set_equipment(EQUIPMENT_HEADWEAR, i->data_id);
                }
                break;

            case ITEM_CATEGORY_FRONT:
                if (item_id == ITEM_INVALID) {
                    repair_target->set_equipment(EQUIPMENT_FRONT, 0);
                } else {
                    repair_target->set_equipment(EQUIPMENT_FRONT, i->data_id);
                }
                break;

            case ITEM_CATEGORY_BACK:
                if (item_id == ITEM_INVALID) {
                    repair_target->set_equipment(EQUIPMENT_BACK, 0);
                } else {
                    repair_target->set_equipment(EQUIPMENT_BACK, i->data_id);
                }
                break;

            case ITEM_CATEGORY_FEET:
                if (item_id == ITEM_INVALID) {
                    repair_target->set_equipment(EQUIPMENT_FEET, 0);
                } else {
                    repair_target->set_equipment(EQUIPMENT_FEET, i->data_id);
                }
                break;

            case ITEM_CATEGORY_CIRCUIT:
                if (item_id != ITEM_INVALID) {
                    repair_target->set_has_circuit(i->data_id, true);
                }
                break;

            case ITEM_CATEGORY_BOLT_SET:
                if (item_id != ITEM_INVALID) {
                    repair_target->set_bolt_set(i->data_id);
                }
                break;
        }
    }

    this->clear_slots();
}

int
game::repair_station_handle_event(tms::event *ev)
{
    if (!this->sel_p_ent || this->sel_p_ent->g_id != O_REPAIR_STATION) {
        this->set_mode(GAME_MODE_DEFAULT);
        return EVENT_DONE;
    }

    repair_station *rs = static_cast<repair_station*>(this->sel_p_ent);

    creature *repair_target = static_cast<creature*>(W->get_entity_by_id(rs->repair_target_id));

    if (!repair_target) {
        this->set_mode(GAME_MODE_DEFAULT);
        return EVENT_DONE;
    }

    switch (ev->type) {
        case TMS_EV_KEY_PRESS:
            switch (ev->data.key.keycode) {
                case TMS_KEY_ESC:
                case TMS_KEY_B:
                case SDL_SCANCODE_AC_BACK:
                    rs->apply_equipments(repair_target);
                    this->set_mode(GAME_MODE_DEFAULT);
                    return EVENT_DONE;

                default:
                    return EVENT_DONE;
            }
            break;

        case TMS_EV_POINTER_DOWN:
            {
                int pid = ev->data.motion.pointer_id;
                tvec2 sp = (tvec2){ev->data.motion.x, ev->data.motion.y};
                rs->down_step = W->step_count;

                down[pid] = true;
                start[pid] = sp;
            }
            break;

        case TMS_EV_POINTER_DRAG:
            {
                int pid = ev->data.motion.pointer_id;
                tvec2 sp = (tvec2){ev->data.motion.x, ev->data.motion.y};

                if (down[pid]) {
                    if (start[pid].x < inventory_width) {
                        if (!dragging[pid] && !sliding[pid]) {
                            float xdiff = sp.x - start[pid].x;
                            float ydiff = std::abs(sp.y - start[pid].y);
                            if (xdiff > _tms.xppcm) {
                                dragging_id = floorf((_tms.window_height + inventory_top - start[pid].y) / (inventory_row_h+padding_y));
                                dragging[pid] = true;
                                highlight_id = -1;
                                if (dragging_id >= 0 && dragging_id < rs->inventory.size()) {
                                    struct rs_item *rsi = rs->inventory.at(dragging_id);
                                    rsi->dragging = true;
                                }
                            } else if (ydiff > _tms.yppcm) {
                                sliding[pid] = true;
                                begin_offset = inventory_top;
                            }
                        } else if (dragging[pid]) {
                            if (dragging_id >= 0 && dragging_id < rs->inventory.size()) {
                                struct rs_item *rsi = rs->inventory.at(dragging_id);
                                rsi->pos = sp;

                                /* check if we're hovering over an item slot */
                                hover_x = floorf((sp.x - (inventory_width + (screen_padding*1.f))) / (inventory_item_w + padding_x));
                                hover_y = floorf((_tms.window_height - padding_y - sp.y) / (inventory_item_h+padding_y));
                            }
                        } else if (sliding[pid]) {
                            inventory_top = begin_offset+sp.y-start[pid].y;
                        }
                    }
                }
            }
            break;

        case TMS_EV_POINTER_MOVE:
            {
                tvec2 sp = (tvec2){ev->data.motion.x, ev->data.motion.y};

                refresh_highlight(sp);
            }
            break;

        case TMS_EV_POINTER_UP:
            {
                int pid = ev->data.motion.pointer_id;
                tvec2 sp = (tvec2){ev->data.motion.x, ev->data.motion.y};

                if (down[pid]) {
                    down[pid] = false;

                    if (dragging[pid]) {
                        struct rs_item *rsi = 0;
                        if (dragging_id >= 0 && dragging_id < rs->inventory.size()) {
                            rsi = rs->inventory[dragging_id];
                            rsi->dragging = false;
                            int release_x = floorf((sp.x - (inventory_width + (screen_padding*1.f))) / (inventory_item_w + padding_x));
                            int release_y = floorf((_tms.window_height - padding_y - sp.y) / (inventory_item_h+padding_y));
                            for (int i=0; i<NUM_EQ_SLOTS; ++i) {
                                if (eq_slots[i].x != release_x) continue;
                                if (eq_slots[i].y != release_y) continue;
                                if (eq_slots[i].category != rsi->category) continue;
                                if (!eq_slots[i].compat) continue;

                                equip(i, rs, dragging_id);


                                break;
                            }
                        }
                        dragging[pid] = false;
                        dragging_id = -1;
                        hover_x = -1;
                        hover_y = -1;

                    } else if (sliding[pid]) {
                        sliding[pid] = false;
                    } else if (pid == 0) {
                        uint32_t step_diff = W->step_count - rs->down_step;
                        if (step_diff < CLICK_MAX_STEPS) {
                            struct rs_item *rsi = 0;
                            if (highlight_id >= 0 && highlight_id < rs->inventory.size()) {
                                rsi = rs->inventory[highlight_id];
                                rsi->dragging = false;
                                int first_prio_id = -1;
                                int second_prio_id = -1;
                                for (int i=0; i<NUM_EQ_SLOTS; ++i) {
                                    if (eq_slots[i].category != rsi->category) continue;

                                    if (eq_slots[i].item) {
                                        second_prio_id = i;
                                    } else {
                                        first_prio_id = i;
                                        break;
                                    }
                                }

                                if (first_prio_id != -1) {
                                    equip(first_prio_id, rs, highlight_id);
                                } else if (second_prio_id != -1) {
                                    equip(second_prio_id, rs, highlight_id);
                                }
                            } else {
                                /* unequipping? */
                                int release_x = floorf((sp.x - (inventory_width + (screen_padding*1.f))) / (inventory_item_w + padding_x));
                                int release_y = floorf((_tms.window_height - padding_y - sp.y) / (inventory_item_h+padding_y));
                                for (int i=0; i<NUM_EQ_SLOTS; ++i) {
                                    if (eq_slots[i].x != release_x) continue;
                                    if (eq_slots[i].y != release_y) continue;
                                    if (!eq_slots[i].compat) break;

                                    /* do not unequip feet and bolt set */
                                    if (eq_slots[i].category == ITEM_CATEGORY_BOLT_SET
                                        || eq_slots[i].category == ITEM_CATEGORY_FEET) {
                                        ui::message("Can not unequip that!");
                                        break;
                                    }

                                    if (eq_slots[i].item) {
                                        if (eq_slots[i].category == ITEM_CATEGORY_LOOSE_HEAD) {
                                            /* if we're unequipping the head, force uniequip of the headwear */
                                            if (eq_slots[0].item) {
                                                rs->inventory.push_back(eq_slots[0].item);
                                                eq_slots[0].item = 0;
                                            }
                                        }
                                        rs->inventory.push_back(eq_slots[i].item);
                                        eq_slots[i].item = 0;
                                    }

                                    break;
                                }
                            }
                        }
                    }
                }

                refresh_highlight(sp);
            }
            break;

        case TMS_EV_POINTER_SCROLL:
            {
                int mx, my;
                SDL_GetMouseState(&mx, &my);

                if (mx < inventory_width) {
                    inventory_top -= (float)ev->data.scroll.y * _tms.yppcm;
                }
            }
            return EVENT_DONE;
    }
    return EVENT_DONE;
}

void
repair_station::activate(creature *by)
{
    tms_infof("repair station activate");
    creature *repair_target;

    if (repair_target_id && (repair_target = static_cast<creature*>(W->get_entity_by_id(repair_target_id)))) {
        tms_infof("we have a target!");
        this->load_equipments(repair_target);
        G->sel_p_ent = this;
        G->set_mode(GAME_MODE_REPAIR_STATION);
    } else {
        ui::message("No target creature in repair station.");
    }
}
