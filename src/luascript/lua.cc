#include "lua.hh"

bool lua_push_stuff(lua_State *Lsrc, lua_State *Ldst, int pos)
{
    if (lua_isnumber(Lsrc, pos)) {
        lua_pushnumber(Ldst, lua_tonumber(Lsrc, pos));
    } else if (lua_isboolean(Lsrc, pos)) {
        lua_pushboolean(Ldst, lua_tonumber(Lsrc, pos));
    } else if (lua_isstring(Lsrc, pos)) {
        lua_pushstring(Ldst, lua_tostring(Lsrc, pos));
    } else if (lua_islightuserdata(Lsrc, pos)) {
        lua_pushlightuserdata(Ldst, lua_touserdata(Lsrc, pos)); // XXX: ? When will this be called? What is light userdata? :D
    } else if (lua_isuserdata(Lsrc, pos)) {
        lua_pushlightuserdata(Ldst, lua_touserdata(Lsrc, pos));
        void *p = luaL_testudata(Lsrc, pos, "EntityMT");
        if (luaL_testudata(Lsrc, pos, "EntityMT")) {
            luaL_setmetatable(Ldst, "EntityMT");
        } else if (luaL_testudata(Lsrc, pos, "This")) {
            luaL_setmetatable(Ldst, "This");
        } else {
            // unknown userdata
        }
    } else if (lua_isnil(Lsrc, pos)) {
        lua_pushnil(Ldst);
    } else if (lua_istable(Lsrc, pos)) {
        lua_newtable(Ldst);

        lua_pushnil(Lsrc); // create place on stack to store key
        if (pos < 0) {
            pos = pos-1;
        }

        tms_assertf(lua_type(Lsrc, pos) == LUA_TTABLE, "item at stack pos is not a table");
        while (lua_next(Lsrc, pos)) {
            lua_push_stuff(Lsrc, Ldst, -2); // push key

            lua_push_stuff(Lsrc, Ldst, -1); // push value

            lua_settable(Ldst, -3); // bind the values into the table we created

            lua_pop(Lsrc, 1); // pop value. we will reuse key
        }

        if (lua_type(Lsrc, -1) != LUA_TTABLE) {
            lua_pop(Lsrc, 1); // pop key
        }
    } else {
        // some unidentifiable argument
        return false;
    }

    return true;
}

static char error_message[1024];

const char* lua_pop_error(lua_State *L, const char *prefix)
{
    tms_assertf(lua_gettop(L) >= 1, "No error found on the stack");

    const char* err_string = luaL_tolstring(L, -1, NULL);

    if (err_string == 0) {
        //XXX: luaL_tolstring shouldn't push anything in case of failure
        lua_pop(L, 1);
        return "";
    }

    snprintf(error_message, 1023, "%s%s", prefix, err_string);

    // S: error string, error value
    lua_pop(L, 2);

    return error_message;
}
