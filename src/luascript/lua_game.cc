#include "lua_game.hh"
#include "escript.hh"
#include "game.hh"
#include "pkgman.hh"
#include "ui.hh"

/* GAME */

/* game:show_numfeed(number, num_decimals) */
static int l_game_show_numfeed(lua_State *L)
{
	int num_decimals = 2;
	if (lua_gettop(L) == 3) {
		num_decimals = luaL_checkint(L, 3);
	}
	double d = luaL_checknumber(L, 2);
	if (num_decimals < 0) num_decimals = 0;
	if (num_decimals > 6) num_decimals = 6;

	G->show_numfeed(d, num_decimals);
	return 0;
}

/* game:finish(win) */
static int l_game_finish(lua_State *L)
{
	ESCRIPT_VERSION_ERROR(L, "game:finish", "1.3.0.2", LEVEL_VERSION_1_3_0_2);
	bool win = (luaL_checkint(L, 2) == 1);
	G->finish(win);

	return 0;
}

/* game:add_score(score) */
static int l_game_add_score(lua_State *L)
{
	ESCRIPT_VERSION_ERROR(L, "game:add_score", "1.3.0.2", LEVEL_VERSION_1_3_0_2);
	int score = luaL_checkint(L, 2);
	G->add_score(score);

	return 0;
}

/* game:set_score(new_score) */
static int l_game_set_score(lua_State *L)
{
	ESCRIPT_VERSION_ERROR(L, "game:set_score", "1.3.0.2", LEVEL_VERSION_1_3_0_2);

	int new_score = luaL_checkint(L, 2);
	G->set_score(new_score);

	return 0;
}

/* game:get_score() */
static int l_game_get_score(lua_State *L)
{
	ESCRIPT_VERSION_ERROR(L, "game:get_score", "1.3.0.2", LEVEL_VERSION_1_3_0_2);

	lua_pushnumber(L, G->get_real_score());
	return 1;
}

/* game:activate_rc(entity) */
static int l_game_activate_rc(lua_State *L)
{
	ESCRIPT_VERSION_ERROR(L, "game:activate_rc", "1.3.0.2", LEVEL_VERSION_1_3_0_2);

	entity *e = *(static_cast<entity**>(luaL_checkudata(L, 2, "EntityMT")));
	G->set_control_panel(e);
	return 0;
}

/* game:activate_rc_by_id(entity_id) */
static int l_game_activate_rc_by_id(lua_State *L)
{
	ESCRIPT_VERSION_ERROR(L, "game:activate_rc_by_id", "1.3.0.2", LEVEL_VERSION_1_3_0_2);

	long entity_id = luaL_checklong(L, 2);
	entity *e = W->get_entity_by_id(entity_id);
	if (e) {
		G->set_control_panel(e);
	}

	return 0;
}

/* game:message(msg, duration) */
static int l_game_message(lua_State *L)
{
	ESCRIPT_VERSION_ERROR(L, "game:message", "1.4", LEVEL_VERSION_1_4);

	bool long_duration = false;
	if (lua_gettop(L) == 3) {
		long_duration = luaL_checkint(L, 3) ? true : false;
	}
	const char *s = luaL_checkstring(L, 2);

	ui::message(s, long_duration);
	return 0;
}

/* local x, y = game:get_cursor(layer) */
static int l_game_get_cursor(lua_State *L)
{
	ESCRIPT_VERSION_ERROR(L, "game:get_cursor", "1.4", LEVEL_VERSION_1_4);

	int layer = 1;

	if (lua_gettop(L) == 2)
		layer = luaL_checkinteger(L, 2);

	if (layer > 3) layer = 3;
	else if (layer < 1) layer = 1;

	layer --;

	b2Vec2 cp = G->get_last_cursor_pos(layer);

	lua_pushnumber(L, cp.x);
	lua_pushnumber(L, cp.y);

	return 2;
}

/* game:poll_event(event_id) */
static int l_game_poll_event(lua_State *L)
{
	ESCRIPT_VERSION_ERROR(L, "game:poll_event", "1.4", LEVEL_VERSION_1_4);

	int ev = luaL_checkinteger(L, 2);

	if (ev < 0) {
		ev = 0;
	} else if (ev >= WORLD_EVENT__NUM) {
		ev = WORLD_EVENT__NUM-1;
	}

	lua_pushboolean(L, current_escript->events[ev] > 0);
	return 1;
}

/* local x, y = game:get_screen_cursor() */
static int l_game_get_screen_cursor(lua_State *L)
{
	ESCRIPT_VERSION_ERROR(L, "game:get_screen_cursor", "1.5", LEVEL_VERSION_1_5);

	G->refresh_last_cursor_pos();

	lua_pushnumber(L, G->last_cursor_pos_x);
	lua_pushnumber(L, G->last_cursor_pos_y);

	return 2;
}

/* game:restart() */
static int l_game_restart(lua_State *L)
{
	ESCRIPT_VERSION_ERROR(L, "game:restart", "1.5", LEVEL_VERSION_1_5);

	G->restart_level();

	return 0;
}

/* game:submit_score() */
static int l_game_submit_score(lua_State *L)
{
	ESCRIPT_VERSION_ERROR(L, "game:submit_score", "1.5", LEVEL_VERSION_1_5);

	if (W->level_id_type == LEVEL_DB)
		G->submit_score();
	else
		ui::message("Can't submit score when playing a local level.");

	return 0;
}

/* game:set_variable(varname, value) */
static int l_game_set_variable(lua_State *L)
{
	ESCRIPT_VERSION_ERROR(L, "game:set_variable", "1.5", LEVEL_VERSION_1_5);

	const char *name = luaL_checkstring(L, 2);
	double value = luaL_checknumber(L, 3);

	std::pair<std::map<std::string, double>::iterator, bool> ret;
	ret = W->level_variables.insert(std::pair<std::string, double>(name, value));

	if (!ret.second) {
		(ret.first)->second = value;
	}

	return 0;
}

/* value = game:get_variable(varname) */
static int l_game_get_variable(lua_State *L)
{
	ESCRIPT_VERSION_ERROR(L, "game:get_variable", "1.5", LEVEL_VERSION_1_5);

	const char *name = luaL_checkstring(L, 2);

	double value = 0.0;

	std::map<std::string, double>::iterator i = W->level_variables.find(name);
	if (i != W->level_variables.end()) {
		value = i->second;
	}

	lua_pushnumber(L, value);

	return 1;
}

/* fps = game:get_fps() */
static int l_game_get_fps(lua_State *L)
{
	ESCRIPT_VERSION_ERROR(L, "game:get_fps", "1.5", LEVEL_VERSION_1_5);

	lua_pushnumber(L, _tms.fps_mean);

	return 1;
}

/* prompt_id = game:prompt(message, btn1, btn2, btn3) */
static int l_game_prompt(lua_State *L)
{
	ESCRIPT_VERSION_ERROR(L, "game:prompt", "1.5.1", LEVEL_VERSION_1_5_1);

	escript *self = static_cast<escript*>(L->userdata);

	if (self->prompt_id == 0 && self->get_response() == PROMPT_RESPONSE_NONE && lua_gettop(L) >= 3) {
		if (self->p_message) {
			free(self->p_message);
			self->p_message = 0;
		}

		if (self->p_btn1) {
			free(self->p_btn1);
			self->p_btn1 = 0;
			self->p_btn1_len = 0;
		}

		if (self->p_btn2) {
			free(self->p_btn2);
			self->p_btn2 = 0;
			self->p_btn2_len = 0;
		}

		if (self->p_btn3) {
			free(self->p_btn3);
			self->p_btn3 = 0;
			self->p_btn3_len = 0;
		}

		const char *message = luaL_checkstring(L, 2);
		const char *btn1 = luaL_checkstring(L, 3);

		if (lua_gettop(L) >= 4) {
			const char *btn2 = luaL_checkstring(L, 4);

			if (lua_gettop(L) >= 5) {
				const char *btn3 = luaL_checkstring(L, 5);

				self->p_btn3     = strdup(btn3);
				self->p_btn3_len = strlen(btn3);
			}

			self->p_btn2      = strdup(btn2);
			self->p_btn2_len = strlen(btn2);
		}

		if (!G->occupy_prompt_slot()) {
			lua_pushnil(L);
			return 1;
		}

		self->p_message  = strdup(message);
		self->p_btn1     = strdup(btn1);
		self->p_btn1_len = strlen(btn1);

		self->prompt_id = rand() % 65535;

		lua_pushnumber(L, self->prompt_id);

		G->current_prompt = self;
		ui::open_dialog(DIALOG_PROMPT, 0);

		return 1;
	}

	/*
	tms_infof("prompt id: %u", self->prompt_id);
	tms_infof("response: %u", self->get_response());
	*/

	lua_pushnil(L);
	return 1;
}

// ---

const luaL_Reg game_meta[] = {
    { NULL, NULL }
};
static const luaL_Reg game_methods[] = {
#define LUA_REG(name) { #name, l_game_##name }
    LUA_REG(show_numfeed),

    LUA_REG(finish),
    LUA_REG(add_score),
    LUA_REG(set_score),
    LUA_REG(get_score),
    LUA_REG(activate_rc),
    LUA_REG(activate_rc_by_id),

    LUA_REG(message),
    LUA_REG(get_cursor),
    LUA_REG(poll_event),

    LUA_REG(get_screen_cursor),
    LUA_REG(restart),
    LUA_REG(submit_score),
    LUA_REG(set_variable),
    LUA_REG(get_variable),
    LUA_REG(get_fps),

    LUA_REG(prompt),

    { NULL, NULL }
#undef LUA_REG
};

void register_game(lua_State *L)
{
    int lib_id, meta_id;

    lua_createtable(L, 0, 0);
    lib_id = lua_gettop(L);

    luaL_newmetatable(L, "Game");
    meta_id = lua_gettop(L);
    luaL_setfuncs(L, game_meta, 0);

    luaL_newlib(L, game_methods);
    lua_setfield(L, meta_id, "__index");

    luaL_newlib(L, game_meta);
    lua_setfield(L, meta_id, "__metatable");

    lua_setmetatable(L, lib_id);
    lua_setglobal(L, "game");
}
