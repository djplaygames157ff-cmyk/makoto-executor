#pragma once
#include <Windows.h>
#include <atomic>
#include <mutex>
#include <queue>
#include <thread>
#include <memory>
#include "evasion.h"
#include "injector.h"

class Executor {
public:
    static Executor* CreateInstance(HMODULE hModule) {
        if (!s_instance) {
            s_instance = new Executor();
        }
        return s_instance;
    }

    static Executor* GetInstance() { return s_instance; }
    static void DestroyInstance() {
        if (s_instance) {
            delete s_instance;
            s_instance = nullptr;
        }
    }

    bool Initialize(HMODULE hModule);
    void ExecuteScript(const std::string& script);
    void Shutdown();
    bool IsInjected() const { return m_injected; }

private:
    Executor();
    ~Executor();

    void ExecutionLoop();
    void UIThread();
    void InitializeDefaultEnvironments();

    static Executor* s_instance;

    HMODULE m_module;
    HANDLE m_targetProcess;
    bool m_injected;
    bool m_running;

    std::unique_ptr<EvasionManager> m_evasion;

    std::thread m_executionThread;
    std::thread m_uiThread;

    std::queue<std::string> m_scriptQueue;
    std::mutex m_scriptQueueMutex;
};
