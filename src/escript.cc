#include "escript.hh"
#include "model.hh"
#include "game.hh"
#include "receiver.hh"
#include "linebuffer.hh"
#include "receiver.hh"
#include "ui.hh"
#include "robotman.hh"
#include "robot_base.hh"
#include "adventure.hh"

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

typedef std::vector<struct escript_sprite>::iterator sprite_iterator;

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

// Blacklist of global namespace functions that are not allowed to be called for security reasons.
static const char* blacklist[] = {"load", "loadfile", "dofile", NULL};

enum {
    FUNC_GLOBAL_INIT,
    FUNC_INIT,
    FUNC_ON_EVENT,
    FUNC_ON_INPUT,
    FUNC_STEP,
    FUNC_ON_HALT,
    FUNC_ON_RESPONSE,

    FUNC_IGNORE,
};

struct function_info function_info[] = {
    { "init",        3000 },
    { "global init", 3000 },
    { "on_event",     300 },
    { "on_input",      50 },
    { "step",         400 },
    { "on_halt",       50 },
    { "on_response",  300 },

    { "NONE",           0 },
};

static const int LUA_FOREACH_MAX_DEPTH = 35;

/*
 * plua_foreach(...)
 * [-0, +0, -]
 *
 * iterate the given table at `index'.
 * recursively iterate any other tables we find.
 * ignore any keys for the first iteration in in the "ignore"-list.
 * any value found that is not a table will be called with the given `cb'
 */
static void plua_foreach(lua_State *L, const int index, void (*cb)(lua_State*, int, void*), void *userdata, bool can_ignore=true);

/*
 * invalidate_entity(...)
 * [-0, +0, -]
 *
 * Check if the value at the given index is an entity pointer.
 * If that entity pointer matches `userdata', we invalidate the entity pointer.
 *
 * `index' must be the index of a value in a table.
 */
static void invalidate_entity(lua_State *L, const int index, void *userdata);

/*
 * subscribe_to_entity(...)
 * [-0, +0, -]
 *
 * Check if the value at the given index is an entity pointer.
 * Userdata is a pointer to the LuaScript object caller.
 * The LuaScript object will subscribe to the given entity pointer.
 *
 * `index' must be the index of a value in a table.
 */
static void subscribe_to_entity(lua_State *L, const int index, void *userdata);

/*
 * Add one object to the table of objects that should not
 * be touched. Returns true if the object was non nil and
 * therefore stored, false otherwise.
 */
static bool m_add_object_to_not_persist
(lua_State * L, std::string name, uint32_t nidx)
{
    // Search for a dot. If one is found, we first have
    // to get the global module.
    std::string::size_type pos = name.find('.');

    if (pos != std::string::npos) {
        std::string table = name.substr(0, pos);
        name = name.substr(pos + 1);

        lua_getglobal(L, table.c_str()); // table object table
        assert(!lua_isnil(L, -1));

        lua_getfield(L, -1, name.c_str()); // table object table function
        if (lua_isnil(L, -1)) {
            lua_pop(L, 1);
            return false;
        }

        lua_pushint32(L, nidx); // table object table function int
        lua_settable(L, 1); //  newtable[function] = int
        lua_pop(L, 1); // pop tabltable
    } else {
        lua_getglobal(L, name.c_str()); // stack: table object value
        if (lua_isnil(L, -1)) {
            lua_pop(L, 1);
            return false;
        }
        lua_pushint32(L, nidx); // stack: table object value int
        lua_settable(L, 1); //  table[symbol] = integer
    }
    return true;
}

// Special handling for the upvalues of pairs and ipairs which are iterator
// functions, but always the same and therefor need not be persisted (in fact
// they are c functions, so they can't be persisted all the same)
static void m_add_iterator_function_to_not_persist
(lua_State * L, std::string global, uint32_t idx)
{
    lua_getglobal(L, global.c_str());
    lua_newtable(L);
    lua_call(L, 1, 1); // pairs{}, stack now contains iterator function
    lua_pushuint32(L, idx);
    lua_settable(L, 1); //  table[function] = integer
}

static bool m_add_object_to_not_unpersist
(lua_State * L, std::string name, uint32_t idx) {
    // S: ... globals

    // Search for a dot. If one is found, we first have
    // to get the global module.
    const std::string::size_type pos = name.find('.');

    if (pos != std::string::npos) {
        const std::string table = name.substr(0, pos);
        name = name.substr(pos + 1);

        lua_getglobal(L, table.c_str()); // S: ... gtables table
        assert(!lua_isnil(L, -1)); // table must already exist!

        lua_pushint32(L, idx); // S: ... gtables table idx

        lua_getfield(L, -2, name.c_str()); // S: ... gtables table idx function
        assert(!lua_isnil(L, -1)); // function must already exist

        lua_settable(L, -4); //  gtables[int] = function, S: ... gtables table
        lua_pop(L, 1); // S: ... gtables
    } else {
        lua_pushint32(L, idx); // S: ... gtable int
        lua_getglobal(L, name.c_str()); // S: ... gtable int object
        lua_settable(L, -3); // S: gtable[int] = object
    }
    return true;
}

static void m_add_iterator_function_to_not_unpersist
(lua_State * L, std::string global, uint32_t idx)
{
    lua_pushuint32(L, idx); // S: ... globals idx
    lua_getglobal(L, global.c_str());  // S: ... globals idx "pairs"
    lua_newtable(L);  // S: ... globals idx "pairs" table
    lua_call(L, 1, 1); // calls, pairs {}: ... globals idx iterator_func
    lua_settable(L, -3); //  globals[int] = function, S: ... globals
}

static Uint32 start_tick = 0; /* set in solve electronics before the script is started */
static Uint32 func_start_tick = 0; /* set in solve electronics before a function is called */
static bool is_first_run = false;
static int cur_func_id = FUNC_IGNORE;
static escript *current_escript = 0; /* only used in poll_event, which is deprecated in 1.5 */
static int timelimit = 3000;
static bool do_call_on_halt = false;

const char *before_code =
"local EVENT_PLAYER_DIE = 0;"
"local EVENT_ENEMY_DIE = 1;"
"local EVENT_INTERACTIVE_DESTROY = 2;"
"local EVENT_PLAYER_RESPAWN = 3;"
"local EVENT_CLICK_DOWN = 4;"
"local EVENT_CLICK_UP = 5;"
"local EVENT_ABSORB = 6;"
"local INPUT_KEY_DOWN = 4;"
"local INPUT_KEY_UP = 8;"
"local INPUT_POINTER_DOWN = 16;"
"local INPUT_POINTER_UP = 32;"
"local KEY_A = 4;"
"local KEY_B = 5;"
"local KEY_C = 6;"
"local KEY_D = 7;"
"local KEY_E = 8;"
"local KEY_F = 9;"
"local KEY_G = 10;"
"local KEY_H = 11;"
"local KEY_I = 12;"
"local KEY_J = 13;"
"local KEY_K = 14;"
"local KEY_L = 15;"
"local KEY_M = 16;"
"local KEY_N = 17;"
"local KEY_O = 18;"
"local KEY_P = 19;"
"local KEY_Q = 20;"
"local KEY_R = 21;"
"local KEY_S = 22;"
"local KEY_T = 23;"
"local KEY_U = 24;"
"local KEY_V = 25;"
"local KEY_W = 26;"
"local KEY_X = 27;"
"local KEY_Y = 28;"
"local KEY_Z = 29;"
"local KEY_1 = 30;"
"local KEY_2 = 31;"
"local KEY_3 = 32;"
"local KEY_4 = 33;"
"local KEY_5 = 34;"
"local KEY_6 = 35;"
"local KEY_7 = 36;"
"local KEY_8 = 37;"
"local KEY_9 = 38;"
"local KEY_0 = 39;"
"local KEY_ENTER = 40;"
"local KEY_ESC = 41;"
"local KEY_BACKSPACE = 42;"
"local KEY_TAB = 43;"
"local KEY_SPACE = 44;"
"local KEY_RIGHT = 79;"
"local KEY_LEFT = 80;"
"local KEY_DOWN = 81;"
"local KEY_UP = 82;"
;
const char *after_code = "";
const char *default_code_old =
"-- This is a new LuaScript object created in a level version below 1.5.\n"
"-- It will not support modern LuaScript functionality such as the init()\n"
"-- and step() callbacks. To use modern LuaScript please upgrade the level\n"
"-- to the latest version.\n"
;
const char *default_code =
"function init(is_sandbox)\n"
"\t\n"
"end\n"
"function step(count)\n"
"\t\n"
"end\n"
;

struct vert {
    tvec3 pos;
    tvec2 uv;
    tvec4 color;
};

static char error_message[1024];

static const char* lua_pop_error(lua_State *L, const char *prefix="Lua error: ")
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

static void
lua_dump_stack(lua_State *L)
{
    int top = lua_gettop(L);
    tms_printf("Total in stack: %d\n", top);

    for (int i = 1; i <= top; ++i) {
        int t = lua_type(L, i);
        tms_printf("[%d] ", i);
        switch (t) {
            case LUA_TSTRING: {
                lua_pushvalue(L, i);
                const char *str = lua_tostring(L, -1);
                lua_pop(L, 1);
                tms_printf("string: '%s'\n", str);
            } break;
            case LUA_TBOOLEAN:
                tms_printf("boolean %s\n",lua_toboolean(L, i) ? "true" : "false");
                break;
            case LUA_TNUMBER:
                tms_printf("number: %g\n", lua_tonumber(L, i));
                break;
            default:
                tms_printf("%s\n", lua_typename(L, t));
                break;
        }
        if (i+1 <= top) {
            tms_printf("  ");
        }
    }
}

static void
on_entity_remove(entity *self, void *userdata)
{
    entity *e = static_cast<entity*>(userdata);
    escript *es = static_cast<escript*>(self);

    tms_assertf(lua_gettop(es->L) == 0, "on_entity_removed with a dirty stack");

    lua_pushglobaltable(es->L); // S: tbl
    plua_foreach(es->L, 1, invalidate_entity, e); // S: tbl
    lua_pop(es->L, 1); // S:

    es->unsubscribe(e);
}

static const char* ignore[] = {
    "_VERSION", "assert", "collectgarbage", "coroutine", "debug", "dofile",
    "error", "gcinfo" "getfenv", "getmetatable", "io", "ipairs", "load",
    "loadfile", "loadstring", "math", "module", "newproxy", "next", "os",
    "package", "pairs", "pcall", "print", "rawequal", "rawget", "rawset",
    "rawlen", "require", "select", "setfenv", "setmetatable", "table",
    "tonumber", "tostring", "type", "unpack", "wl", "xpcall", "string",
    "_", "set_textdomain", "get_build_id", "ngettext", "_ENV",
    "game", "this", "socket", "step", "world", "cam", "math", "_G",
    "init", "include", "world.___persist_entity", "bit32", 0
};

static void
invalidate_entity(lua_State *L, const int index, void *userdata)
{
    void *p = luaL_testudata(L, index, "EntityMT");

    if (p) {
        entity *e = *(static_cast<entity**>(p));

        if (e == userdata) {
            // we found the bugger, invalidate this mother
            lua_pushvalue(L, index-1); // S: ... tbl, key, val(index), key
            lua_pushnil(L); // S: ... tbl, key, val(index), key, nil
            lua_settable(L, index-2); // S: ... tbl, key, val(index)
        }
    }
}

static void
subscribe_to_entity(lua_State *L, const int index, void *userdata)
{
    void *p = luaL_testudata(L, index, "EntityMT");

    if (p) {
        escript *es = static_cast<escript*>(userdata);
        entity *e = *(static_cast<entity**>(p));
        std::set<entity*>::iterator it = es->subscriptions.find(e);
        if (it == es->subscriptions.end()) {
            tms_debugf("Luascript with id %u subscribing to entity with id %u %s",
                    es->id,
                    e->id,
                    e->get_name());
            es->subscribe(e, ENTITY_EVENT_REMOVE, &on_entity_remove, e);
        }
    }
}

static void
plua_foreach(lua_State *L, const int index, void (*cb)(lua_State*, int, void*), void *userdata, bool can_ignore/*=true*/)
{
    if (index >= LUA_FOREACH_MAX_DEPTH) {
        // prevent stack overflows from occuring due to nested tables
        return;
    }

    // S: index = tbl
    lua_pushnil(L); // S: index = tbl, index+1 = key

    while (lua_next(L, index) != 0) {
        // S: index = tbl, index + 1 = key, index + 2 = value
        int key_type = lua_type(L, index + 1);
        bool skip = false;
        if (can_ignore) {
            if (key_type == LUA_TSTRING) {
                const char *key_name = lua_tostring(L, index + 1);

                for (int x=0; ignore[x]; ++x) {
                    if (strcmp(key_name, ignore[x]) == 0) {
                        skip = true;
                        break;
                    }
                }
            }
        }

        if (!skip) {
            if (lua_istable(L, index + 2)) {
                plua_foreach(L, index + 2, cb, userdata, false);
            } else {
                cb(L, index + 2, userdata);
            }
        }

        lua_pop(L, 1); // S: index = tbl, index + 1 = key
    }
}

static struct vert base[4] = {
    {
        (tvec3){.5f,.5f,0.f},
        (tvec2){.5f, 1.f},
        (tvec4){0.f, 0.f, 0.f, 0.f},
    }, {
        (tvec3){-.5f,.5f,0.f},
        (tvec2){0.f, 1.f},
        (tvec4){0.f, 0.f, 0.f, 0.f},
    }, {
        (tvec3){-.5f,-.5f,0.f},
        (tvec2){0.f, .5f},
        (tvec4){0.f, 0.f, 0.f, 0.f},
    }, {
        (tvec3){.5f,-.5f,0.f},
        (tvec2){.5f, .5f},
        (tvec4){0.f, 0.f, 0.f, 0.f},
    }
};

static int
my_writer(lua_State *L, const void *contents, size_t size, void *ud)
{
    struct DataStruct *d = (struct DataStruct *)ud;

    d->buf = (char*)realloc(d->buf, d->size + size + 1);
    if (d->buf == NULL) {
        tms_fatalf("Ran out of memory while writing Lua state");
        return 0;
    }

    memcpy(&(d->buf[d->size]), contents, size);
    d->size += size;
    d->buf[d->size] = 0;

    return 0;
}

static const char*
my_reader(lua_State *L, void *ud, size_t *size)
{
    struct DataStruct *d = (struct DataStruct*)ud;
    *size = d->size;

    return d->buf;
}

static void
persist_all(lua_State *L, escript *e)
{
    assert(lua_gettop(L) == 0); // S:

    //lua_gc(L, LUA_GCCOLLECT, 0);

    lua_newtable(L); // S: perms

    lua_newtable(L); // S: perms root

    lua_rawgeti(L, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS); // S: perms root global

    lua_pushnil(L); // S: perms root global k
    while (lua_next(L, 3) != 0) { // S: perms root global k v
        lua_pushvalue(L, -2); // S: perms root global k v k
        const char *key = lua_tostring(L, -1);
        lua_pop(L, 1); // S: perms root global k v

        bool skip = false;
        for (int x=0; ignore[x]; ++x) {
            if (strcmp(key, ignore[x]) == 0) {
                skip = true;
                break;
            }
        }

        if (!skip) {
            //tms_debugf("Letting through %s", key);
            lua_pushvalue(L, -2); // S: perms root global k v k
            lua_insert(L, -2); // S: perms root global k k v
            lua_settable(L, 2); // S: perms root global k
        } else {
            lua_pop(L, 1); // S: perms root global k
        }
    } // S: perms root global
    lua_pop(L, 1); // S: perms root

    uint32_t i = 1;

    // Now the iterators functions.
    m_add_iterator_function_to_not_persist(L, "pairs", i++);
    m_add_iterator_function_to_not_persist(L, "ipairs", i++);

    // And finally the globals.
    for (int j = 0; ignore[j]; ++j) {
        m_add_object_to_not_persist(L, ignore[j], i++);
    }

    if (e->data.size != 0) {
        free(e->data.buf);
        e->data.buf = (char*)malloc(1);
        e->data.size = 0;
    }

    eris_dump(L, my_writer, &e->data);

    tms_debugf("Dumped len: %d", (int)e->data.size);

    lua_pop(L, 2); // S:
}

static void
unpersist_all(lua_State *L, escript *e)
{
    assert(lua_gettop(L) == 0); // S:

    lua_newtable(L); // S: perms

    uint32_t i = 1;

    m_add_iterator_function_to_not_unpersist(L, "pairs", i++);
    m_add_iterator_function_to_not_unpersist(L, "ipairs", i++);

    for (int j = 0; ignore[j]; ++j) {
        m_add_object_to_not_unpersist(L, ignore[j], i++);
    }

    eris_undump(L, my_reader, &e->data); // S: perms table

    lua_dump_stack(L);

    lua_remove(L, -2); // S: table

    lua_pushnil(L);  // S: table nil
    while (lua_next(L, 1) != 0) {
        // S: table key value
        lua_rawgeti(L, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS);  // S: table key value globals_table
        lua_pushvalue(L, -3); // S: table key value globals_table key
        lua_gettable(L, -2);  // S: table key value globals_table value_in_globals
        if (lua_compare(L, -1, -3, LUA_OPEQ)) {
            lua_pop(L, 3); // S: table key
            continue;
        } else {
            // Make this a global value
            lua_pop(L, 1);  // S: table key value globals_table
            lua_pushvalue(L, -3);  // S: table key value globals_table key
            lua_pushvalue(L, -3);  // S: table key value globals_table key value
            lua_settable(L, -3);  // S: table key value globals_table
            lua_pop(L, 2);  // S: table key
        }
    } // S: table

    lua_pop(L, 1); // S: table
}

static void register_world(lua_State *L);
static void register_game(lua_State *L);
static void register_cam(lua_State *L);
static void register_entity(lua_State *L);
static void register_this(lua_State *L, escript *e);

static uint32_t
upper_power_of_two(uint32_t v)
{
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;
}

/*
 * Recursive function to push stuff!
 * Can currently handle:
 * Numbers, booleans, strings, nil, tables
 */
static bool
lua_push_stuff(lua_State *Lsrc, lua_State *Ldst, int pos)
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

extern "C" {
    // For more detailed documentation about the Lua API's functions,
    // see the Principia wiki: https://principia-web.se/wiki/LuaScript

    /* WORLD */

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
    static int l_world_unpersist_entity(lua_State *L)
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


    /* CAM */

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


    /* ENTITY */

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


    /* THIS */

    /* this:write(socket, value) */
    static int l_this_write(lua_State *L)
    {
        escript *e = *(static_cast<escript**>(luaL_checkudata(L, 1, "This")));
        int socket = luaL_checkint(L, 2);
        double value = tclampf(luaL_checknumber(L, 3), 0.0, 1.0);
        if (socket < 0) socket = 0;
        if (socket > 3) socket = 3;

        if (!e->s_out[socket].written()) {
            e->s_out[socket].write(value);
        }

        return 0;
    }

    /* this:read(socket) */
    static int l_this_read(lua_State *L)
    {
        escript *e = *(static_cast<escript**>(luaL_checkudata(L, 1, "This")));
        int socket = luaL_checkint(L, 2);
        if (socket < 0) socket = 0;
        if (socket > 3) socket = 3;

        lua_pushnumber(L, e->val[socket]);

        return 1;
    }

    /* this:has_plug(socket) */
    static int l_this_has_plug(lua_State *L)
    {
        ESCRIPT_VERSION_ERROR(L, "this:has_plug", "1.5", LEVEL_VERSION_1_5);

        escript *e = *(static_cast<escript**>(luaL_checkudata(L, 1, "This")));
        int socket = luaL_checkint(L, 2);
        if (socket < 0) socket = 0;
        if (socket > 3) socket = 3;

        lua_pushboolean(L, e->socket_active[socket]);

        return 1;
    }

    /* this:write_frequency(frequency, value) */
    static int l_this_write_frequency(lua_State *L)
    {
        ESCRIPT_VERSION_ERROR(L, "this:write_frequency", "1.3.0.2", LEVEL_VERSION_1_3_0_2);

        escript *e = *(static_cast<escript**>(luaL_checkudata(L, 1, "This")));
        uint32_t freq = (uint32_t)luaL_checklong(L, 2);
        double value = tclampf(luaL_checknumber(L, 3), 0.0, 1.0);

        std::pair<std::multimap<uint32_t, receiver_base*>::iterator, std::multimap<uint32_t, receiver_base*>::iterator> range = W->receivers.equal_range(freq);
        for (std::multimap<uint32_t, receiver_base*>::iterator
                i = range.first;
                i != range.second && i != W->receivers.end();
                i++) {
            i->second->pending_value = value;
            i->second->no_broadcast = true;
        }

        return 0;
    }

    /* this:listen_on_frequency(frequency) */
    static int l_this_listen_on_frequency(lua_State *L)
    {
        ESCRIPT_VERSION_ERROR(L, "this:listen_on_frequency", "1.4", LEVEL_VERSION_1_4);

        escript *e = *(static_cast<escript**>(luaL_checkudata(L, 1, "This")));
        uint32_t freq = (uint32_t)luaL_checklong(L, 2);
        if (e->first_run) {
            receiver_base *rb = new receiver_base();
            std::pair<std::map<uint32_t, receiver_base*>::iterator, bool> ret;
            ret = e->receivers.insert(std::pair<uint32_t, receiver_base*>(freq, rb));
            if (!ret.second) {
                delete rb;
            } else {
                W->add_receiver(freq, rb);
            }
        } else {
            lua_pushstring(L, "You can only start listening to frequency in init().");
            lua_error(L);
        }

        return 0;
    }

    /* this:read_frequency(frequency) */
    static int l_this_read_frequency(lua_State *L)
    {
        escript *e = *(static_cast<escript**>(luaL_checkudata(L, 1, "This")));
        uint32_t freq = luaL_checklong(L, 2);

        std::map<uint32_t, receiver_base*>::iterator ret;
        ret = e->receivers.find(freq);
        float v = 0.f;
        if (ret != e->receivers.end()) {
            v = ret->second->pending_value;
        } else {
            char err[512];
            snprintf(err, 511, "You are not listening to frequency %u.\nUse this:listen_on_frequency() to begin.", freq);
            lua_pushstring(L, err);
            lua_error(L);
        }

        lua_pushnumber(L, v);

        return 1;
    }

    /* this:first_run() */
    static int l_this_first_run(lua_State *L)
    {
        ESCRIPT_VERSION_ERROR(L, "this:first_run", "1.3.0.2", LEVEL_VERSION_1_3_0_2);
        ESCRIPT_FUNCTION_DEPRECATED(L, "this:first_run", "1.5", LEVEL_VERSION_1_5);

        escript *e = *(static_cast<escript**>(luaL_checkudata(L, 1, "This")));
        lua_pushboolean(L, e->first_run);

        return 1;
    }

    /* x, y = this:get_position() */
    static int l_this_get_position(lua_State *L)
    {
        ESCRIPT_VERSION_ERROR(L, "this:get_position", "1.3.0.2", LEVEL_VERSION_1_3_0_2);

        escript *e = *(static_cast<escript**>(luaL_checkudata(L, 1, "This")));
        b2Vec2 pos = e->get_position();
        lua_pushnumber(L, pos.x);
        lua_pushnumber(L, pos.y);

        return 2;
    }

    /* id = this:get_id() */
    static int l_this_get_id(lua_State *L)
    {
        ESCRIPT_VERSION_ERROR(L, "this:get_id", "1.5", LEVEL_VERSION_1_5);

        escript *e = *(static_cast<escript**>(luaL_checkudata(L, 1, "This")));
        lua_pushnumber(L, e->id);
        return 1;
    }

    /* width, height = this:get_resolution()*/
    static int l_this_get_resolution(lua_State *L)
    {
        ESCRIPT_VERSION_ERROR(L, "this:get_resolution", "1.5", LEVEL_VERSION_1_5);

        lua_pushnumber(L, _tms.window_width);
        lua_pushnumber(L, _tms.window_height);

        return 2;
    }

    /* ratio = this:get_ratio()*/
    static int l_this_get_ratio(lua_State *L)
    {
        ESCRIPT_VERSION_ERROR(L, "this:get_ratio", "1.5", LEVEL_VERSION_1_5);

        lua_pushnumber(L, (float)_tms.window_width/_tms.window_height);

        return 1;
    }

    /* this:set_sprite_blending(int) */
    static int l_this_set_sprite_blending(lua_State *L)
    {
        ESCRIPT_VERSION_ERROR(L, "this:set_sprite_blending", "1.3.0.2", LEVEL_VERSION_1_3_0_2);

        escript *e = *(static_cast<escript**>(luaL_checkudata(L, 1, "This")));
        e->blending_mode = luaL_checkint(L, 2);

        if (e->blending_mode < 0 || e->blending_mode > 2) {
            lua_pushstring(L, "Invalid blending mode");
            lua_error(L);
        }

        return 0;
    }

    /* this:set_sprite_filtering(int) */
    static int l_this_set_sprite_filtering(lua_State *L)
    {
        ESCRIPT_VERSION_ERROR(L, "this:set_sprite_filtering", "1.3.0.2", LEVEL_VERSION_1_3_0_2);

        escript *e = *(static_cast<escript**>(luaL_checkudata(L, 1, "This")));
        e->filtering = luaL_checkint(L, 2);

        if (e->filtering < 0 || e->filtering > 1) {
            lua_pushstring(L, "Invalid sprite filtering");
            lua_error(L);
        }

        return 0;
    }

    /* this:set_sprite_texel(x, y, r, g, b, a) */
    static int l_this_set_sprite_texel(lua_State *L)
    {
        ESCRIPT_VERSION_ERROR(L, "this:set_sprite_texel", "1.3.0.2", LEVEL_VERSION_1_3_0_2);

        escript *e = *(static_cast<escript**>(luaL_checkudata(L, 1, "This")));

        int u = luaL_checkint(L, 2);
        int v = luaL_checkint(L, 3);
        float r = luaL_checknumber(L, 4);
        float g = luaL_checknumber(L, 5);
        float b = luaL_checknumber(L, 6);
        float a = luaL_checknumber(L, 7);

        if (!e->normal_draw) {
            e->normal_draw = new draw_data(e);
        }

        draw_data *draw = e->normal_draw;

        if (u < 0 || u >= draw->texture_width || v < 0 || v >= draw->texture_height) {
            lua_pushfstring(L, "texel coordinate out of range (%d/%d)", u, v);
            lua_error(L);
        }

        draw->texture->data[draw->texture_width*4*v + 4*u] = (unsigned char)tclampf(roundf(r*255.f), 0, 255.f);
        draw->texture->data[draw->texture_width*4*v + 4*u+1] = (unsigned char)tclampf(roundf(g*255.f), 0, 255.f);
        draw->texture->data[draw->texture_width*4*v + 4*u+2] = (unsigned char)tclampf(roundf(b*255.f), 0.f, 255.f);
        draw->texture->data[draw->texture_width*4*v + 4*u+3] = (unsigned char)tclampf(roundf(a*255.f), 0.f, 255.f);

        draw->texture_modified = true;

        return 0;
    }

    /* this:clear_texels() */
    static int l_this_clear_texels(lua_State *L)
    {
        ESCRIPT_VERSION_ERROR(L, "this:clear_texels", "1.4", LEVEL_VERSION_1_4);

        escript *e = *(static_cast<escript**>(luaL_checkudata(L, 1, "This")));

        unsigned char clear_value = 0;

        if (lua_gettop(L) > 1) {
            float clr = luaL_checknumber(L, 2);
            clear_value = (unsigned char)tclampf(roundf(clr*255.f), 0, 255.f);
        }

        if (!e->normal_draw) {
            e->normal_draw = new draw_data(e);
        }

        draw_data *draw = e->normal_draw;

        tms_texture_clear_buffer(draw->texture, clear_value);

        draw->texture_modified = true;

        return 0;
    }

    /* this:set_draw_tint(r,g,b,a) */
    static int l_this_set_draw_tint(lua_State *L)
    {
        ESCRIPT_VERSION_ERROR(L, "this:set_draw_tint", "1.3.0.2", LEVEL_VERSION_1_3_0_2);

        escript *e = *(static_cast<escript**>(luaL_checkudata(L, 1, "This")));
        e->draw_tint.r = luaL_checknumber(L, 2);
        e->draw_tint.g = luaL_checknumber(L, 3);
        e->draw_tint.b = luaL_checknumber(L, 4);
        e->draw_tint.a = luaL_checknumber(L, 5);

        return 0;
    }

    /* this:set_draw_z(z) */
    static int l_this_set_draw_z(lua_State *L)
    {
        ESCRIPT_VERSION_ERROR(L, "this:set_draw_z", "1.3.0.2", LEVEL_VERSION_1_3_0_2);

        escript *e = *(static_cast<escript**>(luaL_checkudata(L, 1, "This")));
        e->draw_z = luaL_checknumber(L, 2);

        return 0;
    }

    /* this:set_draw_coordinates(int) */
    static int l_this_set_draw_coordinates(lua_State *L)
    {
        ESCRIPT_VERSION_ERROR(L, "this:set_draw_coordinates", "1.5", LEVEL_VERSION_1_5);

        escript *e = *(static_cast<escript**>(luaL_checkudata(L, 1, "This")));
        int mode = luaL_checkint(L, 2);

        if (mode < 0 || mode > 2) {
            lua_pushstring(L, "Invalid camera mode.");
            lua_error(L);
        } else {
            e->coordinate_mode = mode;

            if (e->coordinate_mode == ESCRIPT_LOCAL && lua_gettop(L) == 3) {
                e->local_id = luaL_checknumber(L, 3);
            } else {
                e->local_id = 0;
            }
        }

        return 0;
    }

    /* this:draw_sprite(x, y, r, w, h, bx, by, tx, ty) */
    static int l_this_draw_sprite(lua_State *L)
    {
        ESCRIPT_VERSION_ERROR(L, "this:draw_sprite", "1.3.0.2", LEVEL_VERSION_1_3_0_2);

        escript *e = *(static_cast<escript**>(luaL_checkudata(L, 1, "This")));

        if (!e->normal_draw) {
            e->normal_draw = new draw_data(e);
        }

        draw_data *draw = e->normal_draw;

        float x = luaL_checknumber(L, 2);
        float y = luaL_checknumber(L, 3);
        float r = luaL_checknumber(L, 4);
        float w = luaL_checknumber(L, 5);
        float h = luaL_checknumber(L, 6);
        float bx = (float)luaL_checkint(L, 7);
        float by = (float)luaL_checkint(L, 8);
        float tx = (float)luaL_checkint(L, 9);
        float ty = (float)luaL_checkint(L, 10);

        if (tx == bx) tx = bx+.5f;
        if (ty == by) ty = by+.5f;

        tvec2 uvb = {(float)bx / draw->texture_width, (float)by / draw->texture_height};
        tvec2 uvt = {(float)tx / draw->texture_width, (float)ty / draw->texture_height};

        if (draw->sprite_count < MAX_SPRITES) {
            struct vert *_b = (struct vert*)draw->verts->get_buffer();
            int n = draw->sprite_count;

            entity *local_entity = 0;
            b2Vec2 local_position;

            if (e->coordinate_mode == ESCRIPT_LOCAL) {
                local_entity = (e->local_id != 0 ? W->get_entity_by_id(e->local_id) : 0);

                if (local_entity) {
                    local_position = local_entity->get_position();
                    r += local_entity->get_angle();
                } else {
                    local_position = e->get_position();
                    r += e->get_angle();
                }
            }

            if (r != 0.f) {
                float cs, sn;
                tmath_sincos(r, &sn, &cs);

                for (int ix=0; ix<4; ix++) {
                    _b[n*4+ix] = base[ix];

                    _b[n*4+ix].pos.x *= w;
                    _b[n*4+ix].pos.y *= h;

                    float _x = _b[n*4+ix].pos.x * cs - _b[n*4+ix].pos.y * sn;
                    float _y = _b[n*4+ix].pos.x * sn + _b[n*4+ix].pos.y * cs;
                    _b[n*4+ix].pos.x = _x;
                    _b[n*4+ix].pos.y = _y;

                    if (e->coordinate_mode == ESCRIPT_LOCAL) {
                        _b[n*4+ix].pos.x += local_position.x;
                        _b[n*4+ix].pos.y += local_position.y;
                    }

                    _b[n*4+ix].pos.x += x;
                    _b[n*4+ix].pos.y += y;
                    _b[n*4+ix].pos.z += e->draw_z;

                    _b[n*4+ix].color.r = e->draw_tint.r;
                    _b[n*4+ix].color.g = e->draw_tint.g;
                    _b[n*4+ix].color.b = e->draw_tint.b;
                    _b[n*4+ix].color.a = e->draw_tint.a;

                    _b[n*4+ix].uv.x = ((ix == 0 || ix == 3) ? uvt.x : uvb.x);
                    _b[n*4+ix].uv.y = (ix < 2 ? uvt.y : uvb.y);
                }
            } else {
                for (int ix=0; ix<4; ix++) {
                    _b[n*4+ix] = base[ix];

                    _b[n*4+ix].pos.x *= w;
                    _b[n*4+ix].pos.y *= h;

                    if (e->coordinate_mode == ESCRIPT_LOCAL) {
                        _b[n*4+ix].pos.x += local_position.x;
                        _b[n*4+ix].pos.y += local_position.y;
                    }

                    _b[n*4+ix].pos.x += x;
                    _b[n*4+ix].pos.y += y;

                    _b[n*4+ix].pos.z += e->draw_z;

                    _b[n*4+ix].color.r = e->draw_tint.r;
                    _b[n*4+ix].color.g = e->draw_tint.g;
                    _b[n*4+ix].color.b = e->draw_tint.b;
                    _b[n*4+ix].color.a = e->draw_tint.a;

                    _b[n*4+ix].uv.x = ((ix == 0 || ix == 3) ? uvt.x : uvb.x);
                    _b[n*4+ix].uv.y = (ix < 2 ? uvt.y : uvb.y);
                }
            }

            if (e->solving) {
                draw->sprite_count ++;
            } else {
                draw->pending_sprite_count ++;
            }
        }

        return 0;
    }

    /* this:draw_line(x1, y1, x2, y2, w) */
    static int l_this_draw_line(lua_State *L)
    {
        ESCRIPT_VERSION_ERROR(L, "this:draw_line", "1.4", LEVEL_VERSION_1_4);

        escript_line line;

        escript *e = *(static_cast<escript**>(luaL_checkudata(L, 1, "This")));
        line.x1 = luaL_checknumber(L, 2);
        line.y1 = luaL_checknumber(L, 3);
        line.x2 = luaL_checknumber(L, 4);
        line.y2 = luaL_checknumber(L, 5);
        line.w1 = luaL_checknumber(L, 6);
        line.w2 = line.w1;
        line.z1 = e->draw_z;
        line.z2 = e->draw_z;
        line.r1 = e->draw_tint.r;
        line.g1 = e->draw_tint.g;
        line.b1 = e->draw_tint.b;
        line.a1 = e->draw_tint.a;
        line.r2 = e->draw_tint.r;
        line.g2 = e->draw_tint.g;
        line.b2 = e->draw_tint.b;
        line.a2 = e->draw_tint.a;

        e->add_line(line);

        return 0;
    }

    /* this:draw_gradient_line(x1, y1, x2, y2, w, r, g, b, a) */
    static int l_this_draw_gradient_line(lua_State *L)
    {
        ESCRIPT_VERSION_ERROR(L, "this:draw_gradient_line", "1.5", LEVEL_VERSION_1_5);

        escript_line line;

        escript *e = *(static_cast<escript**>(luaL_checkudata(L, 1, "This")));
        line.x1 = luaL_checknumber(L, 2);
        line.y1 = luaL_checknumber(L, 3);
        line.x2 = luaL_checknumber(L, 4);
        line.y2 = luaL_checknumber(L, 5);
        line.w1 = luaL_checknumber(L, 6);
        line.w2 = line.w1;
        line.z1 = e->draw_z;
        line.z2 = e->draw_z;
        line.r1 = e->draw_tint.r;
        line.g1 = e->draw_tint.g;
        line.b1 = e->draw_tint.b;
        line.a1 = e->draw_tint.a;
        line.r2 = luaL_checknumber(L, 7);
        line.g2 = luaL_checknumber(L, 8);
        line.b2 = luaL_checknumber(L, 9);
        line.a2 = luaL_checknumber(L, 10);

        e->add_line(line);

        return 0;
    }

    /* this:draw_line_3d(x1, y1, z1, x2, y2, z2, w) */
    static int l_this_draw_line_3d(lua_State *L)
    {
        ESCRIPT_VERSION_ERROR(L, "this:draw_line_3d", "1.5", LEVEL_VERSION_1_5);

        escript_line line;

        escript *e = *(static_cast<escript**>(luaL_checkudata(L, 1, "This")));
        line.x1 = luaL_checknumber(L, 2);
        line.y1 = luaL_checknumber(L, 3);
        line.z1 = luaL_checknumber(L, 4);
        line.x2 = luaL_checknumber(L, 5);
        line.y2 = luaL_checknumber(L, 6);
        line.z2 = luaL_checknumber(L, 7);
        line.w1 = luaL_checknumber(L, 8);
        line.w2 = line.w1;
        line.r1 = e->draw_tint.r;
        line.g1 = e->draw_tint.g;
        line.b1 = e->draw_tint.b;
        line.a1 = e->draw_tint.a;
        line.r2 = e->draw_tint.r;
        line.g2 = e->draw_tint.g;
        line.b2 = e->draw_tint.b;
        line.a2 = e->draw_tint.a;

        e->add_line(line);

        return 0;
    }

    /* this:draw_gradient_line_3d(x1, y1, z1, x2, y2, z2, w, r, g, b, a) */
    static int l_this_draw_gradient_line_3d(lua_State *L)
    {
        ESCRIPT_VERSION_ERROR(L, "this:draw_gradient_line_3d", "1.5", LEVEL_VERSION_1_5);

        escript_line line;

        escript *e = *(static_cast<escript**>(luaL_checkudata(L, 1, "This")));
        line.x1 = luaL_checknumber(L, 2);
        line.y1 = luaL_checknumber(L, 3);
        line.z1 = luaL_checknumber(L, 4);
        line.x2 = luaL_checknumber(L, 5);
        line.y2 = luaL_checknumber(L, 6);
        line.z2 = luaL_checknumber(L, 7);
        line.w1 = luaL_checknumber(L, 8);
        line.w2 = line.w1;
        line.r1 = e->draw_tint.r;
        line.g1 = e->draw_tint.g;
        line.b1 = e->draw_tint.b;
        line.a1 = e->draw_tint.a;
        line.r2 = luaL_checknumber(L, 9);
        line.g2 = luaL_checknumber(L, 10);
        line.b2 = luaL_checknumber(L, 11);
        line.a2 = luaL_checknumber(L, 12);

        e->add_line(line);

        return 0;
    }

    /* r, g, b, a = this:get_sprite_texel(x, y) */
    static int l_this_get_sprite_texel(lua_State *L)
    {
        ESCRIPT_VERSION_ERROR(L, "this:get_sprite_texel", "1.5", LEVEL_VERSION_1_5);

        escript *e = *(static_cast<escript**>(luaL_checkudata(L, 1, "This")));

        int u = luaL_checkint(L, 2);
        int v = luaL_checkint(L, 3);

        if (!e->normal_draw) {
            lua_pushnumber(L, 0.f);
            lua_pushnumber(L, 0.f);
            lua_pushnumber(L, 0.f);
            lua_pushnumber(L, 0.f);
            return 4;
        }

        if (u < 0 || u >= e->normal_draw->texture_width || v < 0 || v >= e->normal_draw->texture_height) {
            lua_pushstring(L, "texel coordinate out of range");
            lua_error(L);
        }

        struct tms_texture *tex = e->normal_draw->texture;
        int width = e->normal_draw->texture_width;

        lua_pushnumber(L, tex->data[width*4*v + 4*u+0]/255.f); // r
        lua_pushnumber(L, tex->data[width*4*v + 4*u+1]/255.f); // g
        lua_pushnumber(L, tex->data[width*4*v + 4*u+2]/255.f); // b
        lua_pushnumber(L, tex->data[width*4*v + 4*u+3]/255.f); // a

        return 4;
    }

    /* this:init_draw(width, height) */
    static int l_this_init_draw(lua_State *L)
    {
        ESCRIPT_VERSION_ERROR(L, "this:init_draw", "1.5", LEVEL_VERSION_1_5);

        escript *e = *(static_cast<escript**>(luaL_checkudata(L, 1, "This")));

        if (!e->first_run) {
            lua_pushstring(L, "Draw can only be initialized in the init()-function.");
            lua_error(L);
        }

        int width = luaL_checkint(L, 2);
        int height = luaL_checkint(L, 3);

        width = upper_power_of_two(width);
        height = upper_power_of_two(height);

        if (width < 1 || width > 1024 || height < 1 || height > 1024) {
            lua_pushstring(L, "Draw width/height out of range. Must be between 1 and 1024.");
            lua_error(L);
        }

        if (!e->normal_draw) {
            e->normal_draw = new draw_data(e, width, height);
        }

        return 0;
    }

    /* this:set_static_sprite_texel(x, y, r, g, b, a)*/
    static int l_this_set_static_sprite_texel(lua_State *L)
    {
        ESCRIPT_VERSION_ERROR(L, "this:set_static_sprite_texel", "1.5", LEVEL_VERSION_1_5);

        escript *e = *(static_cast<escript**>(luaL_checkudata(L, 1, "This")));

        int u = luaL_checkint(L, 2);
        int v = luaL_checkint(L, 3);
        float r = luaL_checknumber(L, 4);
        float g = luaL_checknumber(L, 5);
        float b = luaL_checknumber(L, 6);
        float a = luaL_checknumber(L, 7);

        if (!e->static_draw) {
            e->static_draw = new draw_data(e);
        }

        draw_data *draw = e->static_draw;

        if (u < 0 || u >= draw->texture_width || v < 0 || v >= draw->texture_height) {
            lua_pushfstring(L, "texel coordinate out of range (%d/%d)", u, v);
            lua_error(L);
        }

        draw->texture->data[draw->texture_width*4*v + 4*u] = (unsigned char)tclampf(roundf(r*255.f), 0, 255.f);
        draw->texture->data[draw->texture_width*4*v + 4*u+1] = (unsigned char)tclampf(roundf(g*255.f), 0, 255.f);
        draw->texture->data[draw->texture_width*4*v + 4*u+2] = (unsigned char)tclampf(roundf(b*255.f), 0.f, 255.f);
        draw->texture->data[draw->texture_width*4*v + 4*u+3] = (unsigned char)tclampf(roundf(a*255.f), 0.f, 255.f);

        draw->texture_modified = true;

        return 0;
    }

    /* this:clear_static_texels() */
    static int l_this_clear_static_texels(lua_State *L)
    {
        ESCRIPT_VERSION_ERROR(L, "this:clear_static_texels", "1.5", LEVEL_VERSION_1_5);

        escript *e = *(static_cast<escript**>(luaL_checkudata(L, 1, "This")));

        if (!e->static_draw) {
            e->static_draw = new draw_data(e);
        }

        draw_data *draw = e->static_draw;

        int numargs = lua_gettop(L);

        if (numargs > 2) {
            unsigned char colors[4] = {
                127, 127, 127, 127
            };

            float v = luaL_checknumber(L, 2);
            colors[0] = (unsigned char)tclampf(roundf(v*255.f), 0, 255.f);

            if (numargs >= 3) {
                float v = luaL_checknumber(L, 3);
                colors[1] = (unsigned char)tclampf(roundf(v*255.f), 0, 255.f);
            }
            if (numargs >= 4) {
                float v = luaL_checknumber(L, 4);
                colors[2] = (unsigned char)tclampf(roundf(v*255.f), 0, 255.f);
            }
            if (numargs >= 5) {
                float v = luaL_checknumber(L, 5);
                colors[3] = (unsigned char)tclampf(roundf(v*255.f), 0, 255.f);
            }

            uint32_t buf_sz = draw->texture_width * draw->texture_height * draw->texture_num_channels;
            for (uint32_t i=0; i<buf_sz; i += draw->texture_num_channels) {
                for (uint8_t c=0; c<draw->texture_num_channels; ++c) {
                    draw->texture->data[i+c] = colors[c];
                }
            }
        } else {
            unsigned char clear_value = 0;

            if (numargs == 2) {
                float clr = luaL_checknumber(L, 2);
                clear_value = (unsigned char)tclampf(roundf(clr*255.f), 0, 255.f);
            }

            tms_texture_clear_buffer(draw->texture, clear_value);
        }

        draw->texture_modified = true;

        return 0;
    }

    /* this:add_static_sprite(x, y, r, w, h, bx, by, tx, ty) */
    static int l_this_add_static_sprite(lua_State *L)
    {
        ESCRIPT_VERSION_ERROR(L, "this:add_static_sprite", "1.5", LEVEL_VERSION_1_5);

        escript *e = *(static_cast<escript**>(luaL_checkudata(L, 1, "This")));

        if (!e->static_draw) {
            e->static_draw = new draw_data(e);
        }

        float x = luaL_checknumber(L, 2);
        float y = luaL_checknumber(L, 3);
        float r = luaL_checknumber(L, 4);
        float w = luaL_checknumber(L, 5);
        float h = luaL_checknumber(L, 6);
        float bx = (float)luaL_checkint(L, 7);
        float by = (float)luaL_checkint(L, 8);
        float tx = (float)luaL_checkint(L, 9);
        float ty = (float)luaL_checkint(L, 10);

        if (tx == bx) tx = bx+.5f;
        if (ty == by) ty = by+.5f;

        e->add_static_sprite(x, y, r, w, h, bx, by, tx, ty);

        return 0;
    }

    /* this:clear_static_sprites() */
    static int l_this_clear_static_sprites(lua_State *L)
    {
        ESCRIPT_VERSION_ERROR(L, "this:clear_static_sprites", "1.5", LEVEL_VERSION_1_5);

        escript *e = *(static_cast<escript**>(luaL_checkudata(L, 1, "This")));

        if (!e->static_draw) {
            e->static_draw = new draw_data(e);
        }

        e->static_draw->sprite_count = 0;
        e->static_sprites.clear();

        return 0;
    }
}

escript::escript()
    : prompt_id(0)
    , solving(false)
{
    this->set_flag(ENTITY_HAS_CONFIG,        true);
    this->set_flag(ENTITY_HAS_TRACKER,       true);
    this->set_flag(ENTITY_DO_UPDATE_EFFECTS, true);
    this->set_flag(ENTITY_DISABLE_UNLOADING, true);
    this->set_flag(ENTITY_IS_PROMPT,         true);

    this->data.buf = (char*)malloc(1);
    this->data.size = 0;

    this->normal_draw = 0;
    this->static_draw = 0;
    this->L = 0;

    this->dialog_id = DIALOG_ESCRIPT;

    this->set_material(&m_iomisc);
    this->set_mesh(mesh_factory::get_mesh(MODEL_SCRIPT));

    this->run = false;
    this->has_on_event = false;
    this->has_on_response = false;
    this->listen_on_input = false;

    this->cull_effects_method = CULL_EFFECTS_DISABLE;

    this->num_s_in = 4;
    this->num_s_out = 4;

    this->set_num_properties(2);

    this->s_in[0].lpos = b2Vec2(-.395f, -.125f);
    this->s_in[1].lpos = b2Vec2(-.145f, -.125f);
    this->s_in[2].lpos = b2Vec2( .145f, -.125f);
    this->s_in[3].lpos = b2Vec2( .395f, -.125f);
    this->s_out[0].lpos = b2Vec2(-.395f, .125f);
    this->s_out[1].lpos = b2Vec2(-.145f, .125f);
    this->s_out[2].lpos = b2Vec2( .145f, .125f);
    this->s_out[3].lpos = b2Vec2( .395f, .125f);

    this->properties[0].type = P_STR;
    if (W->level.version >= LEVEL_VERSION_1_5) {
        this->set_property(0, default_code);
    } else {
        this->set_property(0, default_code_old);
    }

    this->properties[1].type = P_INT;
    this->properties[1].v.i = 0;

    this->set_as_rect(.65f, .325f);

    this->p_message = 0;
    this->p_btn1 = 0;
    this->p_btn1_len = 0;
    this->p_btn2 = 0;
    this->p_btn2_len = 0;
    this->p_btn3 = 0;
    this->p_btn3_len = 0;

    this->message = &this->p_message;

    this->buttons[0].buf = &this->p_btn1;
    this->buttons[0].len = &this->p_btn1_len;

    this->buttons[1].buf = &this->p_btn2;
    this->buttons[1].len = &this->p_btn2_len;

    this->buttons[2].buf = &this->p_btn3;
    this->buttons[2].len = &this->p_btn3_len;
}

escript::~escript()
{
    if (this->normal_draw) {
        delete this->normal_draw;
    }

    if (this->static_draw) {
        delete this->static_draw;
    }

    if (this->L) {
        lua_close(this->L);
    }

    std::map<uint32_t, receiver_base*>::iterator i;
    for (i = this->receivers.begin(); i != this->receivers.end(); ++i) {
        delete i->second;
    }

    this->receivers.clear();
}

void
escript::remove_from_world()
{
    entity::remove_from_world();
    std::map<uint32_t, receiver_base*>::iterator i;
    for (i = this->receivers.begin(); i != this->receivers.end(); ++i) {
        W->remove_receiver(i->first, i->second);
        delete i->second;
    }

    this->receivers.clear();
}

void
timelimit_cb(lua_State *L, lua_Debug *d)
{
    const uint32_t cur_time = SDL_GetTicks() - start_tick;

    tms_debugf("Cur time: %u", cur_time);

    if (is_first_run) {
        if (cur_time > FIRST_RUN_TIMELIMIT) {
            lua_pushstring(L, "Script halted! Time limit reached!");
            lua_error(L);
        }
    } else {
        if (cur_time > TIMELIMIT) {
            lua_pushstring(L, "Script halted! Time limit reached!");
            lua_error(L);
        }
    }
}

/* Timelimit callback used in 1.5 and above */
void
timelimit_cb_1_5(lua_State *L, lua_Debug *d)
{
    Uint32 ct = SDL_GetTicks();

    if (ct - start_tick > FULL_SCRIPT_TIMELIMIT) {
        tms_debugf("Time limit reached! ct: %d. start_tick: %d. FULL_SCRIPT_TIMELIMIT: %d. diff: %d", ct, start_tick, FULL_SCRIPT_TIMELIMIT, ct - start_tick);
        lua_pushstring(L, "Script halted! Time limit reached!");
        lua_error(L);
    } else {
        if (ct - func_start_tick > function_info[cur_func_id].timelimit) {
            tms_debugf("Time limit reached! ct: %d. func_start_tick: %d. FULL_SCRIPT_TIMELIMIT: %d. diff: %d",
                    ct, func_start_tick, function_info[cur_func_id].timelimit, ct - start_tick)
                do_call_on_halt = true;

            char msg[512];
            snprintf(msg, 511, "Script halted! Time limit reached on %s.", function_info[cur_func_id].name);

            lua_pushstring(L, msg);
            lua_error(L);
        }
    }
}

void
escript::init()
{
    this->input_events.clear();

    for (int x=0; x<WORLD_EVENT__NUM; x++) {
        this->events[x] = 0;
    }

    this->listen_on_input = true;
    this->lines.clear();

    this->L = luaL_newstate();

    this->L->userdata = (void*)this;
    luaopen_base(this->L);
    lua_pop(this->L, 1);

    luaL_requiref(this->L, "math", luaopen_math, 1);
    lua_pop(this->L, 1);

    luaL_requiref(this->L, "string", luaopen_string, 1);
    lua_pop(this->L, 1);

    luaL_requiref(this->L, "table", luaopen_table, 1);
    lua_pop(this->L, 1);

    luaL_requiref(this->L, "bit32", luaopen_bit32, 1);
    lua_pop(this->L, 1);

    register_world(this->L);

    register_game(this->L);

    register_cam(this->L);

    register_entity(this->L);

    register_this(this->L, this);

#ifdef BUILD_LUASOCKET
    if (W->level.flag_active(LVL_ENABLE_LUASOCKET)) {
        luaopen_socket_core(this->L);
        lua_pop(this->L, 1);
    }
#endif

    //apply blacklist
    for (const char** p = blacklist; *p != NULL; p++) {
        lua_pushnil(L);
        lua_setglobal(L, *p);
    }

    start_tick = SDL_GetTicks();
    func_start_tick = SDL_GetTicks();
    cur_func_id = FUNC_GLOBAL_INIT;

    if (W->level.version >= LEVEL_VERSION_1_5) {
        lua_sethook(this->L, timelimit_cb_1_5, LUA_MASKCOUNT, 20);
    } else {
        lua_sethook(this->L, timelimit_cb, LUA_MASKCOUNT, 20);
    }

    if (W->level.version >= LEVEL_VERSION_1_5) {
        char *code = (char*)malloc(strlen(before_code) + strlen(this->properties[0].v.s.buf) + strlen(after_code) + 1);

        strcpy(code, before_code);
        strcat(code, this->properties[0].v.s.buf);
        strcat(code, after_code);

        this->has_on_event = false;
        this->has_on_response = false;
        this->run = false;

        int r = luaL_loadstring(this->L, code);

        if (r == 0) {
            // everything seemed good. perform a single run of the script to prime all the globals
            if (lua_pcall(this->L, 0, 0, 0) != 0) {
                G->add_error(this, ERROR_SCRIPT_COMPILE, lua_pop_error(this->L, "Error initializing Lua: "));
            } else {
                this->run = true;

                lua_getglobal(this->L, "on_event");
                this->has_on_event = !lua_isnil(this->L, -1);
                lua_pop(this->L, 1);

                lua_getglobal(this->L, "on_response");
                this->has_on_response = !lua_isnil(this->L, -1);
                lua_pop(this->L, 1);

                lua_getglobal(this->L, "step");
                this->has_step = !lua_isnil(this->L, -1);
                lua_pop(this->L, 1);

                lua_getglobal(this->L, "on_input");
                if (lua_isnil(this->L, -1)) {
                    this->listen_on_input = false;
                }
                lua_pop(this->L, 1);
            }
        } else {
            G->add_error(this, ERROR_SCRIPT_COMPILE, lua_pop_error(this->L, "Error loading Lua string: "));
        }
    }
}

void
escript::setup()
{
    this->draw_tint = (tvec4){1.f,1.f,1.f,1.f};
    this->local_id = 0;
    this->blending_mode = 1;
    this->filtering = 1;
    this->coordinate_mode = ESCRIPT_WORLD;
    this->draw_z = .0f;
    this->first_run = 1;
}

void
escript::on_pause()
{
    std::map<uint32_t, receiver_base*>::iterator i;
    for (i = this->receivers.begin(); i != this->receivers.end(); ++i) {
        delete i->second;
    }

    this->receivers.clear();

    if (W->level.version >= LEVEL_VERSION_1_5
     && this->properties[1].v.i & ESCRIPT_USE_EXTERNAL_EDITOR
     && G->state.sandbox) {

        char path[ESCRIPT_EXTERNAL_PATH_LEN];
        this->generate_external_path(path);

        FILE *fh;
        size_t sz = 0;
        size_t result = 0;

        char *data = 0;

        fh = fopen(path, "rb");

        if (fh) {
            /* The file exists! */
            fseek(fh, 0L, SEEK_END);
            sz = ftell(fh);
            rewind(fh);
            if (sz > 0) {
                /* The file contains some content that we can read */
                data = (char*)calloc(sizeof(char), sz + 1);

                result = fread(data, sizeof(char), sz, fh);

                if (sz == result) {
                    /* We read the expected amount of bytes,
                        * so we can write that data into
                        * the luascript code buffer */
                    tms_infof("Writing read data into property 0: '%s'", data);
                    this->set_property(0, data);
                }

                free(data);
            }
            fclose(fh);
            tms_infof("File size: %d", (int)sz);
        } else {
            tms_errorf("External editing enabled, yet no file at %s was readable.", path);
        }
    }
}

/* WORLD */
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
static void
register_world(lua_State *L)
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

/* GAME */
static const luaL_Reg game_meta[] = {
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

static void
register_game(lua_State *L)
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

/* CAM */
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

static void
register_cam(lua_State *L)
{
    int lib_id, meta_id;

    lua_createtable(L, 0, 0);
    lib_id = lua_gettop(L);

    luaL_newmetatable(L, "Cam");
    meta_id = lua_gettop(L);
    luaL_setfuncs(L, game_meta, 0);

    luaL_newlib(L, cam_methods);
    lua_setfield(L, meta_id, "__index");

    luaL_newlib(L, cam_meta);
    lua_setfield(L, meta_id, "__metatable");

    lua_setmetatable(L, lib_id);
    lua_setglobal(L, "cam");
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

static void
register_entity(lua_State *L)
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

/* THIS */
static const luaL_Reg this_meta[] = {
    { NULL, NULL }
};
static const luaL_Reg this_methods[] = {
#define LUA_REG(name) { #name, l_this_##name }
    LUA_REG(write),
    LUA_REG(read),
    LUA_REG(has_plug),
    LUA_REG(write_frequency),
    LUA_REG(listen_on_frequency),
    LUA_REG(read_frequency),
    LUA_REG(first_run),
    LUA_REG(get_position),
    LUA_REG(get_id),
    LUA_REG(get_resolution),
    LUA_REG(get_ratio),

    /* draw stuff */
    LUA_REG(set_sprite_blending),
    LUA_REG(set_sprite_filtering),
    LUA_REG(set_sprite_texel),
    LUA_REG(clear_texels),

    LUA_REG(set_draw_tint),
    {"set_sprite_tint", l_this_set_draw_tint}, // backwards compat

    LUA_REG(set_draw_z),
    {"set_sprite_z", l_this_set_draw_z}, // backwards compat

    LUA_REG(set_draw_coordinates),

    LUA_REG(draw_sprite),
    LUA_REG(draw_line),
    LUA_REG(draw_gradient_line),
    LUA_REG(draw_line_3d),
    LUA_REG(draw_gradient_line_3d),

    LUA_REG(get_sprite_texel),

    LUA_REG(init_draw),

    LUA_REG(set_static_sprite_texel),
    LUA_REG(clear_static_texels),
    LUA_REG(add_static_sprite),
    LUA_REG(clear_static_sprites),

    { NULL, NULL }
#undef LUA_REG
};

static void
register_this(lua_State *L, escript *e)
{
    int lib_id, meta_id;

    lua_createtable(L, 0, 0);
    lib_id = lua_gettop(L);

    luaL_newmetatable(L, "This");
    meta_id = lua_gettop(L);
    luaL_setfuncs(L, this_meta, 0);

    luaL_newlib(L, this_methods);
    lua_setfield(L, meta_id, "__index");

    luaL_newlib(L, this_meta);
    lua_setfield(L, meta_id, "__metatable");

    lua_setmetatable(L, lib_id);

    escript **ee = static_cast<escript**>(lua_newuserdata(L, sizeof(entity*)));
    *(ee) = e;
    luaL_setmetatable(L, "This");
    lua_setglobal(L, "this");

    lua_pop(L, 1);
}

void
escript::update_effects()
{
    if (this->normal_draw) {
        this->normal_draw->update_effects();
    }

    if (this->static_draw) {
        this->static_draw->update_effects();
    }

    for (std::vector<escript_line>::const_iterator it = this->lines.begin();
            it != this->lines.end(); ++it) {
        const struct escript_line &l = *it;

        linebuffer::add(l.x1, l.y1, l.z1, l.x2, l.y2, l.z2, l.r1, l.g1, l.b1, l.a1, l.r2, l.g2, l.b2, l.a2, l.w1, l.w2);
    }
}

edevice*
escript::solve_electronics()
{
    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();
    if (!this->s_in[1].is_ready())
        return this->s_in[1].get_connected_edevice();
    if (!this->s_in[2].is_ready())
        return this->s_in[2].get_connected_edevice();
    if (!this->s_in[3].is_ready())
        return this->s_in[3].get_connected_edevice();

    this->solving = true;

    do_call_on_halt = false;
    start_tick = SDL_GetTicks();
    is_first_run = this->first_run;
    current_escript = this;

    this->val[0] = this->s_in[0].get_value();
    this->val[1] = this->s_in[1].get_value();
    this->val[2] = this->s_in[2].get_value();
    this->val[3] = this->s_in[3].get_value();

    this->socket_active[0] = (this->s_in[0].p != 0);
    this->socket_active[1] = (this->s_in[1].p != 0);
    this->socket_active[2] = (this->s_in[2].p != 0);
    this->socket_active[3] = (this->s_in[3].p != 0);

    this->lines = this->pending_lines;
    this->pending_lines.clear();

    if (this->normal_draw) {
        this->draw_pre_solve(this->normal_draw);

        /* clear vertex data */
        this->normal_draw->sprite_count = this->normal_draw->pending_sprite_count;
        this->normal_draw->pending_sprite_count = 0;
    }

    if (this->static_draw) {
        this->draw_pre_solve(this->static_draw);
    }

    if (W->level.version < LEVEL_VERSION_1_5) {
        if (luaL_dostring(this->L, this->properties[0].v.s.buf)) {
            G->add_error(this, ERROR_SCRIPT_COMPILE, lua_pop_error(this->L, "Error compiling Lua code (<1.5): "));
        }
    } else if (this->run) {
        if (this->first_run) {
            /* In preparation of every function call, start_tick and timelimit should be set */
            func_start_tick = SDL_GetTicks();
            cur_func_id = FUNC_INIT;
            lua_getglobal(this->L, "init");

            if (!lua_isnil(this->L, -1)) {
                lua_pushboolean(this->L, G->state.sandbox || G->state.test_playing);

                if (lua_pcall(this->L, 1, 0, 0) != 0) {
                    G->add_error(this, ERROR_SCRIPT_COMPILE, lua_pop_error(this->L, "Error calling init: "));
                }
            } else {
                lua_pop(this->L, 1);
            }
        }

        if (this->has_on_event) {
            /* Call any events */
            for (int x=0; x<WORLD_EVENT__NUM; ++x) {
                while (this->events[x]) {
                    func_start_tick = SDL_GetTicks();
                    cur_func_id = FUNC_ON_EVENT;
                    lua_getglobal(this->L, "on_event");

                    lua_pushnumber(this->L, x); // x = event_id

                    if (lua_pcall(this->L, 1, 0, 0) != 0) {
                        G->add_error(this, ERROR_SCRIPT_COMPILE, lua_pop_error(this->L, "Error calling on_event: "));
                    }

                    -- this->events[x];
                }
            }
        }

        if (this->get_response() != PROMPT_RESPONSE_NONE) {
            if (this->has_on_response) {
                func_start_tick = SDL_GetTicks();
                cur_func_id = FUNC_ON_RESPONSE;

                lua_getglobal(this->L, "on_response");

                lua_pushnumber(this->L, this->get_response()); // response
                lua_pushnumber(this->L, this->prompt_id); // prompt id

                if (lua_pcall(this->L, 2, 0, 0) != 0) {
                    G->add_error(this, ERROR_SCRIPT_COMPILE, lua_pop_error(this->L, "Error calling on_event: "));
                }
            }

            this->set_response(PROMPT_RESPONSE_NONE);
            this->prompt_id = 0;
        }

        for (std::set<tms::event*>::iterator it = this->input_events.begin();
                it != this->input_events.end(); ++it) {
            tms::event *ev = static_cast<tms::event*>(*it);

            func_start_tick = SDL_GetTicks();
            cur_func_id = FUNC_ON_INPUT;
            lua_getglobal(this->L, "on_input");

            int params = 0;

            lua_pushnumber(this->L, ev->type);
            ++ params;

            lua_newtable(this->L);
            ++ params;

            if (ev->type == TMS_EV_KEY_PRESS || ev->type == TMS_EV_KEY_UP) {
                lua_pushstring(this->L, "keycode");
                lua_pushnumber(this->L, ev->data.key.keycode);
                lua_settable(this->L, -3);
            } else if (ev->type == TMS_EV_POINTER_DOWN || ev->type == TMS_EV_POINTER_UP) {
                lua_pushstring(this->L, "pid");
                lua_pushnumber(this->L, ev->data.motion.pointer_id);
                lua_settable(this->L, -3);

                lua_pushstring(this->L, "x");
                lua_pushnumber(this->L, ev->data.motion.x);
                lua_settable(this->L, -3);

                lua_pushstring(this->L, "y");
                lua_pushnumber(this->L, ev->data.motion.y);
                lua_settable(this->L, -3);
            }

            if (lua_pcall(this->L, params, 0, 0) != 0) {
                G->add_error(this, ERROR_SCRIPT_COMPILE, lua_pop_error(this->L, "Error calling on_input: "));
            }
        }

        func_start_tick = SDL_GetTicks();
        if (this->has_step) {
            cur_func_id = FUNC_STEP;
            lua_getglobal(this->L, "step");

            lua_pushnumber(this->L, W->step_count);

            int n = lua_pcall(this->L, 1, 0, 0);

            if (n != 0) {
                G->add_error(this, ERROR_SCRIPT_COMPILE, lua_pop_error(this->L, "Error calling step: "));
            }
        }

        if (do_call_on_halt) {
            func_start_tick = SDL_GetTicks();
            cur_func_id = FUNC_ON_HALT;
            lua_getglobal(L, "on_halt");

            if (!lua_isnil(L, -1)) {
                if (lua_pcall(L, 0, 0, 0) != 0) {
                    G->add_error(this, ERROR_SCRIPT_COMPILE, lua_pop_error(this->L, "Error calling on_halt: "));
                }
            } else {
                lua_pop(L, 1);
            }
        }
    }

    lua_pushglobaltable(this->L); // S: tbl
    plua_foreach(this->L, 1, subscribe_to_entity, this); // S: tbl
    lua_pop(this->L, 1); // S:

    if (this->normal_draw) {
        this->draw_post_solve(this->normal_draw);
    }

    if (this->static_draw) {
        this->draw_post_solve(this->static_draw);
    }

    if (!this->s_out[0].written()) this->s_out[0].write(0.f);
    if (!this->s_out[1].written()) this->s_out[1].write(0.f);
    if (!this->s_out[2].written()) this->s_out[2].write(0.f);
    if (!this->s_out[3].written()) this->s_out[3].write(0.f);

    this->first_run = 0;
    this->input_events.clear();

    std::map<uint32_t, receiver_base*>::iterator i;
    for (i = this->receivers.begin(); i != this->receivers.end(); ++i) {
        i->second->reset_recv_value();
    }

    for (int x=0; x<WORLD_EVENT__NUM; x++) {
        this->events[x] = 0;
    }

    this->solving = false;

    return 0;
}

static unsigned char keys[5] = {0x41, 0xf3, 0x1a, 0x44, 0x14};

#define IS_ENCRYPTED(ver) \
        ver >= LEVEL_VERSION_1_5 \
     && ver <  LEVEL_VERSION_2023_06_05

void
escript::on_load(bool created, bool has_state)
{
    entity::on_load(created, has_state);

    /* XXX: needs to be tested with community levels */
    if (!created && this->properties[1].v.i & ESCRIPT_USE_EXTERNAL_EDITOR && W->level_id_type == LEVEL_LOCAL) {
        char path[ESCRIPT_EXTERNAL_PATH_LEN];
        this->generate_external_path(path);

        FILE *fh = fopen(path, "rb");
        if (fh) {
            if (fseek(fh, 0L, SEEK_END) == 0) {
                size_t sz = ftell(fh);
                if (sz != -1) {
                    rewind(fh);
                    char *data = (char*)calloc(sizeof(char), sz + 1);

                    size_t result = fread(data, sizeof(char), sz, fh);
                    if (result == sz) {
                        tms_debugf("Successfully read source from file!");

                        tms_debugf("Data: '%s'", data);

                        this->set_property(0, data);
                    }

                    free(data);
                }
            }

            fclose(fh);
            return;
        } else {
            tms_errorf("External editing enabled, yet no file at %s was readable.", path);
            /* If this happens, we will fall back to using non-external editing. */
            this->properties[1].v.i &= ~ESCRIPT_USE_EXTERNAL_EDITOR;
        }
    }

    if (!created) {
        // For old level versions above 1.5+, LuaScript code is encrypted
        if (IS_ENCRYPTED(W->level.version)) {
            for (uint32_t x=0; x<this->properties[0].v.s.len; ++x) {
                this->properties[0].v.s.buf[x] ^= keys[x%5];
            }
        }
    }
}

void
escript::pre_write()
{
    entity::pre_write();

    // For old level versions above 1.5+, LuaScript code is encrypted
    if (IS_ENCRYPTED(W->level.version)) {
        for (uint32_t x=0; x<this->properties[0].v.s.len; ++x) {
            this->properties[0].v.s.buf[x] ^= keys[x%5];
        }
    }
}

void
escript::post_write()
{
    entity::post_write();

    // For old level versions above 1.5+, LuaScript code is encrypted
    if (IS_ENCRYPTED(W->level.version)) {
        for (uint32_t x=0; x<this->properties[0].v.s.len; ++x) {
            this->properties[0].v.s.buf[x] ^= keys[x%5];
        }
    }
}

/*
 * State format:
 * #1               uint32  codelen
 * #codelen         buf     code
 * #1               uint32  num_receivers
 * #num_receivers   uint32  receiver_frequency
 * #1               uint32  local_id
 * #1               uint32  blending_mode
 * #1               uint32  filtering
 * #1               uint32  coordinate_mode
 * #1               float   draw_z
 * #4               float   draw_tint
 * #1               uint8   draw_initialized
 * if draw_initialized:
 *      #1          uint32  texture_width
 *      #1          uint32  texture_height
 *      #1          uint8   num_channels
 *      #w*h*nc     buf     texture_buffer
 * if static_draw_initialized:
 *      #1          uint32  texture_width
 *      #1          uint32  texture_height
 *      #1          uint8   num_channels
 *      #w*h*nc     buf     texture_buffer
 *      #1          uint32  num_static_sprites
 *      for sprite in static_sprites:
 *          #1      float   x
 *          #1      float   y
 *          #1      float   r
 *          #1      float   w
 *          #1      float   h
 *          #1      uint32  bx
 *          #1      uint32  by
 *          #1      uint32  tx
 *          #1      uint32  ty
 */
void
escript::write_state(lvlinfo *lvl, lvlbuf *lb)
{
    entity::write_state(lvl, lb);

    tms_infof("gettop: %d", lua_gettop(this->L));

    lua_dump_stack(this->L);

#ifdef DEBUG
    lua_pushboolean(L, true);
    eris_set_setting(L, "path", 1);
    //eris_set_setting(L, "spio", 1);
    lua_pop(L, 1);
#endif

    persist_all(this->L, this);

    /* Save all global variables */
    lb->w_s_uint32(this->data.size);
    lb->w_s_buf(this->data.buf, this->data.size);

    /* Save all frequencies */
    lb->w_s_uint32(this->receivers.size());
    std::map<uint32_t, receiver_base*>::iterator i;
    for (i = this->receivers.begin(); i != this->receivers.end(); ++i) {
        lb->w_s_uint32(i->first);
    }

    lb->w_s_uint32(this->local_id);
    lb->w_s_uint32(this->blending_mode);
    lb->w_s_uint32(this->filtering);
    lb->w_s_uint32(this->coordinate_mode);
    lb->w_s_float(this->draw_z);
    lb->w_s_float(this->draw_tint.r);
    lb->w_s_float(this->draw_tint.g);
    lb->w_s_float(this->draw_tint.b);
    lb->w_s_float(this->draw_tint.a);

    draw_data *draw = this->normal_draw;

    if (draw) {
        lb->w_s_uint8(1); // normal draw is initialized!
        lb->w_s_uint32(draw->texture_width);
        lb->w_s_uint32(draw->texture_height);
        lb->w_s_uint8(draw->texture_num_channels);

        uint32_t buf_sz = draw->texture_width * draw->texture_height * draw->texture_num_channels;
        lb->w_s_buf((char*)draw->texture->data, buf_sz);
    } else {
        lb->w_s_uint8(0);
    }

    draw = this->static_draw;
    if (draw) {
        lb->w_s_uint8(1); // static draw is initialized!
        lb->w_s_uint32(draw->texture_width);
        lb->w_s_uint32(draw->texture_height);
        lb->w_s_uint8(draw->texture_num_channels);

        uint32_t buf_sz = draw->texture_width * draw->texture_height * draw->texture_num_channels;
        lb->w_s_buf((char*)draw->texture->data, buf_sz);

        /* save all added static sprites */
        lb->w_s_uint32(this->static_sprites.size());
        for (sprite_iterator i = this->static_sprites.begin();
                i != this->static_sprites.end(); ++i) {
            struct escript_sprite *s = &(*i);
            lb->w_s_float(s->x);
            lb->w_s_float(s->y);
            lb->w_s_float(s->r);
            lb->w_s_float(s->w);
            lb->w_s_float(s->h);
            lb->w_s_uint32(s->bx);
            lb->w_s_uint32(s->by);
            lb->w_s_uint32(s->tx);
            lb->w_s_uint32(s->ty);
        }
    } else {
        lb->w_s_uint8(0);
    }
}

void
escript::read_state(lvlinfo *lvl, lvlbuf *lb)
{
    entity::read_state(lvl, lb);

    free(this->data.buf);
    this->data.size = lb->r_uint32();
    this->data.buf = (char*)calloc(this->data.size+1, sizeof(char));
    lb->r_buf(this->data.buf, this->data.size);

    /* Restore all frequencies */
    uint32_t num_frequencies = lb->r_uint32();
    for (uint32_t x=0; x<num_frequencies; ++x) {
        uint32_t freq = lb->r_uint32();
        receiver_base *rb = new receiver_base();
        std::pair<std::map<uint32_t, receiver_base*>::iterator, bool> ret;
        ret = this->receivers.insert(std::pair<uint32_t, receiver_base*>(freq, rb));
        if (!ret.second) {
            delete rb;
        } else {
            W->add_receiver(freq, rb);
        }
    }

    this->local_id          = lb->r_uint32();
    this->blending_mode     = lb->r_uint32();
    this->filtering         = lb->r_uint32();
    this->coordinate_mode   = lb->r_uint32();
    this->draw_z            = lb->r_float();
    this->draw_tint.r       = lb->r_float();
    this->draw_tint.g       = lb->r_float();
    this->draw_tint.b       = lb->r_float();
    this->draw_tint.a       = lb->r_float();

    bool normal_draw_initialized = lb->r_uint8();
    if (normal_draw_initialized) {
        int w = lb->r_uint32();
        int h = lb->r_uint32();
        uint8_t num_channels = lb->r_uint8();
        this->normal_draw = new draw_data(this, w, h, num_channels);

        uint32_t buf_sz = this->normal_draw->texture_width * this->normal_draw->texture_height * num_channels;
        lb->r_buf((char*)this->normal_draw->texture->data, buf_sz);
    }

    bool static_draw_initialized = lb->r_uint8();
    if (static_draw_initialized) {
        int w = lb->r_uint32();
        int h = lb->r_uint32();
        uint8_t num_channels = lb->r_uint8();
        this->static_draw = new draw_data(this, w, h, num_channels);

        uint32_t buf_sz = this->static_draw->texture_width * this->static_draw->texture_height * num_channels;
        lb->r_buf((char*)this->static_draw->texture->data, buf_sz);

        uint32_t num_static_sprites = lb->r_uint32();
        tms_debugf("num static sprites: %u", num_static_sprites);
        for (uint32_t i=0; i<num_static_sprites; ++i) {
            struct escript_sprite s;
            s.x = lb->r_float();
            s.y = lb->r_float();
            s.r = lb->r_float();
            s.w = lb->r_float();
            s.h = lb->r_float();
            s.bx = (int)lb->r_uint32();
            s.by = (int)lb->r_uint32();
            s.tx = (int)lb->r_uint32();
            s.ty = (int)lb->r_uint32();

            this->static_sprites.push_back(s);
        }
    }
}

void
escript::restore()
{
    if (this->data.size > 0) {
        this->first_run = false;

        unpersist_all(this->L, this);
    }

    tms_debugf("static_sprites count: %d", (int)this->static_sprites.size());
    for (sprite_iterator i = this->static_sprites.begin();
            i != this->static_sprites.end(); ++i) {
        struct escript_sprite s = *i;

        this->add_static_sprite(s.x, s.y, s.r, s.w, s.h, s.bx, s.by, s.tx, s.ty, false);
    }
}

void
escript::draw_pre_solve(draw_data *draw)
{

}

void
escript::draw_post_solve(draw_data *draw)
{
    /* Re-upload texture if buffer has been modified */
    if (draw->texture_modified) {
        tms_texture_upload(draw->texture);
        tms_debugf("reuploading %p", draw);
        draw->texture_modified = false;
    }

    draw->sprite_mesh->i_count = draw->sprite_count*6;
    if (draw->sprite_count > 0) {
        draw->verts->upload_partial(draw->sprite_count*4*sizeof(struct vert));
        draw->verts_modified = 0;
    }

    if (this->coordinate_mode == ESCRIPT_WORLD || this->coordinate_mode == ESCRIPT_LOCAL) {
        if (!draw->sprite_ent->scene) {
            tms_debugf("add entity to scene");
            this->add_child(draw->sprite_ent);
            tms_scene_add_entity(this->scene, draw->sprite_ent);
        }

        if (this->blending_mode != draw->mat.pipeline[0].blend_mode) {
            struct tms_scene *sc = draw->sprite_ent->scene;
            tms_scene_remove_entity(sc, draw->sprite_ent);
            draw->mat.pipeline[0].blend_mode = this->blending_mode;
            tms_scene_add_entity(sc, draw->sprite_ent);
        }
    } else if (this->coordinate_mode == ESCRIPT_SCREEN) {
        if (draw->sprite_ent->scene) {
            tms_debugf("remove entity from scene");
            this->remove_child(draw->sprite_ent);
            tms_scene_remove_entity(this->scene, draw->sprite_ent);
        }
    }

    bool filter_changed = false;
    if (this->filtering == 0 && draw->texture->filter == GL_LINEAR) {
        draw->texture->filter = GL_NEAREST;
        filter_changed = true;
    } else if (this->filtering == 1 && draw->texture->filter == GL_NEAREST) {
        draw->texture->filter = GL_LINEAR;
        filter_changed = true;
    }

    if (filter_changed) {
        glBindTexture(GL_TEXTURE_2D, draw->texture->gl_texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, draw->texture->filter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, draw->texture->filter);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

draw_data::draw_data(escript *parent, int width, int height, uint8_t num_channels)
{
    this->parent = parent;

    this->texture = tms_texture_alloc();
    this->texture->wrap = GL_CLAMP_TO_EDGE;

    this->texture_width = width;
    this->texture_height = height;
    this->texture_num_channels = num_channels;

    this->verts = new tms::gbuffer(4*MAX_SPRITES*sizeof(struct vert));
    this->verts->usage = TMS_GBUFFER_STREAM_DRAW;

    this->indices = new tms::gbuffer(6*MAX_SPRITES*sizeof(uint16_t));
    this->indices->usage = TMS_GBUFFER_STATIC_DRAW;

    this->va = new tms::varray(3);
    this->va->map_attribute("position", 3, GL_FLOAT, verts);
    this->va->map_attribute("uv", 2, GL_FLOAT, verts);
    this->va->map_attribute("vcolor", 4, GL_FLOAT, verts);

    uint16_t *i = (uint16_t*)indices->get_buffer();
    for (int x=0; x<MAX_SPRITES; x++) {
        int o = x*6;
        int vo = x*4;

        i[o+0] = vo;
        i[o+1] = vo+1;
        i[o+2] = vo+2;
        i[o+3] = vo;
        i[o+4] = vo+2;
        i[o+5] = vo+3;
    }

    this->indices->upload();

    tms_texture_alloc_buffer(this->texture, width, height, num_channels);
    tms_texture_clear_buffer(this->texture, 127);

    this->sprite_ent = new tms::entity();
    this->sprite_mesh = new tms::mesh(va, indices);

    this->sprite_ent->prio = this->parent->get_layer();
    this->sprite_ent->set_mesh(sprite_mesh);

    this->mat = m_spritebuf;
    this->mat.pipeline[0].texture[0] = this->texture;
    this->sprite_ent->set_material(&this->mat);

    this->sprite_mesh->i_start = 0;
    this->sprite_mesh->i_count = 0;

    this->sprite_count = 0;
    this->pending_sprite_count = 0;

    this->texture_modified = true;
    this->verts_modified = true;
}

draw_data::~draw_data()
{
    tms_texture_free(this->texture);
}

void
draw_data::update_effects()
{
    if (this->sprite_ent->scene) {
        tms_scene_uncull_entity(G->get_scene(), (struct tms_entity*)this->sprite_ent);
        tms_graph_uncull_entity(G->graph, (struct tms_entity*)this->sprite_ent);
    }
    tmat4_load_identity(this->sprite_ent->M);
    tmat3_load_identity(this->sprite_ent->N);
}

void
draw_data::resize_texture_buffer(int width, int height, uint8_t num_channels/*=4*/)
{
    if (this->texture->is_buffered) {
        tms_texture_free_buffer(this->texture);
    }

}

void
escript::add_line(const struct escript_line &line)
{
    if (this->solving) {
        this->lines.push_back(line);
    } else {
        this->pending_lines.push_back(line);
    }
}

void
escript::add_static_sprite(float x, float y, float r, float w, float h, int bx, int by, int tx, int ty, bool add/*=true*/)
{
    draw_data *draw = this->static_draw;

    tvec2 uvb = {(float)bx / draw->texture_width, (float)by / draw->texture_height};
    tvec2 uvt = {(float)tx / draw->texture_width, (float)ty / draw->texture_height};

    if (draw->sprite_count < MAX_SPRITES) {
        struct vert *_b = (struct vert*)draw->verts->get_buffer();
        int n = draw->sprite_count;

        if (r != 0.f) {
            float cs, sn;
            tmath_sincos(r, &sn, &cs);

            for (int ix=0; ix<4; ix++) {
                _b[n*4+ix] = base[ix];

                _b[n*4+ix].pos.x *= w;
                _b[n*4+ix].pos.y *= h;

                float _x = _b[n*4+ix].pos.x * cs - _b[n*4+ix].pos.y * sn;
                float _y = _b[n*4+ix].pos.x * sn + _b[n*4+ix].pos.y * cs;
                _b[n*4+ix].pos.x = _x;
                _b[n*4+ix].pos.y = _y;

                _b[n*4+ix].pos.x += x;
                _b[n*4+ix].pos.y += y;
                _b[n*4+ix].pos.z += this->draw_z;

                _b[n*4+ix].color.r = this->draw_tint.r;
                _b[n*4+ix].color.g = this->draw_tint.g;
                _b[n*4+ix].color.b = this->draw_tint.b;
                _b[n*4+ix].color.a = this->draw_tint.a;

                _b[n*4+ix].uv.x = ((ix == 0 || ix == 3) ? uvt.x : uvb.x);
                _b[n*4+ix].uv.y = (ix < 2 ? uvt.y : uvb.y);
            }
        } else {
            for (int ix=0; ix<4; ix++) {
                _b[n*4+ix] = base[ix];

                _b[n*4+ix].pos.x *= w;
                _b[n*4+ix].pos.y *= h;

                _b[n*4+ix].pos.x += x;
                _b[n*4+ix].pos.y += y;

                _b[n*4+ix].pos.z += this->draw_z;

                _b[n*4+ix].color.r = this->draw_tint.r;
                _b[n*4+ix].color.g = this->draw_tint.g;
                _b[n*4+ix].color.b = this->draw_tint.b;
                _b[n*4+ix].color.a = this->draw_tint.a;

                _b[n*4+ix].uv.x = ((ix == 0 || ix == 3) ? uvt.x : uvb.x);
                _b[n*4+ix].uv.y = (ix < 2 ? uvt.y : uvb.y);
            }
        }

        draw->sprite_count ++;

        if (add) {
            struct escript_sprite sprite = {
                x, y,
                r,
                w, h,
                bx, by,
                tx, ty
            };
            this->static_sprites.push_back(sprite);
        }
    }
}

/* Buffers inserted to this function will be assumed to have
 * a length of ESCRIPT_EXTERNAL_PATH_LEN */
void
escript::generate_external_path(char *buf)
{
    snprintf(buf, ESCRIPT_EXTERNAL_PATH_LEN-1, "%s/%d-%d.lua",
            pkgman::get_cache_path(W->level_id_type),
            W->level.local_id, this->id);
}
