#include "escript.hh"
#include "model.hh"
#include "game.hh"
#include "receiver.hh"
#include "linebuffer.hh"
#include "receiver.hh"
#include "ui.hh"

typedef std::vector<struct escript_sprite>::iterator sprite_iterator;

#include "luascript/lua.hh"

#include "luascript/lua_cam.hh"
#include "luascript/lua_entity.hh"
#include "luascript/lua_game.hh"
#include "luascript/lua_this.hh"
#include "luascript/lua_world.hh"

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
escript *current_escript = 0; /* only used in poll_event, which is deprecated in 1.5 */
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

static struct lua_vert base[4] = {
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
        draw->verts->upload_partial(draw->sprite_count*4*sizeof(struct lua_vert));
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

    this->verts = new tms::gbuffer(4*MAX_SPRITES*sizeof(struct lua_vert));
    this->verts->usage = TMS_GBUFFER_STREAM_DRAW;

    this->indices = new tms::gbuffer(6*MAX_SPRITES*sizeof(uint16_t));
    this->indices->usage = TMS_GBUFFER_STATIC_DRAW;
    this->indices->target = GL_ELEMENT_ARRAY_BUFFER;

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
        struct lua_vert *_b = (struct lua_vert*)draw->verts->get_buffer();
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
