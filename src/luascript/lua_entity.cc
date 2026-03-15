#include "lua_entity.hh"
#include "entity.hh"
#include "escript.hh"
#include "game.hh"
#include "luascript/lua_world.hh"
#include "robot_base.hh"
#include "robotman.hh"

#if 0
static int create_entity(lua_State *L)
{
    double val = lua_tonumber(L, lua_upvalueindex(1));
    lua_pushnumber(L, ++val);  /* new value */
    lua_pushvalue(L, -1);  /* duplicate it */
    lua_replace(L, lua_upvalueindex(1));  /* update upvalue */
    return 1;
}
#endif

static int l_entity_persist(lua_State *L)
{
    entity *e = *(static_cast<entity**>(luaL_checkudata(L, 1, "EntityMT"))); // We turn that address into an entity pointer

    lua_pushunsigned(L, e->id);

    lua_pushcclosure(L, &l_world_unpersist_entity, 1);

    return 1;
}

/* entity:get_id() */
static int l_entity_get_id(lua_State *L)
{
    entity *e = *(static_cast<entity**>(luaL_checkudata(L, 1, "EntityMT"))); // We turn that address into an entity pointer
    lua_pushunsigned(L, e->id); // We push the value we wish to return to the stack
    return 1; // And specify to lua how many values we have pushed
}

static int l_entity_get_g_id(lua_State *L)
{
    entity *e = *(static_cast<entity**>(luaL_checkudata(L, 1, "EntityMT")));
    lua_pushnumber(L, e->g_id);
    return 1;
}

/* x, y = entity:position() */
static int l_entity_get_position(lua_State *L)
{
    entity *e = *(static_cast<entity**>(luaL_checkudata(L, 1, "EntityMT")));
    b2Vec2 pos = e->get_position();
    lua_pushnumber(L, pos.x);
    lua_pushnumber(L, pos.y);
    return 2; // We return two values with this function
}

/* angle = entity:get_angle() */
static int l_entity_get_angle(lua_State *L)
{
    entity *e = *(static_cast<entity**>(luaL_checkudata(L, 1, "EntityMT")));
    lua_pushnumber(L, e->get_angle());
    return 1;
}

/* entity:set_angle(angle) */
static int l_entity_set_angle(lua_State *L)
{
    ESCRIPT_VERSION_ERROR(L, "entity:set_angle", "1.5.1", LEVEL_VERSION_1_5_1);

    entity *e = *(static_cast<entity**>(luaL_checkudata(L, 1, "EntityMT")));

    float angle = luaL_checknumber(L, 2);

    e->set_angle(angle);

    return 0;
}

/* entity:set_fixed_rotation(bool) */
static int l_entity_set_fixed_rotation(lua_State *L)
{
    ESCRIPT_VERSION_ERROR(L, "entity:set_fixed_rotation", "1.5.1", LEVEL_VERSION_1_5_1);

    entity *e = *(static_cast<entity**>(luaL_checkudata(L, 1, "EntityMT")));

    bool is_fixed = lua_toboolean(L, 2);

    b2Body *b = e->get_body(0);

    if (b) {
        b->SetFixedRotation(is_fixed);
    }

    return 0;
}

/* entity:is_fixed_rotation() */
static int l_entity_is_fixed_rotation(lua_State *L)
{
    ESCRIPT_VERSION_ERROR(L, "entity:is_fixed_rotation", "1.5.1", LEVEL_VERSION_1_5_1);

    entity *e = *(static_cast<entity**>(luaL_checkudata(L, 1, "EntityMT")));

    b2Body *b = e->get_body(0);

    if (b) {
        lua_pushboolean(L, b->IsFixedRotation());
        return 1;
    }

    return 0;
}

/* vel_x, vel_y = entity:get_velocity() */
static int l_entity_get_velocity(lua_State *L)
{
    entity *e = *(static_cast<entity**>(luaL_checkudata(L, 1, "EntityMT")));

    b2Vec2 vel(0,0);
    if (e->get_body(0)) {
        vel = e->get_body(0)->GetLinearVelocityFromLocalPoint(e->local_to_body(b2Vec2(0,0), 0));
    }
    lua_pushnumber(L, vel.x);
    lua_pushnumber(L, vel.y);

    return 2;
}

static int l_entity_get_angular_velocity(lua_State *L)
{
    entity *e = *(static_cast<entity**>(luaL_checkudata(L, 1, "EntityMT")));

    float vel = 0.f;
    if (e->get_body(0)) {
        vel = e->get_body(0)->GetAngularVelocity();
    }
    lua_pushnumber(L, vel);

    return 1;
}

/* width, height = entity:get_bbox() */
static int l_entity_get_bbox(lua_State *L)
{
    entity *e = *(static_cast<entity**>(luaL_checkudata(L, 1, "EntityMT")));

    lua_pushnumber(L, e->get_width());
    lua_pushnumber(L, e->height);

    return 2;
}

/* layer = entity:get_layer() */
static int l_entity_get_layer(lua_State *L)
{
    ESCRIPT_VERSION_ERROR(L, "entity:get_layer", "1.4", LEVEL_VERSION_1_4);
    entity *e = *(static_cast<entity**>(luaL_checkudata(L, 1, "EntityMT")));
    lua_pushinteger(L, e->get_layer()+1);
    return 1;
}

/* wx, wy = entity:local_to_world(lx, ly) */
static int l_entity_local_to_world(lua_State *L)
{
    ESCRIPT_VERSION_ERROR(L, "entity:local_to_world", "1.4", LEVEL_VERSION_1_4);
    entity *e = *(static_cast<entity**>(luaL_checkudata(L, 1, "EntityMT")));

    float x = luaL_checknumber(L, 2);
    float y = luaL_checknumber(L, 3);

    b2Vec2 r = e->local_to_world(b2Vec2(x,y), 0);

    lua_pushnumber(L, r.x);
    lua_pushnumber(L, r.y);
    return 2;
}

/* lx, ly = entity:world_to_local(wx, wy) */
static int l_entity_world_to_local(lua_State *L)
{
    ESCRIPT_VERSION_ERROR(L, "entity:world_to_local", "1.4", LEVEL_VERSION_1_4);
    entity *e = *(static_cast<entity**>(luaL_checkudata(L, 1, "EntityMT")));

    float x = luaL_checknumber(L, 2);
    float y = luaL_checknumber(L, 3);

    b2Vec2 r = e->world_to_local(b2Vec2(x,y), 0);

    lua_pushnumber(L, r.x);
    lua_pushnumber(L, r.y);
    return 2;
}

/* entity:highlight() */
static int l_entity_highlight(lua_State *L)
{
    ESCRIPT_VERSION_ERROR(L, "entity:highlight", "1.4", LEVEL_VERSION_1_4);
    entity *e = *(static_cast<entity**>(luaL_checkudata(L, 1, "EntityMT")));
    G->add_highlight(e, false, 1.f);
    return 0;
}

/* entity:damage(amount) */
static int l_entity_damage(lua_State *L)
{
    ESCRIPT_VERSION_ERROR(L, "entity:damage", "1.5", LEVEL_VERSION_1_5);

    entity *e = *(static_cast<entity**>(luaL_checkudata(L, 1, "EntityMT")));
    float damage = luaL_checknumber(L, 2);

    if (e->g_id != O_TPIXEL && e->flag_active(ENTITY_IS_INTERACTIVE) && W->level.flag_active(LVL_ENABLE_INTERACTIVE_DESTRUCTION)) {
        G->lock();
        G->damage_interactive(e, 0, 0, damage, b2Vec2(0,0), DAMAGE_TYPE_OTHER);
        G->unlock();
    } else if (e->is_creature()) {
        creature *c = static_cast<creature*>(e);
        c->damage(damage, 0, DAMAGE_TYPE_OTHER, DAMAGE_SOURCE_WORLD, 0);
    }

    return 0;
}

/* entity:is_static() */
static int l_entity_is_static(lua_State *L)
{
    ESCRIPT_VERSION_ERROR(L, "entity:is_static", "1.5", LEVEL_VERSION_1_5);

    entity *e = *(static_cast<entity**>(luaL_checkudata(L, 1, "EntityMT")));

    b2Body *b = e->get_body(0);
    if (b && b->GetType() == b2_staticBody) {
        lua_pushboolean(L, true);
    } else {
        lua_pushboolean(L, false);
    }

    return 1;
}

/* result = entity:absorb(follow_connections) */
static int l_entity_absorb(lua_State *L)
{
    ESCRIPT_VERSION_ERROR(L, "entity:absorb", "1.5", LEVEL_VERSION_1_5);

    entity *e = *(static_cast<entity**>(luaL_checkudata(L, 1, "EntityMT")));

    bool follow_connections = false;
    bool success = true;
    if (lua_gettop(L) >= 2) {
        follow_connections = lua_toboolean(L, 2);
    }

    if (follow_connections) {
        std::set<entity*> loop;
        e->gather_connected_entities(&loop, true, true);
        G->absorb(&loop);
    } else {
        success = G->absorb(e);
    }

    lua_pushboolean(L, success);

    return 1;
}

/* entity:apply_torque(torque) */
static int l_entity_apply_torque(lua_State *L)
{
    ESCRIPT_VERSION_ERROR(L, "entity:apply_torque", "1.5", LEVEL_VERSION_1_5);

    entity *e = *(static_cast<entity**>(luaL_checkudata(L, 1, "EntityMT")));
    float torque = luaL_checknumber(L, 2);

    for (uint32_t x = 0; x < e->get_num_bodies(); ++x) {
        b2Body *b = e->get_body(x);

        if (b) {
            b->ApplyTorque(torque);
        }
    }

    return 0;
}

/* entity:apply_force(x, y) */
static int l_entity_apply_force(lua_State *L)
{
    ESCRIPT_VERSION_ERROR(L, "entity:apply_force", "1.5.1", LEVEL_VERSION_1_5_1);

    entity *e = *(static_cast<entity**>(luaL_checkudata(L, 1, "EntityMT")));
    float x = luaL_checknumber(L, 2);
    float y = luaL_checknumber(L, 3);

    b2Vec2 force(x, y);

    for (uint32_t x = 0; x < e->get_num_bodies(); ++x) {
        b2Body *b = e->get_body(x);

        if (b)
            b->ApplyForceToCenter(force);
    }

    return 0;
}

/* entity:set_velocity(x, y) */
static int l_entity_set_velocity(lua_State *L)
{
    ESCRIPT_VERSION_ERROR(L, "entity:set_velocity", "1.5", LEVEL_VERSION_1_5);

    entity *e = *(static_cast<entity**>(luaL_checkudata(L, 1, "EntityMT")));
    float x = luaL_checknumber(L, 2);
    float y = luaL_checknumber(L, 3);

    b2Vec2 vel(x, y);

    std::set<entity*> *loop = new std::set<entity*>();

    e->gather_connected_entities(loop, false, true);

    for (std::set<entity*>::iterator it = loop->begin(); it != loop->end(); ++it) {
        entity *ie = static_cast<entity*>(*it);

        for (uint32_t x = 0; x < ie->get_num_bodies(); ++x) {
            b2Body *b = ie->get_body(x);

            if (b) {
                b->SetLinearVelocity(vel);
            }
        }
    }

    delete loop;

    return 0;
}

/* entity:warp(x, y, layer) */
static int l_entity_warp(lua_State *L)
{
    ESCRIPT_VERSION_ERROR(L, "entity:warp", "1.5", LEVEL_VERSION_1_5);

    entity *e = *(static_cast<entity**>(luaL_checkudata(L, 1, "EntityMT")));
    float x = luaL_checknumber(L, 2);
    float y = luaL_checknumber(L, 3);
    int layer = -1;

    if (lua_gettop(L) > 3) {
        layer = luaL_checkinteger(L, 4);

        if (layer < 1) layer = 1;
        else if (layer > 3) layer = 3;

        layer --;

        if (layer == e->get_layer()) {
            layer = -1;
        }
    }

    if (!e->conn_ll) {
        e->set_position(x, y);

        for (uint32_t x = 0; x < e->get_num_bodies(); ++x) {
            b2Body *b = e->get_body(x);

            if (b) {
                b->SetAwake(true);
            }
        }

        if (layer != -1) {
            e->set_layer(layer);
        }
    }

    return 0;
}

/* entity:show() */
static int l_entity_show(lua_State *L)
{
    ESCRIPT_VERSION_ERROR(L, "entity:show", "1.5", LEVEL_VERSION_1_5);

    entity *e = *(static_cast<entity**>(luaL_checkudata(L, 1, "EntityMT")));

    if (!e->scene) {
        G->add_entity(e);
        e->set_flag(ENTITY_WAS_HIDDEN, false);
    }

    return 0;
}

/* entity:hide() */
static int l_entity_hide(lua_State *L)
{
    ESCRIPT_VERSION_ERROR(L, "entity:hide", "1.5", LEVEL_VERSION_1_5);

    entity *e = *(static_cast<entity**>(luaL_checkudata(L, 1, "EntityMT")));

    if (e->scene) {
        G->remove_entity(e);
        e->set_flag(ENTITY_WAS_HIDDEN, true);
    }

    return 0;
}

/* entity:is_hidden() */
static int l_entity_is_hidden(lua_State *L)
{
    ESCRIPT_VERSION_ERROR(L, "entity:is_hidden", "1.5.1", LEVEL_VERSION_1_5_1);

    entity *e = *(static_cast<entity**>(luaL_checkudata(L, 1, "EntityMT")));

    lua_pushboolean(L, e->flag_active(ENTITY_WAS_HIDDEN));

    return 1;
}

/* entity:get_name() */
static int l_entity_get_name(lua_State *L)
{
    ESCRIPT_VERSION_ERROR(L, "entity:get_name", "1.5", LEVEL_VERSION_1_5);

    entity *e = *(static_cast<entity**>(luaL_checkudata(L, 1, "EntityMT")));

    lua_pushstring(L, e->get_name());

    return 1;
}

/* entity:is_creature() */
static int l_entity_is_creature(lua_State *L)
{
    ESCRIPT_VERSION_ERROR(L, "entity:is_creature", "1.5", LEVEL_VERSION_1_5);

    entity *e = *(static_cast<entity**>(luaL_checkudata(L, 1, "EntityMT")));

    lua_pushboolean(L, e->is_creature());

    return 1;
}

/* entity:is_robot() */
static int l_entity_is_robot(lua_State *L)
{
    ESCRIPT_VERSION_ERROR(L, "entity:is_robot", "1.5", LEVEL_VERSION_1_5);

    entity *e = *(static_cast<entity**>(luaL_checkudata(L, 1, "EntityMT")));

    lua_pushboolean(L, e->is_robot());

    return 1;
}

/* entity:is_player() */
static int l_entity_is_player(lua_State *L)
{
    ESCRIPT_VERSION_ERROR(L, "entity:is_player", "1.5", LEVEL_VERSION_1_5);

    entity *e = *(static_cast<entity**>(luaL_checkudata(L, 1, "EntityMT")));

    lua_pushboolean(L, e->is_creature() && ((creature*)e)->is_player());

    return 1;
}

/* entity:get_mass() */
static int l_entity_get_mass(lua_State *L)
{
    ESCRIPT_VERSION_ERROR(L, "entity:get_mass", "1.5", LEVEL_VERSION_1_5);

    entity *e = *(static_cast<entity**>(luaL_checkudata(L, 1, "EntityMT")));

    b2Body *b = e->get_body(0);

    float value = 0.f;

    if (b) {
        value = b->GetMass();
    }

    lua_pushnumber(L, value);

    return 1;
}

/* entity:get_density() */
static int l_entity_get_density(lua_State *L)
{
    ESCRIPT_VERSION_ERROR(L, "entity:get_density", "1.5", LEVEL_VERSION_1_5);

    entity *e = *(static_cast<entity**>(luaL_checkudata(L, 1, "EntityMT")));

    b2Body *b = e->get_body(0);

    float value = 0.f;
    int num = 0;

    if (b) {
        for (b2Fixture *f = b->GetFixtureList(); f; f = f->GetNext()) {
            value += f->GetDensity();
            ++ num;
        }

        if (num) {
            value = value / num;
        }
    }

    lua_pushnumber(L, value);

    return 1;
}

/* entity:get_friction() */
static int l_entity_get_friction(lua_State *L)
{
    ESCRIPT_VERSION_ERROR(L, "entity:get_friction", "1.5", LEVEL_VERSION_1_5);

    entity *e = *(static_cast<entity**>(luaL_checkudata(L, 1, "EntityMT")));

    b2Body *b = e->get_body(0);

    float value = 0.f;
    int num = 0;

    if (b) {
        for (b2Fixture *f = b->GetFixtureList(); f; f = f->GetNext()) {
            value += f->GetFriction();
            ++ num;
        }

        if (num) {
            value = value / num;
        }
    }

    lua_pushnumber(L, value);

    return 1;
}

/* entity:get_restitution() */
static int l_entity_get_restitution(lua_State *L)
{
    ESCRIPT_VERSION_ERROR(L, "entity:get_restitution", "1.5", LEVEL_VERSION_1_5);

    entity *e = *(static_cast<entity**>(luaL_checkudata(L, 1, "EntityMT")));

    b2Body *b = e->get_body(0);

    float value = 0.f;
    int num = 0;

    if (b) {
        for (b2Fixture *f = b->GetFixtureList(); f; f = f->GetNext()) {
            value += f->GetRestitution();
            ++ num;
        }

        if (num) {
            value = value / num;
        }
    }

    lua_pushnumber(L, value);

    return 1;
}

/* entity:set_color(r, g, b) */
static int l_entity_set_color(lua_State *L)
{
    ESCRIPT_VERSION_ERROR(L, "entity:set_color", "1.5", LEVEL_VERSION_1_5);

    entity *e = *(static_cast<entity**>(luaL_checkudata(L, 1, "EntityMT")));

    float r = luaL_checknumber(L, 2);
    float g = luaL_checknumber(L, 3);
    float b = luaL_checknumber(L, 4);

    e->set_color4(r, g, b);

    return 0;
}

/* r, g, b, a = entity:get_color() */
static int l_entity_get_color(lua_State *L)
{
    ESCRIPT_VERSION_ERROR(L, "entity:get_color", "1.5", LEVEL_VERSION_1_5);

    entity *e = *(static_cast<entity**>(luaL_checkudata(L, 1, "EntityMT")));

    tvec4 c = e->get_color();

    lua_pushnumber(L, c.r);
    lua_pushnumber(L, c.g);
    lua_pushnumber(L, c.b);
    lua_pushnumber(L, c.a);

    return 4;
}

/* entity:disconnect_all() */
static int l_entity_disconnect_all(lua_State *L)
{
    ESCRIPT_VERSION_ERROR(L, "entity:disconnect_all", "1.5", LEVEL_VERSION_1_5);

    entity *e = *(static_cast<entity**>(luaL_checkudata(L, 1, "EntityMT")));

    e->disconnect_all();

    return 0;
}

/* entity:set_target_id(id) */
static int l_entity_set_target_id(lua_State *L)
{
    ESCRIPT_VERSION_ERROR(L, "entity:set_target_id", "1.5", LEVEL_VERSION_1_5);

    entity *e = *(static_cast<entity**>(luaL_checkudata(L, 1, "EntityMT")));

    if (!e->has_tracker()) {
        char error[512];
        snprintf(error, 511, "Can't use entity:set_target_id on a %s", e->get_name());
        lua_pushstring(L, error);
        lua_error(L);
        return 0;
    }

    uint32_t id = luaL_checkunsigned(L, 2);

    entity *target_entity = W->get_entity_by_id(id);

    switch (e->g_id) {
        case O_ROBOTMAN:
            {
                robotman *rm = static_cast<robotman*>(e);

                if (target_entity && target_entity->is_robot()) {
                    if (rm->get_target()) {
                        rm->unsubscribe((entity*)rm->get_target());

                        rm->set_target(static_cast<robot_base*>(target_entity));
                        rm->subscribe(target_entity, ENTITY_EVENT_REMOVE, on_robotman_target_absorbed);
                    }
                }
            }
            break;

        default:
            tms_errorf("set_target_id not implemented for %s", e->get_name());
            break;
    }

    return 0;
}


/* we pretend this is creature stuff */

/* hp, max_hp = creature:get_hp() */
static int l_creature_get_hp(lua_State *L)
{
    ESCRIPT_VERSION_ERROR(L, "creature:get_hp", "1.5", LEVEL_VERSION_1_5);

    entity *e = *(static_cast<entity**>(luaL_checkudata(L, 1, "EntityMT")));

    if (!e->is_creature()) {
        lua_pushstring(L, "Invalid creature.");
        lua_error(L);
        return 0;
    }

    creature *c = static_cast<creature*>(e);

    lua_pushnumber(L, c->get_hp());
    lua_pushnumber(L, c->get_max_hp());

    return 2;
}

/* armor, max_armor = creature:get_armor() */
static int l_creature_get_armor(lua_State *L)
{
    ESCRIPT_VERSION_ERROR(L, "creature:get_armor", "1.5", LEVEL_VERSION_1_5);

    entity *e = *(static_cast<entity**>(luaL_checkudata(L, 1, "EntityMT")));

    if (!e->is_creature()) {
        lua_pushstring(L, "Invalid creature.");
        lua_error(L);
        return 0;
    }

    creature *c = static_cast<creature*>(e);

    lua_pushnumber(L, c->get_armour());
    lua_pushnumber(L, c->get_max_armour());

    return 2;
}

/* aim = creature:get_aim() */
static int l_creature_get_aim(lua_State *L)
{
    ESCRIPT_VERSION_ERROR(L, "creature:get_aim", "1.5", LEVEL_VERSION_1_5);

    entity *e = *(static_cast<entity**>(luaL_checkudata(L, 1, "EntityMT")));

    if (!e->is_creature()) {
        lua_pushstring(L, "Invalid creature.");
        lua_error(L);
        return 0;
    }

    creature *c = static_cast<creature*>(e);

    float aim = c->get_aim();

    lua_pushnumber(L, aim);

    return 1;
}

/* creature:set_aim(new_aim) */
static int l_creature_set_aim(lua_State *L)
{
    ESCRIPT_VERSION_ERROR(L, "creature:set_aim", "1.5", LEVEL_VERSION_1_5);

    entity *e = *(static_cast<entity**>(luaL_checkudata(L, 1, "EntityMT")));

    if (!e->is_creature()) {
        lua_pushstring(L, "Invalid creature.");
        lua_error(L);
        return 0;
    }

    creature *c = static_cast<creature*>(e);

    float new_aim = luaL_checknumber(L, 2);
    c->aim(new_aim);

    return 0;
}

/* creature:stop(dir) */
static int l_creature_stop(lua_State *L)
{
    ESCRIPT_VERSION_ERROR(L, "creature:stop", "1.5", LEVEL_VERSION_1_5);

    entity *e = *(static_cast<entity**>(luaL_checkudata(L, 1, "EntityMT")));

    if (!e->is_creature()) {
        lua_pushstring(L, "Invalid creature.");
        lua_error(L);
        return 0;
    }

    creature *c = static_cast<creature*>(e);

    if (lua_gettop(L) > 1) {
        int dir = luaL_checkinteger(L, 2);

        if (dir == DIR_LEFT || dir == DIR_RIGHT || dir == DIR_UP || dir == DIR_DOWN) {
            c->stop_moving(dir);
            return 0;
        }
    }

    c->stop_moving(DIR_LEFT);
    c->stop_moving(DIR_RIGHT);
    c->stop_moving(DIR_UP);
    c->stop_moving(DIR_DOWN);

    return 0;
}

/* creature:move(dir) */
static int l_creature_move(lua_State *L)
{
    ESCRIPT_VERSION_ERROR(L, "creature:move", "1.5", LEVEL_VERSION_1_5);

    entity *e = *(static_cast<entity**>(luaL_checkudata(L, 1, "EntityMT")));

    if (!e->is_creature()) {
        lua_pushstring(L, "Invalid creature.");
        lua_error(L);
        return 0;
    }

    creature *c = static_cast<creature*>(e);

    int dir = luaL_checkinteger(L, 2);

    if (dir == DIR_LEFT || dir == DIR_RIGHT || dir == DIR_UP || dir == DIR_DOWN) {
        c->move(dir);
    } else {
        lua_pushstring(L, "Unknown direction.");
        lua_error(L);
    }

    return 0;
}

/* creature:is_action_active() */
static int l_creature_is_action_active(lua_State *L)
{
    ESCRIPT_VERSION_ERROR(L, "creature:is_action_active", "1.5", LEVEL_VERSION_1_5);

    entity *e = *(static_cast<entity**>(luaL_checkudata(L, 1, "EntityMT")));

    if (!e->is_creature()) {
        lua_pushstring(L, "Invalid creature.");
        lua_error(L);
        return 0;
    }

    creature *c = static_cast<creature*>(e);

    lua_pushboolean(L, c->is_action_active());

    return 1;
}

/* creature:action_on() */
static int l_creature_action_on(lua_State *L)
{
    ESCRIPT_VERSION_ERROR(L, "creature:action_on", "1.5", LEVEL_VERSION_1_5);

    entity *e = *(static_cast<entity**>(luaL_checkudata(L, 1, "EntityMT")));

    if (!e->is_creature()) {
        lua_pushstring(L, "Invalid creature.");
        lua_error(L);
        return 0;
    }

    creature *c = static_cast<creature*>(e);

    c->action_on();

    return 0;
}

/* creature:action_off() */
static int l_creature_action_off(lua_State *L)
{
    ESCRIPT_VERSION_ERROR(L, "creature:action_off", "1.5", LEVEL_VERSION_1_5);

    entity *e = *(static_cast<entity**>(luaL_checkudata(L, 1, "EntityMT")));

    if (!e->is_creature()) {
        lua_pushstring(L, "Invalid creature.");
        lua_error(L);
        return 0;
    }

    creature *c = static_cast<creature*>(e);

    c->action_off();

    return 0;
}


/* escript specific functions */

/* entity:call(functionname) */
static int l_escript_call(lua_State *L)
{
    ESCRIPT_VERSION_ERROR(L, "entity:call", "1.5", LEVEL_VERSION_1_5);

    entity *ent = *(static_cast<entity**>(luaL_checkudata(L, 1, "EntityMT")));

    if (ent->g_id != O_ESCRIPT) {
        lua_pushstring(L, "Invalid LuaScript object.");
        lua_error(L);
        return 0;
    }

    const char *function = luaL_checkstring(L, 2);
    int top = lua_gettop(L);

    escript *e = static_cast<escript*>(ent);

    if (!e->L) {
        lua_pushstring(L, "This LuaScript object has not had time to initialize itself yet.");
        lua_error(L);
        return 0;
    }

    lua_getglobal(e->L, function);

    if (!lua_isnil(e->L, -1)) {
        int num_arguments = 0;
        int offset = 2;

        for (int n=0; n<top-offset; ++n) {
            int arg = 1 + n + offset;
            if (lua_push_stuff(L, e->L, arg)) {
                ++ num_arguments;
            }
        }

        if (lua_pcall(e->L, num_arguments, 1, 0) != 0) {
            char prefix[1024];
            G->add_error(e, ERROR_SCRIPT_COMPILE, lua_pop_error(e->L, prefix));
        } else {
            // redirect return value to
            if (!lua_push_stuff(e->L, L, -1)) {
                lua_pushnumber(L, 1337);
            }

            lua_pop(e->L, 1);

            return 1;
        }
    } else {
        lua_pop(e->L, 1);
    }

    return 0;
}


/* ENTITY */
static const luaL_Reg entity_meta[] = {
    { "__persist", l_entity_persist },
    { NULL, NULL }
};

static const luaL_Reg entity_methods[] = {
#define LUA_REG(name) { #name, l_entity_##name }
    LUA_REG(get_id),
    LUA_REG(get_g_id),
    LUA_REG(get_position),
    LUA_REG(get_angle),
    LUA_REG(set_angle),
    LUA_REG(set_fixed_rotation),
    LUA_REG(is_fixed_rotation),
    LUA_REG(get_velocity),
    LUA_REG(get_angular_velocity),
    LUA_REG(get_bbox),
    LUA_REG(get_layer),
    LUA_REG(local_to_world),
    LUA_REG(world_to_local),
    LUA_REG(highlight),

    LUA_REG(damage),
    LUA_REG(is_static),
    LUA_REG(absorb),
    LUA_REG(apply_torque),
    LUA_REG(apply_force),
    LUA_REG(set_velocity),
    LUA_REG(warp),
    LUA_REG(show),
    LUA_REG(hide),
    LUA_REG(is_hidden),
    LUA_REG(get_name),
    LUA_REG(is_creature),
    LUA_REG(is_robot),
    LUA_REG(is_player),
    LUA_REG(get_mass),
    LUA_REG(get_density),
    LUA_REG(get_friction),
    LUA_REG(get_restitution),
    LUA_REG(set_color),
    LUA_REG(get_color),
    LUA_REG(disconnect_all),
    LUA_REG(set_target_id),
#undef LUA_REG

#define LUA_REG(name) { #name, l_creature_##name }
    /* we pretend this is creature stuff */
    LUA_REG(get_hp),
    LUA_REG(get_armor),
    LUA_REG(get_aim),
    LUA_REG(set_aim),
    LUA_REG(stop),
    LUA_REG(move),
    LUA_REG(is_action_active),
    LUA_REG(action_on),
    LUA_REG(action_off),
#undef LUA_REG

    /* escript specific stuff */
    {"call", l_escript_call},

    { NULL, NULL }
};

void register_entity(lua_State *L)
{
    int lib_id, meta_id;

    lua_createtable(L, 0, 0);
    lib_id = lua_gettop(L);

    luaL_newmetatable(L, "EntityMT");
    meta_id = lua_gettop(L);
    luaL_setfuncs(L, entity_meta, 0);

    luaL_newlib(L, entity_methods);
    lua_setfield(L, meta_id, "__index");

    luaL_newlib(L, entity_meta);
    lua_setfield(L, meta_id, "__metatable");

    lua_setmetatable(L, lib_id);
    lua_pop(L, 1);
}
