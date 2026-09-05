#pragma once

extern "C" {
#include "lua.h"
#include "lstate.h"
#include "lualib.h"
#include "lauxlib.h"

#include "eris.h"
}

// used for version checks
#include "world.hh"

#define lua_pushint32(L, n) (lua_pushinteger(L, static_cast<int32_t>(n)))
#define lua_pushuint32(L, n) (lua_pushinteger(L, static_cast<uint32_t>(n)))

#define LUASCRIPT_VERSION_ERROR(L, function_name, version_str, version_num) \
    if (W->level.version < version_num) { \
        lua_pushstring(L, function_name " requires a level created with version " version_str " or above."); \
        lua_error(L); \
        return 0; \
    }

#define LUASCRIPT_FUNCTION_DEPRECATED(L, function_name, version_str, version_num) \
    if (W->level.version > version_num) { \
        lua_pushstring(L, function_name " has been deprecated in " version_str " and should no longer be used."); \
        lua_error(L); \
        return 0; \
    }

#define MAX_SPRITES 512
#define TIMELIMIT 50
#define FIRST_RUN_TIMELIMIT   3000

#define FULL_SCRIPT_TIMELIMIT 5000

struct lua_vert {
    tvec3 pos;
    tvec2 uv;
    tvec4 color;
};

extern struct lua_vert sprite_base[4];

/**
 * Recursive function to push stuff!
 * Can currently handle:
 * Numbers, booleans, strings, nil, tables
 */
bool lua_push_stuff(lua_State *Lsrc, lua_State *Ldst, int pos);

const char* lua_pop_error(lua_State *L, const char *prefix);

void register_cam(lua_State *L);
void register_entity(lua_State *L);
void register_game(lua_State *L);
void register_this(lua_State *L, luascript *e);
void register_world(lua_State *L);

// needed by lua_entity
int l_world_unpersist_entity(lua_State *L);

extern const luaL_Reg game_meta[]; // needed for lua_cam
