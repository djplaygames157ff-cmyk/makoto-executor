#pragma once
#include <Windows.h>
#include <string>
#include <vector>

extern "C" {
    #include "lua.h"
    #include "lauxlib.h"
    #include "lualib.h"
}

namespace LuaCore {
    bool Initialize();
    void Shutdown();
    bool ExecuteScript(const std::string& script);
    lua_State* GetState();

    // Function hooks for bypass
    bool HookScriptEnvironment();
    bool BypassSanitization();

    // Executor state
    extern bool g_initialized;
    extern lua_State* g_state;
}
