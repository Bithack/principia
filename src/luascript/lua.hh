#pragma once

extern "C" {
#include "lua.h"
#include "lstate.h"
#include "lualib.h"
#include "lauxlib.h"
#include "eris.h"

#ifdef BUILD_LUASOCKET
#include "luasocket/luasocket.h"
#endif
}

// used for version checks
#include "world.hh"

#define lua_pushint32(L, n) (lua_pushinteger(L, static_cast<int32_t>(n)))
#define lua_pushuint32(L, n) (lua_pushinteger(L, static_cast<uint32_t>(n)))

#define ESCRIPT_VERSION_ERROR(L, function_name, version_str, version_num) \
    if (W->level.version < version_num) { \
        lua_pushstring(L, function_name " requires a level created with version " version_str " or above."); \
        lua_error(L); \
        return 0; \
    }

#define ESCRIPT_FUNCTION_DEPRECATED(L, function_name, version_str, version_num) \
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

/*
 * Recursive function to push stuff!
 * Can currently handle:
 * Numbers, booleans, strings, nil, tables
 */
bool lua_push_stuff(lua_State *Lsrc, lua_State *Ldst, int pos);

const char* lua_pop_error(lua_State *L, const char *prefix);
