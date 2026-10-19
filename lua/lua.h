#pragma once
#include <stddef.h>

typedef struct lua_State lua_State;
typedef int (*lua_CFunction)(lua_State* L);
typedef double lua_Number;

#define LUA_GLOBALSINDEX (-10002)
#define LUA_MULTRET (-1)

extern "C" {
    lua_State* luaL_newstate(void);
    void luaL_openlibs(lua_State* L);
    void lua_close(lua_State* L);
    int luaL_loadbuffer(lua_State* L, const char* buff, size_t sz, const char* name);
    int lua_pcall(lua_State* L, int nargs, int nresults, int errfunc);
    void lua_pushvalue(lua_State* L, int idx);
    void lua_setfield(lua_State* L, int idx, const char* k);
    void lua_getfield(lua_State* L, int idx, const char* k);
    void lua_setglobal(lua_State* L, const char* s);
    void lua_getglobal(lua_State* L, const char* s);
    void lua_pushstring(lua_State* L, const char* s);
    void lua_pushnumber(lua_State* L, lua_Number n);
    void lua_pushboolean(lua_State* L, int b);
    void lua_pushnil(lua_State* L);
    void lua_newtable(lua_State* L);
    const char* lua_tostring(lua_State* L, int idx);
    int lua_toboolean(lua_State* L, int idx);
    lua_Number lua_tonumber(lua_State* L, int idx);
    void lua_pop(lua_State* L, int n);
}
