#pragma once
#include <Windows.h>
#include <string>
#include <vector>

// Forward declare Lua types to avoid needing full headers
struct lua_State;
typedef int (*lua_CFunction)(struct lua_State* L);

namespace LuaCore {
    bool Initialize();
    void Shutdown();
    bool ExecuteScript(const std::string& script);
    lua_State* GetState();
    bool HookScriptEnvironment();
    bool BypassSanitization();

    extern bool g_initialized;
    extern lua_State* g_state;
}
