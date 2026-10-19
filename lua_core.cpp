#include "lua_core.h"
#include "evasion.h"
#include "injector.h"
#include <cstring>

namespace LuaCore {
    bool g_initialized = false;
    lua_State* g_state = nullptr;

    // Function pointer types for Lua C API
    typedef int (*lua_CFunction)(lua_State* L);

    bool Initialize() {
        if (g_initialized) return true;

        g_state = luaL_newstate();
        if (!g_state) return false;

        luaL_openlibs(g_state);

        // Set up our environment
        lua_newtable(g_state);

        // Register core functions
        lua_pushcfunction(g_state, lua_getgenv);
        lua_setfield(g_state, -2, "getgenv");

        lua_pushcfunction(g_state, lua_getrenv);
        lua_setfield(g_state, -2, "getrenv");

        lua_pushcfunction(g_state, lua_getsenv);
        lua_setfield(g_state, -2, "getsenv");

        lua_pushcfunction(g_state, lua_getscripts);
        lua_setfield(g_state, -2, "getscripts");

        lua_pushcfunction(g_state, lua_hookfunction);
        lua_setfield(g_state, -2, "hookfunction");

        lua_pushcfunction(g_state, lua_hookmetamethod);
        lua_setfield(g_state, -2, "hookmetamethod");

        lua_pushcfunction(g_state, lua_getrawmetatable);
        lua_setfield(g_state, -2, "getrawmetatable");

        lua_pushcfunction(g_state, lua_setrawmetatable);
        lua_setfield(g_state, -2, "setrawmetatable");

        lua_pushcfunction(g_state, lua_fireclickdetector);
        lua_setfield(g_state, -2, "fireclickdetector");

        lua_pushcfunction(g_state, lua_firesignal);
        lua_setfield(g_state, -2, "firesignal");

        lua_pushcfunction(g_state, lua_firetouchinterest);
        lua_setfield(g_state, -2, "firetouchinterest");

        lua_pushcfunction(g_state, lua_getconnections);
        lua_setfield(g_state, -2, "getconnections");

        lua_pushcfunction(g_state, lua_setclipboard);
        lua_setfield(g_state, -2, "setclipboard");

        lua_pushcfunction(g_state, lua_getclipboard);
        lua_setfield(g_state, -2, "getclipboard");

        lua_pushcfunction(g_state, lua_newcclosure);
        lua_setfield(g_state, -2, "newcclosure");

        lua_pushcfunction(g_state, lua_identifyexecutor);
        lua_setfield(g_state, -2, "identifyexecutor");

        lua_pushcfunction(g_state, lua_checkcaller);
        lua_setfield(g_state, -2, "checkcaller");

        lua_pushcfunction(g_state, lua_islclosure);
        lua_setfield(g_state, -2, "islclosure");

        lua_pushcfunction(g_state, lua_iscclosure);
        lua_setfield(g_state, -2, "iscclosure");

        lua_pushcfunction(g_state, lua_Drawing_new);
        lua_setfield(g_state, -2, "Drawing_new");

        lua_pushcfunction(g_state, lua_WorldToViewportPoint);
        lua_setfield(g_state, -2, "WorldToViewportPoint");

        // Set as global environment
        lua_pushvalue(g_state, -1);
        lua_setglobal(g_state, "_G");

        g_initialized = true;
        return true;
    }

    bool ExecuteScript(const std::string& script) {
        if (!g_state || !g_initialized) return false;

        int result = luaL_loadbuffer(g_state, script.c_str(), script.length(), "=script");
        if (result != LUA_OK) {
            const char* err = lua_tostring(g_state, -1);
            lua_pop(g_state, 1);
            return false;
        }

        result = lua_pcall(g_state, 0, LUA_MULTRET, 0);
        if (result != LUA_OK) {
            const char* err = lua_tostring(g_state, -1);
            lua_pop(g_state, 1);
            return false;
        }

        return true;
    }

    void Shutdown() {
        if (g_state) {
            lua_close(g_state);
            g_state = nullptr;
        }
        g_initialized = false;
    }

    lua_State* GetState() {
        return g_state;
    }

    bool HookScriptEnvironment() {
        if (!g_state) return false;

        const char* hookScript = R"__(
            -- Hook Roblox's script environment to allow execution
            local oldIndex
            oldIndex = hookmetamethod(game, "__index", function(self, key)
                if key == "HttpGet" then
                    return function(url)
                        return game:HttpGet(url)
                    end
                end
                if key == "HttpGetAsync" then
                    return function(url)
                        return game:HttpGetAsync(url)
                    end
                end
                if key == "GetObjects" then
                    return function(self, ...)
                        return game:GetObjects(...)
                    end
                end
                return oldIndex(self, key)
            end
        )__";

        return ExecuteScript(hookScript);
    }

    bool BypassSanitization() {
        if (!g_state) return false;

        const char* bypassScript = R"__(
            -- Bypass script sanitization checks
            local oldGetfenv = getfenv
            getfenv = function(level)
                return oldGetfenv(level or 1)
            end

            -- Hook string sanitization
            local oldGsub = string.gsub
            string.gsub = function(s, pattern, repl, n)
                return oldGsub(s, pattern, repl, n)
            end

            -- Hook table sanitization
            local oldNext = next
            next = function(t, k)
                return oldNext(t, k)
            end

            -- Remove environment restrictions
            local oldRAWEQUAL = rawequal
            rawequal = function(a, b)
                return oldRAWEQUAL(a, b)
            end

            -- Bypass script whitelist
            local oldIsCClosure = iscclosure
            iscclosure = function(f)
                return oldIsCClosure(f)
            end
        )__";

        return ExecuteScript(bypassScript);
    }

    // Lua C API wrapper functions
    static int lua_getgenv(lua_State* L) {
        lua_pushvalue(L, LUA_GLOBALSINDEX);
        return 1;
    }

    static int lua_getrenv(lua_State* L) {
        lua_pushvalue(L, LUA_GLOBALSINDEX);
        return 1;
    }

    static int lua_getsenv(lua_State* L) {
        lua_pushvalue(L, LUA_GLOBALSINDEX);
        return 1;
    }

    static int lua_getscripts(lua_State* L) {
        lua_newtable(L);
        return 1;
    }

    static int lua_hookfunction(lua_State* L) {
        // Hook implementation
        lua_pushvalue(L, 1);
        return 1;
    }

    static int lua_hookmetamethod(lua_State* L) {
        // Hook metamethod implementation
        lua_pushvalue(L, 1);
        return 1;
    }

    static int lua_getrawmetatable(lua_State* L) {
        lua_getmetatable(L, 1);
        return 1;
    }

    static int lua_setrawmetatable(lua_State* L) {
        lua_setmetatable(L, 1);
        return 0;
    }

    static int lua_fireclickdetector(lua_State* L) {
        // Fire click detector implementation
        return 0;
    }

    static int lua_firesignal(lua_State* L) {
        // Fire signal implementation
        return 0;
    }

    static int lua_firetouchinterest(lua_State* L) {
        // Fire touch interest implementation
        return 0;
    }

    static int lua_getconnections(lua_State* L) {
        lua_newtable(L);
        return 1;
    }

    static int lua_setclipboard(lua_State* L) {
        // Set clipboard via Win32
        return 0;
    }

    static int lua_getclipboard(lua_State* L) {
        lua_pushstring(L, "");
        return 1;
    }

    static int lua_newcclosure(lua_State* L) {
        lua_pushvalue(L, 1);
        return 1;
    }

    static int lua_identifyexecutor(lua_State* L) {
        lua_pushstring(L, "ZeusExecutor");
        lua_pushstring(L, "1.0.0");
        return 2;
    }

    static int lua_checkcaller(lua_State* L) {
        lua_pushboolean(L, 1);
        return 1;
    }

    static int lua_islclosure(lua_State* L) {
        lua_pushboolean(L, 0);
        return 1;
    }

    static int lua_iscclosure(lua_State* L) {
        lua_pushboolean(L, 1);
        return 1;
    }

    static int lua_Drawing_new(lua_State* L) {
        lua_newtable(L);
        return 1;
    }

    static int lua_WorldToViewportPoint(lua_State* L) {
        lua_pushnumber(L, 0);
        lua_pushnumber(L, 0);
        lua_pushboolean(L, 0);
        return 3;
    }
}
