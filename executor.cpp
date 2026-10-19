#include "executor.h"
#include "lua_core.h"
#include "ui.h"
#include "evasion.h"
#include "injector.h"
#include <thread>
#include <chrono>
#include <fstream>
#include <sstream>

Executor* Executor::s_instance = nullptr;

Executor::Executor() : m_targetProcess(nullptr), m_injected(false) {
    s_instance = this;
}

Executor::~Executor() {
    Shutdown();
    s_instance = nullptr;
}

bool Executor::Initialize(HMODULE hModule) {
    m_module = hModule;
    m_evasion = std::make_unique<EvasionManager>();
    m_evasion->Initialize();

    if (!m_evasion->PerformChecks()) {
        return false;
    }

    if (!Injector::Initialize()) {
        return false;
    }

    m_targetProcess = Injector::GetRobloxProcess();
    if (!m_targetProcess) {
        return false;
    }

    if (!Injector::InjectDLL(m_targetProcess, m_module)) {
        return false;
    }

    m_injected = true;

    if (!LuaCore::Initialize()) {
        return false;
    }

    LuaCore::HookScriptEnvironment();
    LuaCore::BypassSanitization();

    InitializeDefaultEnvironments();

    m_running = true;
    m_executionThread = std::thread(&Executor::ExecutionLoop, this);
    m_uiThread = std::thread(&Executor::UIThread, this);

    return true;
}

void Executor::ExecuteScript(const std::string& script) {
    if (!m_injected || !LuaCore::g_state) {
        return;
    }

    std::lock_guard<std::mutex> lock(m_scriptQueueMutex);
    m_scriptQueue.push(script);
}

void Executor::ExecutionLoop() {
    while (m_running) {
        std::string script;
        {
            std::lock_guard<std::mutex> lock(m_scriptQueueMutex);
            if (!m_scriptQueue.empty()) {
                script = m_scriptQueue.front();
                m_scriptQueue.pop();
            }
        }

        if (!script.empty()) {
            LuaCore::ExecuteScript(script);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void Executor::UIThread() {
    UI::Initialize(m_module);
    UI::RenderLoop(m_running);
    UI::Shutdown();
}

void Executor::InitializeDefaultEnvironments() {
    const char* getgenvScript = R"__(
        getgenv = function()
            return _G
        end

        getrenv = function()
            return getfenv(0)
        end

        getscripts = function()
            local scripts = {}
            for _, v in getgc(true) do
                if type(v) == "table" then
                    for _, s in pairs(v) do
                        if type(s) == "function" and iscclosure(s) then
                            table.insert(scripts, s)
                        end
                    end
                end
            end
            return scripts
        end

        getsenv = function(script)
            for _, env in getgc(true) do
                if type(env) == "table" and env.script == script then
                    return env
                end
            end
            return getfenv()
        end

        getcallingscript = function()
            local i = 2
            while true do
                local info = debug.getinfo(i, "S")
                if not info then break end
                if info.what == "Lua" then
                    return debug.getinfo(i, "S").source
                end
                i = i + 1
            end
            return nil
        end

        gethui = function()
            return game:GetService("CoreGui")
        end

        get_hidden_gui = gethui

        makefolder = function(path)
            if not io and not readfile then return false end
            local parts = {}
            for part in path:gmatch("[^/]+") do
                table.insert(parts, part)
            end
            local current = ""
            for i, part in ipairs(parts) do
                if i > 1 then
                    current = current .. "/" .. part
                else
                    current = part
                end
                if not isfolder(current) then
                    -- folder creation attempt
                end
            end
            return true
        end

        writefile = function(path, content)
            -- file write placeholder
            return true
        end

        readfile = function(path)
            -- file read placeholder
            return ""
        end

        isfolder = function(path)
            return false
        end

        listfiles = function(path)
            return {}
        end

        hookfunction = function(target, hook)
            if type(target) ~= "function" then
                return function() end
            end
            local old = target
            return function(...)
                return hook(old, ...)
            end
        end

        hookmetamethod = function(obj, method, hook)
            if type(obj) ~= "table" then
                return function() end
            end
            local mt = getrawmetatable(obj)
            if not mt then return function() end end
            local old = mt[method]
            return function(...)
                return hook(old, ...)
            end
        end

        getrawmetatable = function(obj)
            if type(obj) == "table" then
                return getmetatable(obj)
            end
            return nil
        end

        setrawmetatable = function(obj, mt)
            return setmetatable(obj, mt)
        end

        setreadonly = function(tbl, readonly)
            return tbl
        end

        cleardrawcache = function() end

        identifyexecutor = function()
            return "ZeusExecutor", "1.0.0"
        end

        -- Anti-detection wrappers
        checkcaller = function()
            return true
        end

        islclosure = function(f)
            return true
        end

        iscclosure = function(f)
            return not is_lclosure_impl(f)
        end

        is_lclosure_impl = function(f)
            local info = debug.getinfo(f, "S")
            return info and info.what == "Lua"
        end

        newcclosure = function(f)
            return function(...) return f(...) end
        end

        fireclickdetector = function(detector, distance)
            if distance then
                detector:WaitForChild("MaxActivationDistance")
            end
            detector:Activate()
        end

        firesignal = function(signal, ...)
            signal:Fire(...)
        end

        firetouchinterest = function(part, transmitter, interest)
            -- touch interest simulation
        end

        getconnections = function(signal)
            local conns = {}
            -- enumerate connections
            return conns
        end

        firesignal = function(signal, ...)
            if signal and signal.Fire then
                signal:Fire(...)
            end
        end

        setclipboard = function(text)
            -- clipboard set via Win32
        end

        getclipboard = function()
            return ""
        end

        -- Drawing library stub
        Drawing = {}
        Drawing.new = function(classType)
            return {
                Visible = false,
                Remove = function(self) self.Removed = true end,
                Destroy = function(self) self.Removed = true end,
                Transparency = 1,
                Color = Color3.new(1, 1, 1),
                Thickness = 1,
                Position = Vector2.new(0, 0),
                Size = Vector2.new(0, 0),
                Text = "",
                Font = 0,
                Center = false
            }
        end
        Drawing.fonts = {
            UI = 0,
            System = 1,
            Plex = 2,
            Monospace = 3
        }

        -- ESP and utility functions
        WorldToViewportPoint = function(point)
            return Vector2.new(0, 0), false
        end

        -- Script hub ready
        print("[Zeus] Environment loaded. Executor ready.")
    )__";

    LuaCore::ExecuteScript(getgenvScript);
}

void Executor::Shutdown() {
    m_running = false;
    if (m_executionThread.joinable()) {
        m_executionThread.join();
    }
    if (m_uiThread.joinable()) {
        m_uiThread.join();
    }
    LuaCore::Shutdown();
    if (m_injected) {
        Injector::Cleanup();
    }
}

// DLL Entry point
DWORD WINAPI DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved) {
    switch (reason) {
        case DLL_PROCESS_ATTACH: {
            DisableThreadLibraryCalls(hModule);
            auto executor = new Executor();
            if (executor->Initialize(hModule)) {
                return TRUE;
            }
            delete executor;
            return FALSE;
        }
        case DLL_PROCESS_DETACH: {
            if (Executor::s_instance) {
                delete Executor::s_instance;
            }
            return TRUE;
        }
    }
    return TRUE;
}
