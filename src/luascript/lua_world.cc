#include "lua_world.hh"
#include "entity.hh"
#include "game.hh"

/* world:get_entity(entity_id) */
static int l_world_get_entity(lua_State *L)
{
	uint32_t id = luaL_checkunsigned(L, 2);
	entity *e = W->get_entity_by_id(id);
	if (e) {
		entity **ee = static_cast<entity**>(lua_newuserdata(L, sizeof(entity*)));
		*(ee) = e;

		luaL_setmetatable(L, "EntityMT");

		// we _ALWAYS_ assume userdata is a pointer to this.
		//escript *es = static_cast<escript*>(L->userdata);
		//es->subscribe(e, ENTITY_EVENT_REMOVE, on_entity_absorb);
	} else {
		lua_pushnil(L);
	}

	return 1; /* We push one value to the lua stack */
}

/* entity, ptx, pty, norx, nory = world:raycast(startx, starty, endx, endy, layer) */
static int l_world_raycast(lua_State *L)
{
	ESCRIPT_VERSION_ERROR(L, "world:raycast", "1.4", LEVEL_VERSION_1_4);
	float x1 = luaL_checknumber(L, 2);
	float y1 = luaL_checknumber(L, 3);
	float x2 = luaL_checknumber(L, 4);
	float y2 = luaL_checknumber(L, 5);
	int layer = 1;
	int sublayer_mask = 15;

	if (lua_gettop(L) > 5) {
		layer = luaL_checkinteger(L, 6);

		if (lua_gettop(L) == 7) {
			sublayer_mask = luaL_checkinteger(L, 7);
		}
	}

	sublayer_mask &= 15;
	if (layer < 1) layer = 1;
	else if (layer > 3) layer = 3;

	layer --;

	class callback : public b2RayCastCallback {
		public:
		int mask;
		entity *result;
		b2Vec2 result_pt;
		b2Vec2 result_nor;

		callback(int layer, int sublayer_mask)
		{
			this->mask = (sublayer_mask << (layer*4));
			this->result = 0;
		}

		float32 ReportFixture(b2Fixture *f, const b2Vec2 &pt, const b2Vec2 &nor, float32 fraction)
		{
			entity *r = static_cast<entity*>(f->GetUserData());

			if (f->IsSensor()) return -1;

			if (r) {
				if (f->GetFilterData().categoryBits & this->mask) {
					result = r;
					result_pt = pt;
					result_nor = nor;
					return fraction;
				}
			}

			return -1;
		}
	} cb(layer, sublayer_mask);

	W->b2->RayCast(&cb, b2Vec2(x1, y1), b2Vec2(x2, y2));

	if (cb.result) {
		entity **ee = static_cast<entity**>(lua_newuserdata(L, sizeof(entity*)));
		*(ee) = cb.result;

		luaL_setmetatable(L, "EntityMT");

		lua_pushnumber(L, cb.result_pt.x);
		lua_pushnumber(L, cb.result_pt.y);
		lua_pushnumber(L, cb.result_nor.x);
		lua_pushnumber(L, cb.result_nor.y);
		return 5;
	} else {
		lua_pushnil(L);
		return 1;
	}
}

/* world:query(min_x, min_y, max_x, max_y, layer, sublayers) */
static int l_world_query(lua_State *L)
{
	ESCRIPT_VERSION_ERROR(L, "world:query", "1.4", LEVEL_VERSION_1_4);
	float x1 = luaL_checknumber(L, 2);
	float y1 = luaL_checknumber(L, 3);
	float x2 = luaL_checknumber(L, 4);
	float y2 = luaL_checknumber(L, 5);
	int layer = 1;
	int sublayer_mask = 15;

	if (lua_gettop(L) > 5) {
		layer = luaL_checkinteger(L, 6);

		if (lua_gettop(L) == 7) {
			sublayer_mask = luaL_checkinteger(L, 7);
		}
	}

	sublayer_mask &= 15;
	if (layer < 1) layer = 1;
	else if (layer > 3) layer = 3;

	layer --;

	class callback : public b2QueryCallback {
		public:
		int mask;
		std::set<entity*> results;

		callback(int layer, int sublayer_mask)
		{
			this->mask = (sublayer_mask << (layer*4));
		}

		bool ReportFixture(b2Fixture *f)
		{
			entity *r = static_cast<entity*>(f->GetUserData());

			if (f->IsSensor()) return true;

			if (r) {
				if (f->GetFilterData().categoryBits & this->mask) {
					this->results.insert(r);
				}
			}

			return true;
		}
	} cb(layer, sublayer_mask);

	b2AABB aabb;
	aabb.lowerBound.Set(x1, y1);
	aabb.upperBound.Set(x2, y2);
	W->b2->QueryAABB(&cb, aabb);

	lua_newtable(L);
	int x = 1;
	for (std::set<entity*>::iterator i = cb.results.begin(); i != cb.results.end(); i ++, x++) {
		lua_pushnumber(L, x);
		entity **ee = static_cast<entity**>(lua_newuserdata(L, sizeof(entity*)));
		*(ee) = (*i);
		luaL_setmetatable(L, "EntityMT");
		lua_settable(L, -3);
	}
	return 1;
}

/* x, y = world:get_gravity() */
static int l_world_get_gravity(lua_State *L)
{
	ESCRIPT_VERSION_ERROR(L, "world:get_gravity", "1.4", LEVEL_VERSION_1_4);

	b2Vec2 g = W->get_gravity();
	lua_pushnumber(L, g.x);
	lua_pushnumber(L, g.y);
	return 2;
}

/* world:set_gravity(x, y) */
static int l_world_set_gravity(lua_State *L)
{
	ESCRIPT_VERSION_ERROR(L, "world:set_gravity", "1.5.1", LEVEL_VERSION_1_5_1);

	float x = luaL_checknumber(L, 2);
	float y = luaL_checknumber(L, 3);
	W->set_gravity(x, y);

	return 0;
}

/* id = world:get_adventure_id() */
static int l_world_get_adventure_id(lua_State *L)
{
	ESCRIPT_VERSION_ERROR(L, "world:get_adventure_id", "1.5", LEVEL_VERSION_1_5);

	lua_pushnumber(L, W->level.get_adventure_id());
	return 1;
}

/* bup, bdown, bleft, bright = world:get_borders() */
static int l_world_get_borders(lua_State *L)
{
	ESCRIPT_VERSION_ERROR(L, "world:get_borders", "1.5", LEVEL_VERSION_1_5);

	lua_pushnumber(L, W->level.size_y[1]);
	lua_pushnumber(L, W->level.size_y[0]);
	lua_pushnumber(L, W->level.size_x[0]);
	lua_pushnumber(L, W->level.size_x[1]);
	return 4;
}

/* x, y = world:get_world_point(x, y) */
static int l_world_get_world_point(lua_State *L)
{
	ESCRIPT_VERSION_ERROR(L, "world:get_world_point", "1.5", LEVEL_VERSION_1_5);

	float x = luaL_checknumber(L, 2);
	float y = luaL_checknumber(L, 3);
	int layer = 1;

	if (lua_gettop(L) > 3) {
		layer = luaL_checkinteger(L, 4);
	}

	if (layer < 1) layer = 1;
	else if (layer > 3) layer = 3;

	-- layer;

	tvec3 out;

	W->get_layer_point(G->cam, x, y, layer, &out);

	lua_pushnumber(L, out.x);
	lua_pushnumber(L, out.y);
	return 2;
}

/* x, y = world:get_screen_point(x, y) */
static int l_world_get_screen_point(lua_State *L)
{
	ESCRIPT_VERSION_ERROR(L, "world:get_screen_point", "1.5", LEVEL_VERSION_1_5);

	float x = luaL_checknumber(L, 2);
	float y = luaL_checknumber(L, 3);

	lua_pushnumber(L, (x / _tms.window_width)  * 100.);
	lua_pushnumber(L, (y / _tms.window_height) * 100.);
	return 2;
}

/* world:set_bg_color(r, g, b) */
static int l_world_set_bg_color(lua_State *L)
{
	ESCRIPT_VERSION_ERROR(L, "world:set_bg_color", "1.5", LEVEL_VERSION_1_5);

	float r = luaL_checknumber(L, 2);
	float g = luaL_checknumber(L, 3);
	float b = luaL_checknumber(L, 4);

	G->state.bg_color.r = r;
	G->state.bg_color.g = g;
	G->state.bg_color.b = b;

	if (G->bgent) {
		G->bgent->set_color4(r, g, b);
	}

	return 0;
}

/* world:set_ambient_light(intensity) */
static int l_world_set_ambient_light(lua_State *L)
{
	ESCRIPT_VERSION_ERROR(L, "world:set_ambient_light", "1.5", LEVEL_VERSION_1_5);

	float i = luaL_checknumber(L, 2);

	G->tmp_ambientdiffuse.x = i;

	return 0;
}

/* world:set_diffuse_light(intensity) */
static int l_world_set_diffuse_light(lua_State *L)
{
	ESCRIPT_VERSION_ERROR(L, "world:set_diffuse_light", "1.5", LEVEL_VERSION_1_5);

	float i = luaL_checknumber(L, 2);

	G->tmp_ambientdiffuse.y = i;

	return 0;
}

// This is a secret function!
int l_world_unpersist_entity(lua_State *L)
{
	uint32_t id = lua_tounsigned(L, lua_upvalueindex(1));

	tms_debugf("Attempting to entity userdata with id %u", id);
	entity *e = W->get_entity_by_id(id);

	entity **ee = static_cast<entity**>(lua_newuserdata(L, sizeof(entity*)));
	*(ee) = e;

	luaL_setmetatable(L, "EntityMT");

	/* If get_entity_by_id returns 0, the entity userdata will be "invalid".
		* This might be something we should check for in all
		* functions that use a raw pointer. */

	return 1;
}

// ---

static const luaL_Reg world_meta[] = {
    { NULL, NULL }
};
static const luaL_Reg world_methods[] = {
#define LUA_REG(name) { #name, l_world_##name }
    {"get_entity_by_id", l_world_get_entity}, // backward compat
    LUA_REG(get_entity),

    LUA_REG(raycast),
    LUA_REG(query),
    LUA_REG(get_gravity),
    LUA_REG(set_gravity),

    LUA_REG(get_adventure_id),
    LUA_REG(get_borders),
    LUA_REG(get_world_point),
    LUA_REG(get_screen_point),
    LUA_REG(set_bg_color),
    LUA_REG(set_ambient_light),
    LUA_REG(set_diffuse_light),

    // private! ;-)
    {"___persist_entity", l_world_unpersist_entity},

    { NULL, NULL }
#undef LUA_REG
};

void register_world(lua_State *L)
{
    int lib_id, meta_id;

    lua_createtable(L, 0, 0);
    lib_id = lua_gettop(L);

    luaL_newmetatable(L, "World");
    meta_id = lua_gettop(L);
    luaL_setfuncs(L, world_meta, 0);

    luaL_newlib(L, world_methods);
    lua_setfield(L, meta_id, "__index");

    luaL_newlib(L, world_meta);
    lua_setfield(L, meta_id, "__metatable");

    lua_setmetatable(L, lib_id);
    lua_setglobal(L, "world");
}
