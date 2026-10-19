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
    while (m_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
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
