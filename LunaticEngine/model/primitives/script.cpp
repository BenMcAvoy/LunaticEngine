#include "pch.h"

#include "script.h"

#include "core/engine.h"

#include "model/primitives/sprite.h"
#include "model/primitives/script.h"

using namespace Lunatic;

// Helper functions to register simple vector metatables for arithmetic in Lua
static int luavec_read_component(lua_State* L, int idx, const char* key, int numericIndex, float def) {
    lua_getfield(L, idx, key);
    bool has = !lua_isnil(L, -1);
    float val = def;
    if (has) val = static_cast<float>(luaL_optnumber(L, -1, def));
    lua_pop(L, 1);
    if (!has && numericIndex > 0) {
        lua_rawgeti(L, idx, numericIndex);
        val = static_cast<float>(luaL_optnumber(L, -1, def));
        lua_pop(L,1);
    }
    return *reinterpret_cast<int*>(&val); // not used directly; helper kept for symmetry
}

static void luavec_readN(lua_State* L, int idx, int n, float* out) {
    const char* keysXY[4] = {"x","y","z","w"};
    const char* keysRGB[4] = {"r","g","b","a"};
    for (int k=0;k<n;++k) {
        // default 0
        float v = 0.0f; bool has = false;
        lua_getfield(L, idx, keysXY[k]); has = !lua_isnil(L, -1); if (has) v = static_cast<float>(luaL_optnumber(L, -1, 0.0)); lua_pop(L,1);
        if (!has) { lua_getfield(L, idx, keysRGB[k]); has = !lua_isnil(L, -1); if (has) v = static_cast<float>(luaL_optnumber(L, -1, 0.0)); lua_pop(L,1); }
        if (!has) { lua_rawgeti(L, idx, k+1); v = static_cast<float>(luaL_optnumber(L, -1, 0.0)); lua_pop(L,1); }
        out[k] = v;
    }
}

static void luavec_pushN(lua_State* L, int n, const float* v, const char* mtName) {
    const char* keysXY[4] = {"x","y","z","w"};
    const char* keysRGB[4] = {"r","g","b","a"};
    lua_newtable(L);
    for (int k=0;k<n;++k) { lua_pushnumber(L, v[k]); lua_setfield(L, -2, keysXY[k]); }
    for (int k=0;k<n && keysRGB[k];++k) { lua_pushnumber(L, v[k]); lua_setfield(L, -2, keysRGB[k]); }
    for (int k=0;k<n;++k) { lua_pushnumber(L, v[k]); lua_rawseti(L, -2, k+1); }
    luaL_getmetatable(L, mtName); lua_setmetatable(L, -2);
}

static int luavec_add(lua_State* L) {
    int n = static_cast<int>(lua_tointeger(L, lua_upvalueindex(1)));
    const char* mtName = lua_tostring(L, lua_upvalueindex(2));
    float a[4]{}, b[4]{}; luavec_readN(L, 1, n, a); luavec_readN(L, 2, n, b);
    for (int i=0;i<n;++i) a[i] += b[i];
    luavec_pushN(L, n, a, mtName);
    return 1;
}
static int luavec_sub(lua_State* L) {
    int n = static_cast<int>(lua_tointeger(L, lua_upvalueindex(1)));
    const char* mtName = lua_tostring(L, lua_upvalueindex(2));
    float a[4]{}, b[4]{}; luavec_readN(L, 1, n, a); luavec_readN(L, 2, n, b);
    for (int i=0;i<n;++i) a[i] -= b[i];
    luavec_pushN(L, n, a, mtName);
    return 1;
}
static int luavec_mul(lua_State* L) {
    int n = static_cast<int>(lua_tointeger(L, lua_upvalueindex(1)));
    const char* mtName = lua_tostring(L, lua_upvalueindex(2));
    float v[4]{}; float s = 1.0f;
    if (lua_istable(L, 1) && lua_isnumber(L, 2)) { luavec_readN(L, 1, n, v); s = static_cast<float>(lua_tonumber(L, 2)); }
    else if (lua_isnumber(L, 1) && lua_istable(L, 2)) { s = static_cast<float>(lua_tonumber(L, 1)); luavec_readN(L, 2, n, v); }
    else return luaL_error(L, "__mul expects (vec, number) or (number, vec)");
    for (int i=0;i<n;++i) v[i] *= s;
    luavec_pushN(L, n, v, mtName);
    return 1;
}
static int luavec_div(lua_State* L) {
    int n = static_cast<int>(lua_tointeger(L, lua_upvalueindex(1)));
    const char* mtName = lua_tostring(L, lua_upvalueindex(2));
    float v[4]{}; float s = 1.0f;
    if (lua_istable(L, 1) && lua_isnumber(L, 2)) { luavec_readN(L, 1, n, v); s = static_cast<float>(lua_tonumber(L, 2)); }
    else return luaL_error(L, "__div expects (vec, number)");
    if (s == 0.0f) return luaL_error(L, "division by zero");
    for (int i=0;i<n;++i) v[i] /= s;
    luavec_pushN(L, n, v, mtName);
    return 1;
}
static int luavec_unm(lua_State* L) {
    int n = static_cast<int>(lua_tointeger(L, lua_upvalueindex(1)));
    const char* mtName = lua_tostring(L, lua_upvalueindex(2));
    float v[4]{}; luavec_readN(L, 1, n, v);
    for (int i=0;i<n;++i) v[i] = -v[i];
    luavec_pushN(L, n, v, mtName);
    return 1;
}
static void luavec_register_mt(lua_State* L, const char* mtName, int n) {
    if (luaL_newmetatable(L, mtName)) {
        lua_pushinteger(L, n); lua_pushstring(L, mtName); lua_pushcclosure(L, luavec_add, 2); lua_setfield(L, -2, "__add");
        lua_pushinteger(L, n); lua_pushstring(L, mtName); lua_pushcclosure(L, luavec_sub, 2); lua_setfield(L, -2, "__sub");
        lua_pushinteger(L, n); lua_pushstring(L, mtName); lua_pushcclosure(L, luavec_mul, 2); lua_setfield(L, -2, "__mul");
        lua_pushinteger(L, n); lua_pushstring(L, mtName); lua_pushcclosure(L, luavec_div, 2); lua_setfield(L, -2, "__div");
        lua_pushinteger(L, n); lua_pushstring(L, mtName); lua_pushcclosure(L, luavec_unm, 2); lua_setfield(L, -2, "__unm");
    }
    lua_pop(L, 1);
}

Script::Script(std::string_view name) : Instance(name), Updateable() {
    typeInfo = rttr::type::get<Script>();
}

void Script::loadCode(std::string_view path) {
    if (!luaInit_) {
        L_ = luaL_newstate();
        luaL_openlibs(L_); // TODO: Sandbox out `os` and `io` libraries, anything dangerous



        luaInit_ = true;
    }

    codePath_ = path;
    if (luaL_loadfile(L_, codePath_.data()) != LUA_OK) {
        spdlog::error("Lua load error in script {}: {}", getName(), lua_tostring(L_, -1));
        lua_pop(L_, 1); // remove error message
        finished_ = true;
        return;
    }

    co_ = lua_newthread(L_);
    lua_pushvalue(L_, -2); // copy the loaded chunk to the top of the stack
    lua_xmove(L_, co_, 1); // move the chunk to the coroutine
    lua_pop(L_, 1); // remove the original chunk from the main stack
    if (co_ == nullptr) {
        spdlog::error("Failed to create Lua coroutine for script {}", getName());
        finished_ = true;
        return;
    }

    /* HACK: environment example:
    lua_newtable(co_);
    lua_pushvalue(co_, -1);
    lua_setglobal(co_, "_ENV");
    */

    //env = setmetatable({
    //   root = <userdata>, script = <userdata>, print = <closure>
    // }, { __index = _G })
    // so that multiple scripts don't share the same globals (esp. `script`).
    // Stack: co_ has the chunk function on top right now.
    // Create env table
    lua_newtable(co_); // env

    // env.root
    {
        auto& engine = Engine::getInstance();
        auto& root = engine.rootInstance;
        LuaInstance::createInLua(root, co_);    // push userdata
        lua_setfield(co_, -2, "root");         // env.root = userdata (pops userdata)
    }

    // env.script
    {
        auto script = shared_from_this();
        LuaInstance::createInLua(script, co_);  // push userdata
        lua_setfield(co_, -2, "script");       // env.script = userdata (pops userdata)
    }

    // env.print
    {
        // Push the Script* as a lightuserdata upvalue for the closure
        lua_pushlightuserdata(co_, this);
        lua_pushcclosure(co_, [](lua_State* L) -> int {
            Script* script = static_cast<Script*>(lua_touserdata(L, lua_upvalueindex(1)));

            int n = lua_gettop(L);
            std::string output;
            for (int i = 1; i <= n; ++i) {
                lua_getglobal(L, "tostring");
                lua_pushvalue(L, i);
                if (lua_pcall(L, 1, 1, 0) != LUA_OK) {
                    const char* err = lua_tostring(L, -1);
                    output += "<tostring error: ";
                    if (err) output += err;
                    output += ">";
                    lua_pop(L, 1);
                } else {
                    size_t len = 0;
                    const char* str = lua_tolstring(L, -1, &len);
                    if (str && len > 0) output.append(str, len);
                    lua_pop(L, 1);
                }
                if (i < n) output += "\t";
            }

            std::string name = script ? std::string(script->getName()) : std::string("<unknown>");
            spdlog::info("[Lua][{}]: {}", name, output);
            return 0;
        }, 1);
        lua_setfield(co_, -2, "print"); // env.print = closure
    }

    // env.engine (create a LuaEngine from Engine::getInstance())
    {
        auto& engine = Engine::getInstance();
        LuaEngine::createInLua(&engine, co_);    // push userdata
        lua_setfield(co_, -2, "engine");         // env.engine = userdata (pops userdata)
    }

    // env.vec2: table with __call constructor and constants
    lua_pushcfunction(co_, [](lua_State* L) -> int {
        float x = static_cast<float>(luaL_checknumber(L, 1));
        float y = static_cast<float>(luaL_checknumber(L, 2));
        lua_newtable(L);
        lua_pushnumber(L, x); lua_setfield(L, -2, "x");
        lua_pushnumber(L, y); lua_setfield(L, -2, "y");
        return 1;
    });
    int vec2_ctor = lua_gettop(co_);
    lua_newtable(co_);                 // vec2 table
    lua_pushvalue(co_, vec2_ctor); lua_setfield(co_, -2, "new");
    lua_newtable(co_);                 // mt
    lua_pushvalue(co_, vec2_ctor); lua_setfield(co_, -2, "__call");
    lua_setmetatable(co_, -2);         // setmetatable(vec2, mt)
    // constants
    lua_pushvalue(co_, vec2_ctor); lua_pushnumber(co_, 0); lua_pushnumber(co_, 0); lua_call(co_, 2, 1); lua_setfield(co_, -2, "zero");
    lua_pushvalue(co_, vec2_ctor); lua_pushnumber(co_, 1); lua_pushnumber(co_, 1); lua_call(co_, 2, 1); lua_setfield(co_, -2, "one");
    lua_setfield(co_, -3, "vec2");   // env.vec2 = vec2 table
    lua_pop(co_, 1); // pop ctor

    // env.vec3: table with __call constructor and constants
    lua_pushcfunction(co_, [](lua_State* L) -> int {
        float x = static_cast<float>(luaL_checknumber(L, 1));
        float y = static_cast<float>(luaL_checknumber(L, 2));
        float z = static_cast<float>(luaL_checknumber(L, 3));
        lua_newtable(L);
        lua_pushnumber(L, x); lua_setfield(L, -2, "x");
        lua_pushnumber(L, y); lua_setfield(L, -2, "y");
        lua_pushnumber(L, z); lua_setfield(L, -2, "z");
        lua_pushnumber(L, x); lua_setfield(L, -2, "r");
        lua_pushnumber(L, y); lua_setfield(L, -2, "g");
        lua_pushnumber(L, z); lua_setfield(L, -2, "b");
        return 1;
    });
    int vec3_ctor = lua_gettop(co_);
    lua_newtable(co_);
    lua_pushvalue(co_, vec3_ctor); lua_setfield(co_, -2, "new");
    lua_newtable(co_);
    lua_pushvalue(co_, vec3_ctor); lua_setfield(co_, -2, "__call");
    lua_setmetatable(co_, -2);
    // constants
    // black and white (RGB)
    lua_pushvalue(co_, vec3_ctor); lua_pushnumber(co_, 0); lua_pushnumber(co_, 0); lua_pushnumber(co_, 0); lua_call(co_, 3, 1); lua_setfield(co_, -2, "black");
    lua_pushvalue(co_, vec3_ctor); lua_pushnumber(co_, 1); lua_pushnumber(co_, 1); lua_pushnumber(co_, 1); lua_call(co_, 3, 1); lua_setfield(co_, -2, "white");
    lua_setfield(co_, -3, "vec3");
    lua_pop(co_, 1);

    // env.vec4: table with __call constructor and color constants
    lua_pushcfunction(co_, [](lua_State* L) -> int {
        float x = static_cast<float>(luaL_checknumber(L, 1));
        float y = static_cast<float>(luaL_checknumber(L, 2));
        float z = static_cast<float>(luaL_checknumber(L, 3));
        float w = static_cast<float>(luaL_checknumber(L, 4));
        lua_newtable(L);
        lua_pushnumber(L, x); lua_setfield(L, -2, "x");
        lua_pushnumber(L, y); lua_setfield(L, -2, "y");
        lua_pushnumber(L, z); lua_setfield(L, -2, "z");
        lua_pushnumber(L, w); lua_setfield(L, -2, "w");
        lua_pushnumber(L, x); lua_setfield(L, -2, "r");
        lua_pushnumber(L, y); lua_setfield(L, -2, "g");
        lua_pushnumber(L, z); lua_setfield(L, -2, "b");
        lua_pushnumber(L, w); lua_setfield(L, -2, "a");
        return 1;
    });
    int vec4_ctor = lua_gettop(co_);
    lua_newtable(co_);
    lua_pushvalue(co_, vec4_ctor); lua_setfield(co_, -2, "new");
    lua_newtable(co_);
    lua_pushvalue(co_, vec4_ctor); lua_setfield(co_, -2, "__call");
    lua_setmetatable(co_, -2);
    // common color constants
    // white, black, red, green, blue, transparent
    lua_pushvalue(co_, vec4_ctor); lua_pushnumber(co_, 1); lua_pushnumber(co_, 1); lua_pushnumber(co_, 1); lua_pushnumber(co_, 1); lua_call(co_, 4, 1); lua_setfield(co_, -2, "white");
    lua_pushvalue(co_, vec4_ctor); lua_pushnumber(co_, 0); lua_pushnumber(co_, 0); lua_pushnumber(co_, 0); lua_pushnumber(co_, 1); lua_call(co_, 4, 1); lua_setfield(co_, -2, "black");
    lua_pushvalue(co_, vec4_ctor); lua_pushnumber(co_, 1); lua_pushnumber(co_, 0); lua_pushnumber(co_, 0); lua_pushnumber(co_, 1); lua_call(co_, 4, 1); lua_setfield(co_, -2, "red");
    lua_pushvalue(co_, vec4_ctor); lua_pushnumber(co_, 0); lua_pushnumber(co_, 1); lua_pushnumber(co_, 0); lua_pushnumber(co_, 1); lua_call(co_, 4, 1); lua_setfield(co_, -2, "green");
    lua_pushvalue(co_, vec4_ctor); lua_pushnumber(co_, 0); lua_pushnumber(co_, 0); lua_pushnumber(co_, 1); lua_pushnumber(co_, 1); lua_call(co_, 4, 1); lua_setfield(co_, -2, "blue");
    lua_pushvalue(co_, vec4_ctor); lua_pushnumber(co_, 0); lua_pushnumber(co_, 0); lua_pushnumber(co_, 0); lua_pushnumber(co_, 0); lua_call(co_, 4, 1); lua_setfield(co_, -2, "transparent");
    lua_setfield(co_, -3, "vec4");
    lua_pop(co_, 1);

    // Register vector metatables (once per state)
    luavec_register_mt(co_, "LuaVec2Meta", 2);
    luavec_register_mt(co_, "LuaVec3Meta", 3);
    luavec_register_mt(co_, "LuaVec4Meta", 4);

    lua_newtable(co_);                // mt
    lua_pushvalue(co_, LUA_GLOBALSINDEX); // push _G
    lua_setfield(co_, -2, "__index"); // mt.__index = _G (pops _G)
    lua_setmetatable(co_, -2);         // setmetatable(env, mt) (pops mt)
    lua_setfenv(co_, -2);              // setfenv(chunk, env) (pops env)

    finished_ = false;
}

void Script::reloadCode() {
    if (codePath_.empty()) {
        spdlog::warn("No code path set for script {}, cannot reload", getName());
        return;
    }

    // We should clear all luaDataStores since reloading scripts wipes the backing data
	Engine::getInstance().clearAllLuaData();
    loadCode(codePath_);
}

void Script::update() {
	if (codePath_.empty() || finished_) {
		return; // No code to execute
	}

	//env_["script"] = static_cast<Instance*>(this);
	//static auto& engine = Engine::getInstance();
    //env_["root"] = engine.rootInstance.get();

    // TODO: bind userdatas

    int status = lua_resume(co_, 0);
    if (status == LUA_YIELD) {
        // Coroutine yielded, continue next frame
        return;
    } else if (status == LUA_OK) {
        // Coroutine finished successfully
        finished_ = true;
        return;
    } else {
        // An error occurred
        spdlog::error("Lua runtime error in script {}: {}", getName(), lua_tostring(co_, -1));
        lua_pop(co_, 1); // remove error message
        finished_ = true;
        return;
    }
}
