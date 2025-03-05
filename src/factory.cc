#include "factory.hh"
#include "material.hh"
#include "model.hh"
#include "world.hh"
#include "ui.hh"
#include "game.hh"
#include "object_factory.hh"
#include "item.hh"
#include "adventure.hh"
#include "soundmanager.hh"
#include "robot_base.hh"
#include "faction.hh"
#include "gui.hh"

static std::vector<struct factory_object> generic_objects;
static std::vector<struct factory_object> armory_objects;
static std::vector<struct factory_object> robot_objects;
static std::vector<struct factory_object> oil_mixer_objects;

static int filtered[512]; /* XXX */
static int num_filtered = 0;
static bool cat_hide[NUM_FACTORIES][of::num_categories];
static int list_cats[] = {0,1,2,3,4,5,6};
static const int num_list_cats = sizeof(list_cats)/sizeof(int);

factory::factory(int factory_type)
{
    this->factory_type = factory_type;

    this->menu_scale = .66f;
    this->set_flag(ENTITY_IS_BETA,              true);
    this->set_flag(ENTITY_IS_LOW_PRIO,          true);
    this->set_flag(ENTITY_ALLOW_CONNECTIONS,    false);
    this->set_flag(ENTITY_HAS_CONFIG,           true);
    this->set_flag(ENTITY_HAS_INGAME_CONFIG,    true);
    this->set_flag(ENTITY_DO_STEP,              true);
    this->set_flag(ENTITY_CAN_BE_GRABBED,       false);
    this->set_flag(ENTITY_DYNAMIC_UNLOADING,    true);

    this->dialog_id = DIALOG_FACTORY;

    this->set_num_properties(FACTORY_NUM_EXTRA_PROPERTIES+NUM_RESOURCES);

    this->properties[0].type = P_STR; // recipe list
    this->properties[1].type = P_FLT; // base oil
    this->properties[1].v.i = 0.f;
    this->properties[2].type = P_INT; // faction for robot factory
    this->properties[2].v.i = 0;

    for (int x=0; x<NUM_RESOURCES; ++x) {
        this->properties[FACTORY_NUM_EXTRA_PROPERTIES+x].type = P_INT;
        this->properties[FACTORY_NUM_EXTRA_PROPERTIES+x].v.i = 0;
    }

    this->num_s_in = 5;
    this->num_s_out = 5;

    switch (this->factory_type) {
        case FACTORY_ROBOT:
            this->set_mesh(mesh_factory::get_mesh(MODEL_FACTORY_ROBOT));
            this->width = 1.85f/2.f;
            this->height = 3.f/2.f;
            this->set_property(0, "0;1;2;3;4");
            this->sz_top = tvec2f(this->get_width(), .35f);
            this->sz_bottom = tvec2f(this->get_width(), .175f);

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
            break;

        case FACTORY_ARMORY:
            this->set_mesh(mesh_factory::get_mesh(MODEL_FACTORY_ARMORY));
            this->width = 1.15f/2.f;
            this->height = 1.8f/2.f;
            this->set_property(0, "0;1;2;3;4");
            this->sz_top = tvec2f(this->get_width()+.25f, .35f);
            this->sz_bottom = tvec2f(this->get_width()+.25f, .125f);

            this->s_in[0].lpos = b2Vec2(-.6f, this->height-.2f);
            this->s_in[1].lpos = b2Vec2(-.3f, this->height-.2f);
            this->s_in[2].lpos = b2Vec2( .0f, this->height-.2f);
            this->s_in[3].lpos = b2Vec2( .3f, this->height-.2f);
            this->s_in[4].lpos = b2Vec2( .6f, this->height-.2f);

            this->s_out[0].lpos = b2Vec2(-.6f, this->height-.5f);
            this->s_out[1].lpos = b2Vec2(-.3f, this->height-.5f);
            this->s_out[2].lpos = b2Vec2( .0f, this->height-.5f);
            this->s_out[3].lpos = b2Vec2( .3f, this->height-.5f);
            this->s_out[4].lpos = b2Vec2( .6f, this->height-.5f);
            break;

        case FACTORY_OIL_MIXER:
            this->set_mesh(mesh_factory::get_mesh(MODEL_FACTORY_OIL_MIXER));
            this->width = 1.15f/2.f;
            this->height = 1.8f/2.f;
            this->set_property(0, "0;1;2;3;4");
            this->sz_top = tvec2f(this->get_width()+.25f, .35f);
            this->sz_bottom = tvec2f(this->get_width()+.25f, .125f);

            this->s_in[0].lpos = b2Vec2(-.6f, this->height-.2f);
            this->s_in[1].lpos = b2Vec2(-.3f, this->height-.2f);
            this->s_in[2].lpos = b2Vec2( .0f, this->height-.2f);
            this->s_in[3].lpos = b2Vec2( .3f, this->height-.2f);
            this->s_in[4].lpos = b2Vec2( .6f, this->height-.2f);

            this->s_out[0].lpos = b2Vec2(-.6f, this->height-.5f);
            this->s_out[1].lpos = b2Vec2(-.3f, this->height-.5f);
            this->s_out[2].lpos = b2Vec2( .0f, this->height-.5f);
            this->s_out[3].lpos = b2Vec2( .3f, this->height-.5f);
            this->s_out[4].lpos = b2Vec2( .6f, this->height-.5f);
            break;

        case FACTORY_GENERIC:
        default:
            this->set_mesh(mesh_factory::get_mesh(MODEL_FACTORY_GENERIC));
            this->width = 3.1f/2.f;
            this->height = 3.15f/2.f;
            this->set_property(0, "14;15;16");
            this->sz_top = tvec2f(this->get_width(), .35f);
            this->sz_bottom = tvec2f(this->get_width(), .125f);

            this->s_in[0].lpos = b2Vec2((-this->get_width()/2.f)+.025f-.6f, this->height-.275f);
            this->s_in[1].lpos = b2Vec2((-this->get_width()/2.f)+.025f-.3f, this->height-.275f);
            this->s_in[2].lpos = b2Vec2((-this->get_width()/2.f)+.025f+.0f, this->height-.275f);
            this->s_in[3].lpos = b2Vec2((-this->get_width()/2.f)+.025f+.3f, this->height-.275f);
            this->s_in[4].lpos = b2Vec2((-this->get_width()/2.f)+.025f+.6f, this->height-.275f);

            this->s_out[0].lpos = b2Vec2((-this->get_width()/2.f)+.025f-.6f, this->height-.575f);
            this->s_out[1].lpos = b2Vec2((-this->get_width()/2.f)+.025f-.3f, this->height-.575f);
            this->s_out[2].lpos = b2Vec2((-this->get_width()/2.f)+.025f+.0f, this->height-.575f);
            this->s_out[3].lpos = b2Vec2((-this->get_width()/2.f)+.025f+.3f, this->height-.575f);
            this->s_out[4].lpos = b2Vec2((-this->get_width()/2.f)+.025f+.6f, this->height-.575f);
            break;
    }

    this->set_material(&m_factory);
    //this->set_uniform("~color", .2f, .2f, .2f, 1.f);

    this->menu_scale = 0.25f;
    this->down_step = 0;
    this->num_absorbed = 0;
    this->last_absorbed = false;
    this->item_completed = false;
    this->error = false;
    this->very_hungry = false;
    this->bottom = 0;
    this->conveyor_speed = 0.f;
    this->conveyor_invert = false;

    this->scaleselect = true;
}

/** Inputs
 * 0 = Add to queue
 * 1 = What to build
 * 2 = Absorb items without interaction
 * 3 = Conveyor ON/OFF
 * 4 = Conveyor invert direction
 **/

/** Outputs
 * 0 = Something was emitted
 * 1 = Something was absorbed
 * 2 = Error (not enough material)
 * 3 = Current build progress
 * 4 = Binary, if something is building
 **/

int
factory::handle_event(uint32_t type, uint64_t pointer_id, tvec2 pos)
{
    switch (type) {
        case TMS_EV_POINTER_DOWN:
            this->down_pid = pointer_id;
            this->down_step = W->step_count;
            break;

        case TMS_EV_POINTER_UP:
            {
                if (pointer_id == this->down_pid) {
                    uint32_t step_diff = W->step_count - this->down_step;
                    if (step_diff < CLICK_MAX_STEPS) {
                        tms_debugf("clicked factory");
                        G->sel_p_ent = this;
                        G->set_mode(GAME_MODE_FACTORY);
                        return EVENT_DONE;
                    }
                }
            }
            break;
    }

    return EVENT_CONT;
}

void
factory::generate_recipes(std::vector<uint32_t> *vec, const char *real_buf)
{
    std::vector<char*> strings = p_split(real_buf, strlen(real_buf), ";");

    for (std::vector<char*>::iterator it = strings.begin();
            it != strings.end(); ++it) {
        vec->push_back(atoi(*it));
    }
}

void
factory::init()
{
    num_filtered = 0;

    this->autorecipe_list.clear();

    factory::generate_recipes(&this->autorecipe_list, this->properties[0].v.s.buf);
}

void
factory::setup()
{
    this->build_accum = 0;
    memset(this->queue, 0, sizeof(this->queue));
    this->queue_size = 0;
    this->num_absorbed = 0;
    this->last_absorbed = false;
    this->item_completed = false;
    this->error = false;
    this->very_hungry = false;
    this->conveyor_speed = 0.f;
    this->conveyor_invert = false;

    //this->set_flag(ENTITY_IS_STATIC, true);
}

void factory::restore()
{
    edev_simpleconnect::restore();
    //this->set_flag(ENTITY_IS_STATIC, true);
}

void
factory::write_state(lvlinfo *lvl, lvlbuf *lb)
{
    edev_simpleconnect::write_state(lvl, lb);

    lb->w_s_uint64(this->build_accum);
    lb->w_s_uint8((uint8_t)this->last_absorbed);
    lb->w_s_uint8((uint8_t)this->item_completed);
    lb->w_s_uint8((uint8_t)this->error);

    lb->w_s_uint8((uint8_t)this->very_hungry);
    lb->w_s_float(this->conveyor_speed);
    lb->w_s_uint8((uint8_t)this->conveyor_invert);
    lb->w_s_uint32(this->num_absorbed);

    for (int x=0; x<FACTORY_QUEUE_SIZE; x++) {
        lb->w_s_int32(this->queue[x].item);
        lb->w_s_int32(this->queue[x].count);
        lb->w_s_uint64(this->queue[x].completed);
        lb->w_s_float(this->queue[x].build_speed);
    }

    lb->w_s_int32(this->queue_size);
}

void
factory::read_state(lvlinfo *lvl, lvlbuf *lb)
{
    edev_simpleconnect::read_state(lvl, lb);

    this->build_accum = lb->r_uint64();
    this->last_absorbed = (bool)lb->r_uint8();
    this->item_completed = (bool)lb->r_uint8();
    this->error = (bool)lb->r_uint8();

    this->very_hungry = (bool)lb->r_uint8();
    this->conveyor_speed = lb->r_float();
    this->conveyor_invert = (bool)lb->r_uint8();
    this->num_absorbed = lb->r_uint32();

    for (int x=0; x<FACTORY_QUEUE_SIZE; x++) {
        this->queue[x].item = lb->r_int32();
        this->queue[x].count = lb->r_int32();
        this->queue[x].completed = lb->r_uint64();
        this->queue[x].build_speed = lb->r_float();
    }

    this->queue_size = lb->r_int32();
}

void
factory::add_to_world()
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

    switch (this->factory_type) {
        case FACTORY_ROBOT:
            {
                b2Vec2 verts[4] = {
                    b2Vec2( (this->get_width()+.75f), -this->height),
                    b2Vec2(-(this->get_width()+.75f), -this->height),
                    b2Vec2( (this->get_width()),      -this->height+(this->sz_bottom.h*2.f)),
                    b2Vec2(-(this->get_width()),      -this->height+(this->sz_bottom.h*2.f)),
                };

                box.Set(verts, 4);
                (this->bottom = (this->body->CreateFixture(&fd)))->SetUserData(this);
            }
            break;
    }

    /* top */
    box.SetAsBox(this->sz_top.w, this->sz_top.h, b2Vec2(0.f, this->height-(this->sz_top.h/1.f)), 0);
    (this->body->CreateFixture(&fd))->SetUserData(this);

    /* bottom */
    if (!this->bottom) {
        // no custom bottom has been created, make a generic one
        box.SetAsBox(this->sz_bottom.w, this->sz_bottom.h, b2Vec2(0.f, -this->height+this->sz_bottom.h), 0);
        (this->bottom = (this->body->CreateFixture(&fd)))->SetUserData(this);
    }

    /* emit sensor */
    fd.isSensor = true;
    box.SetAsBox(this->get_width(), this->height-this->sz_bottom.h-this->sz_top.h, b2Vec2(0,(this->sz_bottom.h-this->sz_top.h)), 0);
    (this->emit_sensor = this->body->CreateFixture(&fd))->SetUserData(this);

    /* absorb sensor */
    box.SetAsBox(this->get_width()-.125f, .25f, b2Vec2(0.f, this->height+.25f), 0);
    (this->absorb_sensor = this->body->CreateFixture(&fd))->SetUserData(this);

    this->body->SetSleepingAllowed(false);
}

void
factory::on_touch(b2Fixture *my, b2Fixture *other)
{
    entity *e;

    if (my == this->emit_sensor) {
        if (W->is_adventure() && adventure::player && other == adventure::player->get_sensor_fixture()) {
            adventure::current_factory = this;
        }
    } else if (my == this->absorb_sensor) {
        if ((e = static_cast<entity*>(other->GetUserData()))) {
            if (e->g_id == O_ITEM) {
                item *i = (item*)e;

                // TODO: Do we need to lock/unlock game before each absorb?
                // Should this be handled automatically in game::absorb?
                if (i->get_item_type() == ITEM_OIL) {
                    if (G->absorb(e)) {
                        this->add_oil(10.f);

                        if (e->get_body(0)) e->get_body(0)->SetLinearVelocity(b2Vec2(0,0));

                        ++ this->num_absorbed;
                        G->add_highlight(this, false, 1.f);
                    }
                }
#if 0 /* XXX scrap moved to item, needs fix */
            } else if (e->g_id == O_SCRAP) {
                scrap *s = static_cast<scrap*>(e);
                uint64_t items[NUM_RESOURCES];
                memset(items, 0, sizeof(items));

                if (G->absorb(e)) {
                    switch (s->get_scrap_type()) {
                        case SCRAP_ROBOT_HEAD:
                            items[RESOURCE_ALUMINIUM] = 5;
                            items[RESOURCE_COPPER] = 3;
                            break;

                        case SCRAP_ROBOT_BODY:
                            items[RESOURCE_ALUMINIUM] = 20;
                            items[RESOURCE_COPPER] = 1;
                            break;
                    }

                    for (int x=0; x<NUM_RESOURCES; x++) {
                        this->items[x] += items[x];
                    }

                    if (e->get_body(0)) e->get_body(0)->SetLinearVelocity(b2Vec2(0,0));

                    ++ this->num_absorbed;
                    G->add_highlight(this, false, 1.f);
                }
#endif

            } else {
                if ((G->interacting_with(e) || this->very_hungry) && !e->conn_ll && (e->g_id != 0 || e->flag_active(ENTITY_IS_BEAM))) {
                    struct factory_object *fo = 0;
                    std::vector<struct factory_object> &objs = this->objects();

                    for (std::vector<struct factory_object>::iterator it = objs.begin();
                            it != objs.end(); ++it) {
                        struct factory_object &_fo = *it;

                        if (_fo.gid == e->g_id && (e->num_properties == 0 || _fo.set_prop_0 == -1 || e->properties[0].v.i == _fo.set_prop_0)) {
                            fo = &_fo;
                            break;
                        }
                    }

                    if (fo) {
                        tms_debugf("Factory %p attempting to absorb factory-compatible entity %p", this, e);
                        if (G->absorb(e)) {
                            tms_debugf("Factory %p successfully absorbed entity %p, oil: %f", this, e, fo->worth.oil);

                            this->add_oil(fo->worth.oil);

                            for (int x=0; x<NUM_RESOURCES; x++) {
                                if (fo->worth.resources[x]) {
                                    tms_debugf("Factory %p gained %u %s", this, fo->worth.resources[x], resource_data[x].name);
                                }

                                this->add_resources(x, fo->worth.resources[x]);
                            }

                            if (e->get_body(0)) e->get_body(0)->SetLinearVelocity(b2Vec2(0,0));

                            ++ this->num_absorbed;
                            G->add_highlight(this, false, 1.f);
                        } else {
                            tms_debugf("Factory %p failed absorbing entity %p, because it has already been absorbed, or is part of a grouperino", this, e);
                        }
                    } else {
                        tms_debugf("%p is not a factory-compatible entity", e);
                    }
                }
            }
        }
    }
}

void
factory::on_untouch(b2Fixture *my, b2Fixture *other)
{
    if (my == this->emit_sensor) {
        if (W->is_adventure() && adventure::player && other == adventure::player->get_sensor_fixture()) {
            adventure::current_factory = 0;
        }
    } else if (my == this->absorb_sensor) {

    }
}

static int padding_x;
static int fwidth;
static int fheight;
static int padding_y;
static int btn_outer_x;
static int btn_inner_x;
static int btn_outer_y;
static int btn_inner_y;
static int menu_top[NUM_FACTORIES];

static bool down[MAX_P];
static bool inside[MAX_P];
static bool dragging[MAX_P];
static tvec2 start[MAX_P];
static tvec2 pos[MAX_P];

static int top_height;

static int slot_x;
static int slot_y;

void
game::render_factory(void)
{
    if (!this->sel_p_ent || !IS_FACTORY(this->sel_p_ent->g_id)) {
        this->set_mode(GAME_MODE_DEFAULT);
        return;
    }

    factory *fa = (factory*)(this->sel_p_ent);
    std::vector<struct factory_object> &objs = fa->objects();

    if (menu_top[fa->factory_type] < 0) menu_top[fa->factory_type] = 0;
    else if (menu_top[fa->factory_type] > btn_outer_y*(num_filtered-1)) menu_top[fa->factory_type] = btn_outer_y*(num_filtered-1);

    num_filtered = 0;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    glActiveTexture(GL_TEXTURE0);
    tms_ddraw_set_color(this->get_surface()->ddraw, .0f, 0.f, .0f, .75f);
    tms_ddraw_square(this->get_surface()->ddraw, _tms.window_width/2.f, _tms.window_height/2.f,
            _tms.window_width, _tms.window_height);

    padding_x = _tms.xppcm*.125f;
    fwidth = _tms.xppcm*4.f;
    fheight = _tms.window_height-_tms.yppcm;
    //fheight = _tms.yppcm*4.f;
    padding_y = _tms.yppcm*.125f;
    btn_outer_x = _tms.xppcm*1.125f*.65f;
    btn_inner_x = _tms.xppcm*.65f;
    btn_outer_y = _tms.yppcm*1.125f*.65f;
    btn_inner_y = _tms.yppcm*.65f;

    top_height = _tms.yppcm;

    tms_ddraw_set_color(this->get_surface()->ddraw, .2f, 0.2f, .2f, 1.f);
    tms_ddraw_square(this->get_surface()->ddraw, _tms.window_width/2.f, _tms.window_height/2.f, fwidth+padding_x*2.f, fheight+padding_y*2.f);

    tms_ddraw_set_color(this->get_surface()->ddraw, .1f, 0.1f, .1f, 1.f);
    tms_ddraw_square(this->get_surface()->ddraw, _tms.window_width/2.f, _tms.window_height/2.f-top_height/2, fwidth, fheight-top_height);

    slot_x = btn_outer_x * .8f;
    slot_y = btn_outer_y * .8f;

    struct tms_sprite *label = 0;
    if (fa->factory_type == FACTORY_GENERIC) {
        label = filterlabel;
    } else if (fa->factory_type == FACTORY_ROBOT) {
        label = factionlabel;
    }

    if (label) {
        /* render list of categories */
        tms_ddraw_set_color(this->get_surface()->ddraw, 0.7f, 0.7f, 0.7f, 1.f);
        glBindTexture(GL_TEXTURE_2D, this->texts->texture.gl_texture);
        tms_ddraw_sprite(this->get_surface()->ddraw, label,
                _tms.window_width/2.f - fwidth/2.f + label->width/2.f,
                _tms.window_height/2.f+fheight/2.f-top_height +slot_y*.25f,
                label->width*.7f,
                label->height*.7f
                );

        if (fa->factory_type == FACTORY_GENERIC) {
            float xp = 0.f;
            for (int x=0; x<num_list_cats; x++) {
                if (cat_hide[fa->factory_type][list_cats[x]])
                    tms_ddraw_set_color(this->get_surface()->ddraw, 0.3f, 0.3f, 0.3f, 1.f);
                else
                    tms_ddraw_set_color(this->get_surface()->ddraw, 1.f, 1.f, 1.f, 1.f);
                tms_ddraw_sprite(this->get_surface()->ddraw, catsprite_hints[list_cats[x]],
                        _tms.window_width/2.f - fwidth/2.f + xp + catsprite_hints[list_cats[x]]->width*.7f/2.f + label->width + _tms.xppcm*.125f,
                        _tms.window_height/2.f+fheight/2.f-top_height +slot_y*.25f,
                        catsprite_hints[list_cats[x]]->width*.7f,
                        catsprite_hints[list_cats[x]]->height*.7f
                        );

                xp += _tms.xppcm * .125f + catsprite_hints[list_cats[x]]->width*.7f;
            }
        } else if (fa->factory_type == FACTORY_ROBOT) {
            float xp = 0.f;
            float w = 17.5f;
            for (uint32_t x=0; x<NUM_FACTIONS; ++x) {
                if (fa->properties[2].v.i == x)
                    tms_ddraw_set_color(this->get_surface()->ddraw, factions[x].color.r, factions[x].color.g, factions[x].color.b, 1.f);
                else
                    tms_ddraw_set_color(this->get_surface()->ddraw, factions[x].color.r*.2f, factions[x].color.g*.2f, factions[x].color.b*.2f, 1.f);

                tms_ddraw_square(this->get_surface()->ddraw,
                        _tms.window_width/2.f - fwidth/2.f + xp + w/2.f + label->width + _tms.xppcm*.125f,
                        _tms.window_height/2.f+fheight/2.f-top_height +slot_y*.25f,
                        w,
                        w
                        );

                xp += w*2.f;
            }
        }
    }

    GLuint last_bind = -1;
    for (int x=0; x<FACTORY_QUEUE_SIZE; x++) {
        float comp = 0.f;
        if (x<fa->queue_size) {
            comp = (double)fa->queue[x].completed / 1000000.;
        }
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        tms_ddraw_set_color(this->get_surface()->ddraw, .1f, 0.1f+comp*.4f, .1f, 1.f);
        tms_ddraw_square(this->get_surface()->ddraw,
                _tms.window_width/2.f - 3.f * slot_x + x*slot_x + slot_x/2.f,
                _tms.window_height/2.f+fheight/2.f-top_height +slot_y,
                slot_x*.8f, slot_y*.8f);

        if (x < fa->queue_size) {
            glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_COLOR);
            struct tms_sprite *img;
            int cat = 0;
            GLuint gltex;
            struct factory_object &o = objs.at(fa->queue[x].item);
            if (fa->factory_type == FACTORY_ARMORY || fa->factory_type == FACTORY_OIL_MIXER) {
                img = &item_options[o.gid].image;
                gltex = this->get_item_texture()->gl_texture;
            } else {
                struct menu_obj mo = menu_objects[gid_to_menu_pos[o.gid]];
                img = &mo.image;
                cat = mo.category;
                gltex = this->get_sandbox_texture(cat)->gl_texture;
            }

            if (gltex != last_bind) {
                last_bind = gltex;
                glBindTexture(GL_TEXTURE_2D, gltex);
            }

            tms_ddraw_set_color(this->get_surface()->ddraw, 1.f, 1.f, 1.f, 1.f);
            tms_ddraw_sprite(this->get_surface()->ddraw, img,
                    _tms.window_width/2.f - 3.f * slot_x + x*slot_x + slot_x/2.f,
                    _tms.window_height/2.f+fheight/2.f-top_height +slot_y,
                    slot_x*.8f * o.scale_x,
                    slot_y*.8f * o.scale_y
                    );
        }
    }
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    int x1 = _tms.window_width/2.f - fwidth/2.f + padding_x;
    int y1 = _tms.window_height/2.f + fheight/2.f - padding_y - top_height;
    int a_x = 0.f;
    int a_y = 0.f;
    int base_a_y = 0.f;
    int iw = _tms.xppcm*.25f;
    int ih = _tms.yppcm*.25f;

    glEnable(GL_SCISSOR_TEST);
    glScissor(
            _tms.window_width/2.f - fwidth/2.f,
            _tms.window_height/2.f-fheight/2.f,
            fwidth,
            fheight - top_height
            );

    for (int filter=0; filter<2; filter++) {
        float alpha = filter?.25f:1.f;
        int ry = 0;

        tms_ddraw_set_color(this->get_surface()->ddraw, 1.f, 1.f, 1.f, alpha);

        /* render object icons */
        glBlendFunc(GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR);
        last_bind = -1;
        for (std::vector<struct factory_object>::const_iterator it = objs.begin();
                it != objs.end(); ++it) {
            const struct factory_object &fo = *it;

            struct tms_sprite *img;
            int cat = 0;
            GLuint gltex;
            if (fa->factory_type == FACTORY_ARMORY || fa->factory_type == FACTORY_OIL_MIXER) {
                img = &item_options[fo.gid].image;
                gltex = this->get_item_texture()->gl_texture;
            } else {
                struct menu_obj mo = menu_objects[gid_to_menu_pos[fo.gid]];
                img = &mo.image;
                cat = mo.category;
                gltex = this->get_sandbox_texture(cat)->gl_texture;
            }

            if (gltex != last_bind) {
                last_bind = gltex;
                glBindTexture(GL_TEXTURE_2D, gltex);
            }
            if ((int)fa->can_afford(fo) == filter || (cat_hide[fa->factory_type][cat])) {
                continue;
            }

            ry++;

            filtered[num_filtered++] = it - objs.begin();

            if (ry%2 == 0) {
                tms_ddraw_set_color(this->get_surface()->ddraw, .185f*alpha, 0.185f*alpha, .185f*alpha, 1.f);
                tms_ddraw_square(this->get_surface()->ddraw, _tms.window_width/2.f,
                    y1 - a_y - btn_outer_y/2.f + menu_top[fa->factory_type],
                    fwidth, btn_outer_y);
                tms_ddraw_set_color(this->get_surface()->ddraw, 1.f*alpha, 1.f*alpha, 1.f*alpha, 1.f);
            }

            tms_ddraw_sprite(this->get_surface()->ddraw, img,
                    x1 + btn_outer_x/2.f,
                    y1 - a_y - btn_outer_y/2.f + menu_top[fa->factory_type],
                    btn_inner_x * fo.scale_x, btn_inner_y * fo.scale_y);

            a_y += btn_outer_y;
        }
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        tms_ddraw_set_color(this->get_surface()->ddraw, 1.f, 1.f, 1.f, alpha);

        a_y = base_a_y;
        glBindTexture(GL_TEXTURE_2D, this->texts->texture.gl_texture);
        ry = 0;

        for (std::vector<struct factory_object>::const_iterator it = objs.begin();
                it != objs.end(); ++it) {
            const struct factory_object &fo = *it;
            struct tms_sprite *img = 0;
            int cat = 0;

            if (fa->factory_type == FACTORY_ARMORY || fa->factory_type == FACTORY_OIL_MIXER) {
                const struct item_option &io = item_options[fo.gid];

                cat = 0;

                img = io.name_spr;

            } else {
                const struct menu_obj &mo = menu_objects[gid_to_menu_pos[fo.gid]];

                cat = mo.category;

                img = mo.name;
            }

            if ((int)fa->can_afford(fo) == filter || (cat_hide[fa->factory_type][cat])) {
                continue;
            }

            ry++;

            float w = img->width;
            float h = img->height;

            tms_ddraw_sprite(this->get_surface()->ddraw, img,
                    x1 + btn_outer_x + padding_x*.5f + w/2.f,
                    y1 - a_y - btn_outer_y/2.f + h/2.f + menu_top[fa->factory_type],
                    w, h);

            a_y += btn_outer_y;
        }

        /* render object names */

        /* render object costs  */
        a_y = base_a_y;
        ry = 0;
        glBindTexture(GL_TEXTURE_2D, gui_spritesheet::atlas->texture.gl_texture);

        for (std::vector<struct factory_object>::const_iterator it = objs.begin();
                it != objs.end(); ++it) {
            const struct factory_object &fo = *it;
            int cat = 0;

            if (!(fa->factory_type == FACTORY_ARMORY || fa->factory_type == FACTORY_OIL_MIXER)) {
                struct menu_obj mo = menu_objects[gid_to_menu_pos[fo.gid]];
                cat = mo.category;
            }

            if ((int)fa->can_afford(fo) == filter || (cat_hide[fa->factory_type][cat])) {
                continue;
            }

            ry++;

            int yy = 0;
            if (fo.worth.oil > 0.f) {
                tms_ddraw_sprite(this->get_surface()->ddraw,
                        gui_spritesheet::get_sprite(S_OIL),
                        x1 + btn_outer_x + padding_x*.5f + iw/2.f*1.25f + yy*iw*1.25f,
                        y1 - a_y - btn_outer_y/2.f - ih/2.f + menu_top[fa->factory_type],
                        iw, ih);

                float x = x1 + btn_outer_x + padding_x*.5f + (yy+1)*iw*1.25f;
                float y = y1 - a_y - btn_outer_y/2.f - ih/2.f + menu_top[fa->factory_type];

                G->add_text(fo.worth.oil, font::small, x, y, tvec4f(1.f, 1.f, 1.f, alpha), 0, false);

                yy+=2;
            }

            for (int i=0; i<NUM_RESOURCES; ++i) {
                if (fo.worth.resources[i] > 0) {
                    tms_ddraw_sprite(this->get_surface()->ddraw,
                            gui_spritesheet::get_sprite(S_INVENTORY_ICONS0+i),
                            x1 + btn_outer_x + padding_x*.5f + iw/2.f*1.25f + yy*iw*1.25f,
                            y1 - a_y - btn_outer_y/2.f - ih/2.f + menu_top[fa->factory_type],
                            iw, ih);

                    float x = x1 + btn_outer_x + padding_x*.5f + (yy+1)*iw*1.25f;
                    float y = y1 - a_y - btn_outer_y/2.f - ih/2.f + menu_top[fa->factory_type];

                    G->add_text(fo.worth.resources[i], font::small, x, y, tvec4f(1.f, 1.f, 1.f, alpha), 0, false);

                    yy+=2;
                }
            }

            a_y += btn_outer_y;
        }
        base_a_y = a_y;
    }
    glDisable(GL_SCISSOR_TEST);
    glScissor(0,0,_tms.window_width, _tms.window_height);
    tms_ddraw_set_color(this->get_surface()->ddraw, 1.f,1.f,1.f,1.f);
    /* render the current amounts of everythign */

    char num[64];
    sprintf(num, "%.1f", fa->get_oil());
    int yy = 0;
    int o = 0;

    x1 -= _tms.xppcm;

    o += iw*1.25f/2.f;
    tms_ddraw_sprite(this->get_surface()->ddraw,
            gui_spritesheet::get_sprite(S_OIL),
            x1 + o,
            y1 + top_height - ih/2.f,
            iw, ih);
    o += iw*1.25f/2.f;

    float x = x1 + o;
    float y = y1 + top_height;

    this->add_text(fa->get_oil(), font::small, x, y, TV_WHITE, 1, false, ALIGN_LEFT, ALIGN_TOP);

    for (int i=0; i<NUM_RESOURCES; ++i) {
        y1 -= _tms.yppcm*.3f;

        o = 0;

        o += iw/2.f*1.25f;
        tms_ddraw_sprite(this->get_surface()->ddraw,
                gui_spritesheet::get_sprite(S_INVENTORY_ICONS0+i),
                x1 + o,
                y1 + top_height - ih/2.f,
                iw, ih);
        o += iw/2.f*1.25f;

        float x = x1 + o;
        float y = y1 + top_height;

        uint64_t player_resources = adventure::player ? adventure::player->get_num_resources(i) : 0;

        if (player_resources > 0) {
            sprintf(num, "%u%" PRIu64, fa->get_num_resources(i), player_resources);
        } else {
            sprintf(num, "%u", fa->get_num_resources(i));
        }

        this->add_text(num, font::small, x, y, TV_WHITE, false, ALIGN_LEFT, ALIGN_TOP);
    }

    for (int i=0; i<fa->queue_size; ++i) {
        float x = _tms.window_width/2.f - 3.f * slot_x + i*slot_x + o;
        float y = _tms.window_height/2.f + fheight/2.f-top_height + slot_y / 2.f;

        this->add_text(fa->queue[i].count, font::small, x, y, TV_WHITE, 0, false, ALIGN_CENTER, ALIGN_BOTTOM);
    }


    glDisable(GL_BLEND);
}

int
game::factory_handle_event(tms::event *ev)
{
    if (!this->sel_p_ent || !IS_FACTORY(this->sel_p_ent->g_id)) {
        this->set_mode(GAME_MODE_DEFAULT);
        return EVENT_DONE;
    }

    factory *fa = static_cast<factory*>(this->sel_p_ent);

    switch (ev->type) {
        case TMS_EV_KEY_PRESS:
            switch (ev->data.key.keycode) {
                case TMS_KEY_ESC:
                case TMS_KEY_B:
                case SDL_SCANCODE_AC_BACK:
                    tms_infof("DEFAULT!");
                    this->set_mode(GAME_MODE_DEFAULT);
                    return EVENT_DONE;

                default:
                    return EVENT_DONE;
            }
            break;

        case TMS_EV_POINTER_SCROLL:
            menu_top[fa->factory_type] -= (float)ev->data.scroll.y * btn_outer_y/2;
            return 1;
            break;

        case TMS_EV_POINTER_DOWN:
            {
                int pid = ev->data.motion.pointer_id;
                tvec2 sp = (tvec2){ev->data.motion.x, ev->data.motion.y};

                if (fabsf(sp.x - _tms.window_width/2.f) < fwidth/2.f &&
                    fabsf(sp.y - _tms.window_height/2.f) < fheight/2.f) {

                    down[pid] = true;
                    pos[pid] = sp;
                    start[pid] = sp;
                    dragging[pid] = false;
                    tms_infof("clicked inside factory");

                    if (sp.y < _tms.window_height/2.f + fheight/2.f - top_height) {
                        inside[pid] = true;
                    } else {
                        inside[pid] = false;
                        tms_infof("clicked top part");

                        struct tms_sprite *label = 0;
                        if (fa->factory_type == FACTORY_GENERIC) {
                            label = filterlabel;
                        } else if (fa->factory_type == FACTORY_ROBOT) {
                            label = factionlabel;
                        } else {
                            // factory type has no label, so we won't listen for inputs in this area
                            return EVENT_DONE;
                        }

                        float xp = 0.f;
                        if (fa->factory_type == FACTORY_GENERIC) {
                            for (int x=0; x<num_list_cats; x++) {
                                float dx = sp.x - (_tms.window_width/2.f - fwidth/2.f + xp + catsprite_hints[list_cats[x]]->width*.7f/2.f + label->width + _tms.xppcm*.125f);
                                float dy = sp.y - (_tms.window_height/2.f+fheight/2.f-top_height +slot_y*.25f);
                                if (fabsf(dx) < catsprite_hints[list_cats[x]]->width*.7f/2.f + _tms.xppcm*.125f/2.f
                                    && fabsf(dy) < catsprite_hints[list_cats[x]]->height*.7f/2.f + _tms.yppcm*.125f/2.f) {
                                    cat_hide[fa->factory_type][list_cats[x]] = !cat_hide[fa->factory_type][list_cats[x]];
                                }

                                xp += _tms.xppcm * .125f + catsprite_hints[list_cats[x]]->width*.7f;
                            }
                        } else if (fa->factory_type == FACTORY_ROBOT) {
                            float w = 17.5f;
                            for (int x=0; x<NUM_FACTIONS; ++x) {
                                float dx = sp.x - (_tms.window_width/2.f - fwidth/2.f + xp + w/2.f + label->width + _tms.xppcm*.125f);
                                float dy = sp.y - (_tms.window_height/2.f+fheight/2.f-top_height +slot_y*.25f);
                                if (fabsf(dx) < w/2.f + _tms.xppcm*.125f/2.f
                                    && fabsf(dy) < catsprite_hints[list_cats[x]]->height*.7f/2.f + _tms.yppcm*.125f/2.f) {
                                    fa->properties[2].v.i = x;
                                }

                                xp += w*2.f;
                            }
                        }
                    }
                } else {
                    this->set_mode(GAME_MODE_DEFAULT);
                }
                return EVENT_DONE;
            }
            break;

        case TMS_EV_POINTER_DRAG:
            {
                int pid = ev->data.motion.pointer_id;
                tvec2 sp = (tvec2){ev->data.motion.x, ev->data.motion.y};

                if (down[pid]) {
                    float ydiff = sp.y - start[pid].y;
                    if (fabsf(ydiff) > 15.f) {
                        dragging[pid] = true;

                        menu_top[fa->factory_type] += sp.y - pos[pid].y;
                    }

                    pos[pid] = sp;
                }
            }
            break;

        case TMS_EV_POINTER_UP:
            {
                int pid = ev->data.motion.pointer_id;
                tvec2 sp = (tvec2){ev->data.motion.x, ev->data.motion.y};

                if (down[pid]) {
                    if (!dragging[pid]) {
                        if (sp.y < _tms.window_height/2.f + fheight/2.f - top_height) {
                            int selection = floorf(((_tms.window_height/2.f + fheight/2.f - top_height + menu_top[fa->factory_type])-sp.y)/btn_outer_y);

                            if (selection >= 0 && selection < num_filtered
                                && filtered[selection] < fa->objects().size() && filtered[selection] >= 0) {
                                switch (fa->add_to_queue(filtered[selection])) {
                                    case 0:
                                        /* success */
                                        break;

                                    case 1:
                                        ui::message("You can not afford this object.");
                                        break;

                                    case 2:
                                        ui::message("Queue is full!");
                                        break;
                                }
                            }
                        }
                    }
                }

                down[pid] = false;
            }
            break;
    }

    /* always catch shit */
    return EVENT_DONE;
}

void
factory::step()
{
    if (this->queue_size > 0) {
        if (this->queue[0].count > 0) {
            switch (this->factory_type) {
                case FACTORY_OIL_MIXER:
                    G->play_sound(SND_BUBBLES, this->get_position().x, this->get_position().y, 0, 0.35f, true, this);
                    break;
            }

            uint64_t inc = G->timemul(WORLD_STEP);
            this->queue[0].completed += (double)(inc * FACTORY_SPEED) / this->queue[0].build_speed;
            build_accum += inc;

            if (build_accum > 100000) {
                /* We have no chimney, so don't emit a smoke effect.
                 * We probably need some other indicator that something is being produced. */
                /*
                entity *e;
                if (this->factory_type == FACTORY_GENERIC)
                    e = new smoke_effect(this->local_to_world(b2Vec2(-1.25f, 3.25f), 0), this->get_layer());
                else if (this->factory_type == FACTORY_ROBOT)
                    e = new smoke_effect(this->local_to_world(b2Vec2(-0.5f, 2.75f), 0), this->get_layer());
                G->emit(e, 0, b2Vec2(0.f,0.f));
                */
                build_accum = 0;
            }

            if (this->queue[0].completed >= 1000000) {
                std::vector<struct factory_object> &objs = this->objects();

                this->queue[0].completed = 0;
                this->queue[0].count --;

                entity *e;
                struct factory_object &o = objs.at(this->queue[0].item);
                if (this->factory_type == FACTORY_ARMORY || this->factory_type == FACTORY_OIL_MIXER) {
                    e = of::create(O_ITEM);
                    e->properties[0].v.i = o.gid;
                } else {
                    e = of::create(o.gid);
                    if (o.set_prop_0 != -1) {
                        e->properties[0].v.i8 = o.set_prop_0;
                    }
                }
                //e->_pos = this->get_position();
                e->_pos = this->local_to_world(b2Vec2(0, (this->sz_bottom.h-this->sz_top.h)), 0);
                e->_angle = this->get_angle();
                e->set_layer(this->get_layer());

                if (this->factory_type == FACTORY_ROBOT && e->flag_active(ENTITY_IS_ROBOT)) {
                    ((robot_base*)e)->set_faction(this->properties[2].v.i);
                    e->properties[ROBOT_PROPERTY_ROAMING].v.i8 = true;
                }

                G->emit(e);
                this->item_completed = true;
                G->play_sound(SND_DING, this->get_position().x, this->get_position().y, 0, 1.f);
                tms_infof("completed item %d!", this->queue[0].item);
                this->cleanup_completed();
            }
        }
    } else {
        switch (this->factory_type) {
            case FACTORY_OIL_MIXER:
                sm::stop(&sm::bubbles, this);
                break;
        }
    }
}

void
factory::cleanup_completed()
{
    bool done = false;

    while (!done && queue_size>0) {
        done = true;
        for (int x=0; x<queue_size; x++) {
            if (queue[x].count == 0) {
                if (x != queue_size-1) {
                    memmove(&queue[x], &queue[x+1], sizeof(struct factory_queue_item)*(queue_size-x));
                }
                done = false;
                queue_size --;
                break;
            }
        }
    }
}

bool
factory::can_afford(const struct factory_object& fo) const
{
    if (fo.worth.oil > this->get_oil()) {
        return false;
    }

    for (int x=0; x<NUM_RESOURCES; x++) {
        if (fo.worth.resources[x] > this->get_num_resources(x) + (adventure::player ? adventure::player->get_num_resources(x) : 0)) {
            return false;
        }
    }

    return true;
}

int
factory::add_to_queue(int sel)
{
    std::vector<struct factory_object> &objs = this->objects();
    const struct factory_object &fo = objs.at(sel);

    int x = this->queue_size;

    if (!this->can_afford(fo)) {
        this->error = true;
        return 1;
    }

    if (this->queue_size>0 && this->queue[this->queue_size-1].item == sel)
        x = this->queue_size - 1;
    else if (this->queue_size < FACTORY_QUEUE_SIZE) {
        this->queue[x].count = 0;
        this->queue[x].completed = 0;
        this->queue_size ++;
    }

    if (x >= FACTORY_QUEUE_SIZE || this->queue_size > FACTORY_QUEUE_SIZE) {
        this->error = true;
        return 2;
    }

    /* cost checking passed, now decrement shit */
    this->add_oil(-fo.worth.oil);

    for (int x=0; x<NUM_RESOURCES; x++) {
        uint64_t cost = fo.worth.resources[x];
        if (this->get_num_resources(x) < cost) {
            cost -= this->get_num_resources(x);
            this->set_num_resources(x, 0);
        } else {
            this->add_resources(x, -cost);
            cost = 0;
        }

        adventure::player->add_resource(x, -cost);
    }

    this->queue[x].item = sel;
    this->queue[x].count ++;
    this->queue[x].build_speed = fo.worth.oil;
    for (int y=0; y<NUM_RESOURCES; y++) {
        this->queue[x].build_speed += fo.worth.resources[y];
    }

    this->queue[x].build_speed = 1.f + this->queue[x].build_speed*.125f;

    return 0;
}

float
factory::get_tangent_speed()
{
    return this->conveyor_invert ? -this->conveyor_speed : this->conveyor_speed;
}

edevice*
factory::solve_electronics()
{
    if (!this->s_out[0].written()) {
        this->s_out[0].write(this->item_completed ? 1.f : 0.f);
        this->item_completed = false;
    }

    if (!this->s_out[1].written()) {
        if (this->num_absorbed && !this->last_absorbed) {
            this->last_absorbed = true;
            this->s_out[1].write(1.f);
            -- this->num_absorbed;
        } else {
            this->s_out[1].write(0.f);
            this->last_absorbed = false;
        }
    }

    if (!this->s_out[2].written()) {
        this->s_out[2].write(this->error ? 1.f : 0.f);

        this->error = false;
    }

    if (!this->s_out[3].written()) {
        if (this->queue_size) {
            this->s_out[3].write(tclampf((float)this->queue[0].completed/1000000, 0.f, 1.f));
        } else {
            this->s_out[3].write(0.f);
        }
    }

    if (!this->s_out[4].written()) {
        this->s_out[4].write(this->queue_size > 0 ? 1.f : 0.f);
    }

    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();
    if (!this->s_in[1].is_ready())
        return this->s_in[1].get_connected_edevice();
    if (!this->s_in[2].is_ready())
        return this->s_in[2].get_connected_edevice();
    if (!this->s_in[3].is_ready())
        return this->s_in[3].get_connected_edevice();
    if (!this->s_in[4].is_ready())
        return this->s_in[4].get_connected_edevice();

    float build_fraction;

    if (this->s_in[1].p) {
        build_fraction = this->s_in[1].get_value();
    } else {
        build_fraction = 0.f;
    }

    if ((bool)((int)roundf(this->s_in[0].get_value())) && this->autorecipe_list.size() > 0) {
        int index = floorf(std::abs((this->autorecipe_list.size()) * build_fraction-0.00001f));
        this->add_to_queue(this->autorecipe_list.at(index));
    }

    this->very_hungry = (bool)((int)roundf(this->s_in[2].get_value()));
    this->conveyor_speed = this->s_in[3].get_value() * 2.5f;
    this->conveyor_invert = (bool)((int)roundf(this->s_in[4].get_value()));

    return 0;
}

std::vector<struct factory_object>&
factory::objects()
{
    switch (this->factory_type) {
        case FACTORY_ROBOT:
            return robot_objects;

        case FACTORY_ARMORY:
            return armory_objects;

        case FACTORY_OIL_MIXER:
            return oil_mixer_objects;

        case FACTORY_GENERIC:
        default:
            return generic_objects;
    }
}

void
factory::init_recipes()
{
    /* Factory */
    {
        generic_objects.push_back(
                factory_object().g_id(O_PLANK)
                                .prop0(0)
                                .add(RESOURCE_WOOD, 1)
                );
        generic_objects.push_back(
                factory_object().g_id(O_PLANK)
                                .prop0(1)
                                .add(RESOURCE_WOOD, 2)
                );
        generic_objects.push_back(
                factory_object().g_id(O_PLANK)
                                .prop0(2)
                                .add(RESOURCE_WOOD, 3)
                );
        generic_objects.push_back(
                factory_object().g_id(O_PLANK)
                                .prop0(3)
                                .add(RESOURCE_WOOD, 4)
                );
            generic_objects.push_back(
                    factory_object().g_id(O_BALL)
                                    .add(RESOURCE_WOOD, 1)
                    );
            generic_objects.push_back(
                factory_object().g_id(O_BOX)
                                .prop0(0)
                                .scale(.5f, .5f)
                                .add(RESOURCE_WOOD, 1)
                    );
            generic_objects.push_back(
                factory_object().g_id(O_BOX)
                                .prop0(1)
                                .add(RESOURCE_WOOD, 2)
                    );
            generic_objects.push_back(
                factory_object().g_id(O_CORNER)
                                .add(RESOURCE_WOOD, 1)
                    );
            generic_objects.push_back(
                factory_object().g_id(O_CYLINDER)
                                .prop0(0)
                                .scale(.5f, .5f)
                                .add(RESOURCE_WOOD, 1)
                    );
            generic_objects.push_back(
                factory_object().g_id(O_CYLINDER)
                                .prop0(1)
                                .add(RESOURCE_WOOD, 3)
                    );
            generic_objects.push_back(
                factory_object().g_id(O_CYLINDER)
                                .prop0(2)
                                .scale(1.5f, 1.5f)
                                .add(RESOURCE_WOOD, 5)
                    );
            generic_objects.push_back(
                factory_object().g_id(O_SUBLAYER_PLANK)
                                .add(RESOURCE_WOOD, 2)
                    );
            generic_objects.push_back(
                factory_object().g_id(O_METAL_BALL)
                                .add(RESOURCE_IRON, 1)
                    );
            generic_objects.push_back(
                factory_object().g_id(O_RUBBER_BEAM)
                                .prop0(0)
                                .add(RESOURCE_WOOD, 1)
                                            .add_oil(1.f)
                    );
            generic_objects.push_back(
                factory_object().g_id(O_PLASTIC_BEAM)
                                .prop0(0)
                                .scale(.25f)
                                .add_oil(1.f)
                    );
            generic_objects.push_back(
                factory_object().g_id(O_PLASTIC_BEAM)
                                .prop0(1)
                                .scale(.5f)
                                .add_oil(2.f)
                    );
            generic_objects.push_back(
                factory_object().g_id(O_PLASTIC_BEAM)
                                .prop0(2)
                                .scale(.75f)
                                .add_oil(3.f)
                    );
            generic_objects.push_back(
                factory_object().g_id(O_PLASTIC_BEAM)
                                .prop0(3)
                                .add_oil(4.f)
        /* MECHANICS */

                    );
            generic_objects.push_back(
                factory_object().g_id(O_WHEEL)
                                .add_oil(2.f)
                                            .add(RESOURCE_IRON, 2)
                    );
            generic_objects.push_back(
                factory_object().g_id(O_SEESAW)
                                .add(RESOURCE_IRON, 2)
                    );
            generic_objects.push_back(
                factory_object().g_id(O_THRUSTER)
                                .add(RESOURCE_IRON, 3)
                                            .add(RESOURCE_COPPER, 1)
                                            .add(RESOURCE_TOPAZ, 4)
                    );
            generic_objects.push_back(
                factory_object().g_id(O_ROCKET)
                                .add(RESOURCE_IRON, 6)
                                            .add(RESOURCE_COPPER, 1)
                                            .add(RESOURCE_TOPAZ, 10)
                    );
            generic_objects.push_back(
                factory_object().g_id(O_FAN)
                                .add(RESOURCE_IRON, 2)
                                            .add(RESOURCE_COPPER, 1)
                                            .add(RESOURCE_EMERALD, 1)
                    );
            generic_objects.push_back(
                factory_object().g_id(O_TOGGLE_BUTTON)
                                .add(RESOURCE_IRON, 1)
                                            .add(RESOURCE_COPPER, 1)
                                            .add_oil(1.f)
        /* ELECTRONICS */
                    );
            generic_objects.push_back(
                    factory_object().g_id(O_BATTERY)
                                    .add(RESOURCE_RUBY, 1)
                                                .add(RESOURCE_COPPER, 1)
                                                .add(RESOURCE_TOPAZ, 1)
                                                .add_oil(2.f)
                    );
            generic_objects.push_back(
                factory_object().g_id(O_POWER_SUPPLY)
                                .add(RESOURCE_RUBY, 25)
                                .add(RESOURCE_COPPER, 9)
                                .add(RESOURCE_IRON, 6)
                                .add(RESOURCE_TOPAZ, 15)
                                .add_oil(25.f)
                    );
            generic_objects.push_back(
                factory_object().g_id(O_SIMPLE_MOTOR)
                                .add(RESOURCE_EMERALD, 1)
                   .add(RESOURCE_COPPER, 2)
                   .add_oil(2.f)
                    );
            generic_objects.push_back(
                factory_object().g_id(O_MAGNETIC_PLUG)
                                .add(RESOURCE_TOPAZ, 1)
                   .add(RESOURCE_COPPER, 3)
                   .add(RESOURCE_IRON, 2)
                    );
            generic_objects.push_back(
                factory_object().g_id(O_MAGNETIC_SOCKET)
                                .add(RESOURCE_TOPAZ, 1)
                                .add(RESOURCE_COPPER, 2)
                                .add(RESOURCE_IRON, 2)
        /* ROBOTICS */

                    );
            generic_objects.push_back(
                factory_object().g_id(O_DC_MOTOR)
                                .add(RESOURCE_EMERALD, 1)
                                .add(RESOURCE_COPPER, 1)
                                .add_oil(2.f)
                    );
            generic_objects.push_back(
                factory_object().g_id(O_SERVO_MOTOR)
                                .add(RESOURCE_EMERALD, 1)
                   .add(RESOURCE_COPPER, 1)
                   .add_oil(2.f)
                    );
            generic_objects.push_back(
                factory_object().g_id(O_CT_MINI)
                                .add(RESOURCE_SAPPHIRE, 2)
                   .add(RESOURCE_EMERALD, 2)
                   .add(RESOURCE_COPPER, 4)
                   .add_oil(3.f)
                    );
            generic_objects.push_back(
                factory_object().g_id(O_RC_BASIC)
                                .add(RESOURCE_EMERALD, 4)
                   .add(RESOURCE_DIAMOND, 4)
                   .add(RESOURCE_COPPER, 4)
                   .add_oil(3.f)
                    );
            generic_objects.push_back(
                factory_object().g_id(O_LASER_SENSOR)
                                .add(RESOURCE_SAPPHIRE, 1)
                   .add(RESOURCE_COPPER, 1)
                   .add_oil(1.f)
                    );
            generic_objects.push_back(
                factory_object().g_id(O_PROXIMITY_SENSOR)
                                .add(RESOURCE_SAPPHIRE, 4)
                   .add(RESOURCE_COPPER, 1)
                   .add(RESOURCE_IRON, 2)
                    );
            generic_objects.push_back(
                factory_object().g_id(O_PRESSURE_SENSOR)
                                .add(RESOURCE_SAPPHIRE, 4)
                   .add(RESOURCE_COPPER, 1)
                   .add_oil(4.f)
                    );
            generic_objects.push_back(
                factory_object().g_id(O_IMPACT_SENSOR)
                                .add(RESOURCE_SAPPHIRE, 4)
                   .add(RESOURCE_COPPER, 1)
                   .add_oil(4.f)
                   );

        /* signal-i1o1 */
#define Q_I1O1(gid,oil_cost)\
            generic_objects.push_back(\
                factory_object().g_id(gid)\
                                .add_oil(oil_cost)\
                                .add(RESOURCE_COPPER, 2)\
                )

        Q_I1O1(O_INVERTER,1.f);
        Q_I1O1(O_FLOOR,1.f);
        Q_I1O1(O_CEIL,1.f);
        Q_I1O1(O_SQUARE,1.f);
        Q_I1O1(O_SQRT,1.f);
        Q_I1O1(O_SPARSIFIER,1.f);
        Q_I1O1(O_SPARSIFIERPLUS,1.f);
        Q_I1O1(O_EPSILON,1.f);
        Q_I1O1(O_TOGGLER,2.f);
        Q_I1O1(O_MAVG,2.f);
        Q_I1O1(O_FIFO,2.f);
        Q_I1O1(O_VALUE_SHIFT,2.f);
        Q_I1O1(O_CLAMP,2.f);
        Q_I1O1(O_MULADD,1.f);
        Q_I1O1(O_ESUB,1.f);
        Q_I1O1(O_DECAY,1.f);
        Q_I1O1(O_LINEAR_DECAY,1.f);
        Q_I1O1(O_SNAP,1.f);

        /* signal-i2o1 */
#define Q_I2O1(gid)\
            generic_objects.push_back(\
                factory_object().g_id(gid)\
                                .add_oil(2.f)\
                                            .add(RESOURCE_COPPER, 3)\
                )

        Q_I2O1(O_XORGATE);
        Q_I2O1(O_ORGATE);
        Q_I2O1(O_ANDGATE);
        Q_I2O1(O_NANDGATE);
        Q_I2O1(O_IFGATE);
        Q_I2O1(O_CMPE);
        Q_I2O1(O_CMPL);
        Q_I2O1(O_CMPLE);
        Q_I2O1(O_MIN);
        Q_I2O1(O_MAX);
        Q_I2O1(O_SUM);
        Q_I2O1(O_MUL);
        Q_I2O1(O_AVG);
        Q_I2O1(O_CONDENSER);
        Q_I2O1(O_WRAPCONDENSER);
        Q_I2O1(O_MEMORY);
        Q_I2O1(O_WRAPADD);
        Q_I2O1(O_WRAPSUB);
        Q_I2O1(O_WRAPDIST);

        /* signal-misc */
#define Q_I2O2(gid)\
            generic_objects.push_back(\
                factory_object().g_id(gid)\
                                .add_oil(3.f)\
                                            .add(RESOURCE_COPPER, 4)\
                )

#define Q_S5(gid)\
            generic_objects.push_back(\
                factory_object().g_id(gid)\
                                .add_oil(4.f)\
                                            .add(RESOURCE_COPPER, 5)\
                )
#define Q_SIG(gid,oil,copper)\
            generic_objects.push_back(\
                factory_object().g_id(gid)\
                                .add_oil(oil)\
                                            .add(RESOURCE_COPPER, copper)\
                )
        Q_I2O1(O_YSPLITTER);
        Q_I2O2(O_IFELSE);
        Q_S5(O_SINCOS);
        Q_S5(O_ATAN2);
        Q_SIG(O_SWITCH, 4.f, 8);

        generic_objects.push_back(
            factory_object()
                .g_id(O_PASSIVE_DISPLAY)
                .add(RESOURCE_TOPAZ, 2)
                .add(RESOURCE_COPPER, 2)
                .add_oil(4.f)
            );
        generic_objects.push_back(
            factory_object()
                .g_id(O_ACTIVE_DISPLAY)
                .add(RESOURCE_TOPAZ, 2)
                .add(RESOURCE_COPPER, 2)
                .add_oil(4.f)
                );
        generic_objects.push_back(
                factory_object().g_id(O_DEBUGGER)
                .add(RESOURCE_TOPAZ, 1)
                .add(RESOURCE_COPPER, 2)
                .add_oil(2.f)
                );
        generic_objects.push_back(
            factory_object().g_id(O_GRAPH)
                            .add(RESOURCE_TOPAZ, 5)
                                        .add(RESOURCE_COPPER, 2)
                                        .add_oil(5.f)
                    );
        generic_objects.push_back(
            factory_object().g_id(O_POINTER)
                            .add(RESOURCE_EMERALD, 1)
                                        .add(RESOURCE_TOPAZ, 2)
                                        .add(RESOURCE_COPPER, 1)
                                        .add_oil(5.f)
                );

        Q_SIG(O_SINEWAVE, 1.f, 1);
        Q_SIG(O_SAWTOOTH, 1.f, 1);
        Q_SIG(O_RANDOM, 1.f, 1);
        Q_SIG(O_SEQUENCER, 1.f, 1);

        /* game */
        generic_objects.push_back(
            factory_object().g_id(O_CRANE)
                            .add(RESOURCE_SAPPHIRE, 6)
                            .add(RESOURCE_TOPAZ, 6)
                            .add(RESOURCE_DIAMOND, 6)
                            .add(RESOURCE_COPPER, 2)
                            .add(RESOURCE_IRON, 8)
                );
    }

    /* Robot Factory */
    {
        robot_objects.push_back(
            factory_object().g_id(O_ROBOT)
                            .scale(.5f, .5f)
                            .prop0(0)
                            .add(item_options[ITEM_ROBOT_FRONT].worth)
                            .add(item_options[ITEM_ROBOT_BACK].worth)
                            .add(item_options[ITEM_ARM_CANNON].worth)
                            .add(item_options[ITEM_BIPED].worth)
                            .add(item_options[ITEM_ROBOT_HEAD].worth)
                );

        robot_objects.push_back(
            factory_object().g_id(O_COMPANION)
                            .scale(.5f, .5f)
                            .prop0(0)
                            .add(item_options[ITEM_ROBOT_FRONT].worth)
                            .add(item_options[ITEM_ROBOT_BACK].worth)
                            .add(item_options[ITEM_BIPED].worth)
                );

        robot_objects.push_back(
            factory_object().g_id(O_SPIKEBOT)
                            .scale(.5f, .5f)
                            .prop0(0)
                            .add(item_options[ITEM_ROBOT_FRONT].worth)
                            .add(item_options[ITEM_ROBOT_BACK].worth)
                            .add(item_options[ITEM_BIPED].worth)
                );

        robot_objects.push_back(
            factory_object().g_id(O_LOBBER)
                            .scale(.5f, .5f)
                            .prop0(0)
                            .add(item_options[ITEM_BIPED].worth)
                );

        robot_objects.push_back(
            factory_object().g_id(O_BOMBER)
                            .scale(.5f, .5f)
                            .prop0(0)
                            .add(item_options[ITEM_BIPED].worth)
                );
    }

    /* Armory */
    {
        armory_objects.push_back(factory_object().item(ITEM_SHOTGUN));
        armory_objects.push_back(factory_object().item(ITEM_RAILGUN));
        armory_objects.push_back(factory_object().item(ITEM_ROCKET_LAUNCHER));
        armory_objects.push_back(factory_object().item(ITEM_BUILDER));
        armory_objects.push_back(factory_object().item(ITEM_ZAPPER));
        armory_objects.push_back(factory_object().item(ITEM_BOMBER));
        armory_objects.push_back(factory_object().item(ITEM_TESLAGUN));
        armory_objects.push_back(factory_object().item(ITEM_PLASMAGUN));
    }

    /* Oil Mixer */
    {
        oil_mixer_objects.push_back(factory_object().item(ITEM_SPEED_OIL));
        oil_mixer_objects.push_back(factory_object().item(ITEM_JUMP_OIL));
        oil_mixer_objects.push_back(factory_object().item(ITEM_ARMOUR_OIL));
        oil_mixer_objects.push_back(factory_object().item(ITEM_MINER_UPGRADE));
    }
}
