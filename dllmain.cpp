#include "executor.h"

extern "C" {
    __declspec(dllexport) BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
        switch (fdwReason) {
            case DLL_PROCESS_ATTACH:
                DisableThreadLibraryCalls(hinstDLL);
                Executor::CreateInstance(hinstDLL);
                return TRUE;
            case DLL_PROCESS_DETACH:
                Executor::DestroyInstance();
                return TRUE;
            case DLL_THREAD_ATTACH:
            case DLL_THREAD_DETACH:
                return TRUE;
        }
        return TRUE;
    }

    __declspec(dllexport) void ExecuteLuaScript(const char* script) {
        Executor::GetInstance()->ExecuteScript(script);
    }

    __declspec(dllexport) bool IsExecutorReady() {
        return Executor::GetInstance() && Executor::GetInstance()->IsInjected();
    }

    __declspec(dllexport) void ToggleUI() {
        UI::ToggleVisibility();
    }
}
