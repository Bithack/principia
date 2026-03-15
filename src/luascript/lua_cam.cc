#include "lua_cam.hh"
#include "lua_game.hh"
#include "adventure.hh"
#include "game.hh"

/* cam:get_position() */
static int l_cam_get_position(lua_State *L)
{
	lua_pushnumber(L, G->cam->_position.x);
	lua_pushnumber(L, G->cam->_position.y);
	lua_pushnumber(L, G->cam->_position.z);
	return 3;
}

/* cam:get_velocity() */
static int l_cam_get_velocity(lua_State *L)
{
	lua_pushnumber(L, G->cam_vel.x);
	lua_pushnumber(L, G->cam_vel.y);
	lua_pushnumber(L, G->cam_vel.z);
	return 3;
}

/* cam:set_position(x, y, z) */
static int l_cam_set_position(lua_State *L)
{
	double x = luaL_checknumber(L, 2);
	double y = luaL_checknumber(L, 3);
	double z = luaL_checknumber(L, 4);

	G->cam->_position.z = z;

	if (false && G->follow_object) {
		/* camera is following something, set its relative position */
		b2Vec2 fp = G->follow_object->get_position();

		x -= fp.x;
		y -= fp.y;

		G->cam_rel_pos.x = x;
		G->cam_rel_pos.y = y;
	} else {
		G->cam->_position.x = x;
		G->cam->_position.y = y;
	}

	return 0;
}

/* cam:set_velocity(x, y, z) */
static int l_cam_set_velocity(lua_State *L)
{
	// TODO: This probably needs to fiddle with the numbers whether the player has smooth cam enabled or not
	double x = luaL_checknumber(L, 2);
	double y = luaL_checknumber(L, 3);
	double z = luaL_checknumber(L, 4);
	G->cam_vel.x = x;
	G->cam_vel.y = y;
	G->cam_vel.z = z;

	return 0;
}

/* cam:follow_entity(entity, snap, preserve_position) */
static int l_cam_follow_entity(lua_State *L)
{
	entity *e = *(static_cast<entity**>(luaL_checkudata(L, 2, "EntityMT")));
	bool snap = lua_toboolean(L, 3);
	bool preserve_position = lua_toboolean(L, 4);

	G->set_follow_object(e, snap, preserve_position);

	return 0;
}

/* cam:follow_entity_by_id(entity_id, snap, preserve_position) */
static int l_cam_follow_entity_by_id(lua_State *L)
{
	long entity_id = luaL_checklong(L, 2);
	bool snap = lua_toboolean(L, 3);
	bool preserve_position = lua_toboolean(L, 4);

	entity *e = W->get_entity_by_id(entity_id);
	if (e) {
		G->set_follow_object(e, snap, preserve_position);
	}

	return 0;
}

/* cam:get_zoom_ratio() */
static int l_cam_get_zoom_ratio(lua_State *L)
{
	float cur_z = G->cam->_position.z;
	float max_z = 60.f;
	float min_z = 4.f;

	if (!W->level.flag_active(LVL_DISABLE_ADVENTURE_MAX_ZOOM) && !W->is_paused() && W->is_adventure() && G->follow_object == adventure::player) {
		max_z = 20.f;
	}

	lua_pushnumber(L, tclampf((cur_z-min_z)/(max_z-min_z), 0.f, 1.f));

	return 1;
}

// ---

static const luaL_Reg cam_meta[] = {
    { NULL, NULL }
};
static const luaL_Reg cam_methods[] = {
#define LUA_REG(name) { #name, l_cam_##name }
    LUA_REG(get_position),
    LUA_REG(get_velocity),
    LUA_REG(set_position),
    LUA_REG(set_velocity),
    LUA_REG(follow_entity),
    LUA_REG(follow_entity_by_id),

    LUA_REG(get_zoom_ratio),

    { NULL, NULL }
#undef LUA_REG
};

void register_cam(lua_State *L)
{
    int lib_id, meta_id;

    lua_createtable(L, 0, 0);
    lib_id = lua_gettop(L);

    luaL_newmetatable(L, "Cam");
    meta_id = lua_gettop(L);
    luaL_setfuncs(L, game_meta, 0); // XXX

    luaL_newlib(L, cam_methods);
    lua_setfield(L, meta_id, "__index");

    luaL_newlib(L, cam_meta);
    lua_setfield(L, meta_id, "__metatable");

    lua_setmetatable(L, lib_id);
    lua_setglobal(L, "cam");
}
