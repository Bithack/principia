#pragma once

#include "lua.hh"

void register_world(lua_State *L);

// needed by lua_entity
int l_world_unpersist_entity(lua_State *L);
