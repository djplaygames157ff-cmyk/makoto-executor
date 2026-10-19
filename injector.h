#pragma once
#include <Windows.h>
#include <tlhelp32.h>
#include <string>
#include <vector>

namespace Injector {
    bool Initialize();
    HANDLE GetRobloxProcess();
    bool InjectDLL(HANDLE process, HMODULE module);
    void Cleanup();

    // Internal helpers
    DWORD FindProcessByName(const std::wstring& name);
    bool ManualMapDLL(HANDLE process, const std::string& dllPath);
    bool LoadLibraryInject(HANDLE process, const std::string& dllPath);

    // Thread creation with stolen thread (hijack)
    HANDLE StealthCreateThread(HANDLE process, LPTHREAD_START_ROUTINE start, LPVOID param);

    // Export table
    struct ExportEntry {
        std::string name;
        DWORD ordinal;
        DWORD rva;
    };

    extern std::vector<ExportEntry> g_exports;
}
