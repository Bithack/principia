#pragma once

#include "lua.hh"

void register_game(lua_State *L);

extern const luaL_Reg game_meta[]; // needed for lua_cam
