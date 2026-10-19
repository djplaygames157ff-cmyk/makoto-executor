#include "injector.h"
#include <winternl.h>

#define THREAD_CREATE_FLAGS_HIDE_FROM_DEBUGGER 0x00000004

typedef NTSTATUS(NTAPI* NtCreateThreadEx_t)(
    PHANDLE ThreadHandle,
    ACCESS_MASK DesiredAccess,
    PVOID ObjectAttributes,
    HANDLE ProcessHandle,
    LPTHREAD_START_ROUTINE StartAddress,
    PVOID Parameter,
    ULONG CreateFlags,
    SIZE_T ZeroBits,
    SIZE_T StackSize,
    SIZE_T MaximumStackSize,
    PVOID AttributeList
);

bool Injector::Initialize() {
    return true;
}

HANDLE Injector::GetRobloxProcess() {
    DWORD pid = 0;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return nullptr;

    PROCESSENTRY32W pe = { sizeof(pe) };
    if (Process32FirstW(snapshot, &pe)) {
        do {
            if (_wcsicmp(pe.szExeFile, L"WindowsPlayer.exe") == 0 ||
                _wcsicmp(pe.szExeFile, L"RobloxPlayerBeta.exe") == 0) {
                pid = pe.th32ProcessID;
                break;
            }
        } while (Process32NextW(snapshot, &pe));
    }
    CloseHandle(snapshot);

    if (pid == 0) return nullptr;

    HANDLE process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    return process;
}

bool Injector::InjectDLL(HANDLE process, HMODULE module) {
    char path[MAX_PATH];
    if (GetModuleFileNameA(module, path, MAX_PATH) == 0)
        return false;

    return LoadLibraryInject(process, path);
}

bool Injector::LoadLibraryInject(HANDLE process, const std::string& dllPath) {
    HMODULE kernel32 = GetModuleHandleW(L"kernel32.dll");
    if (!kernel32) return false;

    FARPROC loadLibrary = GetProcAddress(kernel32, "LoadLibraryA");
    if (!loadLibrary) return false;

    size_t pathLen = dllPath.length() + 1;
    LPVOID remotePath = VirtualAllocEx(process, nullptr, pathLen,
                                        MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remotePath) return false;

    if (!WriteProcessMemory(process, remotePath, dllPath.c_str(), pathLen, nullptr)) {
        VirtualFreeEx(process, remotePath, 0, MEM_RELEASE);
        return false;
    }

    HANDLE thread = CreateRemoteThread(process, nullptr, 0,
                                        (LPTHREAD_START_ROUTINE)loadLibrary,
                                        remotePath, 0, nullptr);
    if (!thread) {
        VirtualFreeEx(process, remotePath, 0, MEM_RELEASE);
        return false;
    }

    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);
    VirtualFreeEx(process, remotePath, 0, MEM_RELEASE);
    return true;
}

HANDLE Injector::StealthCreateThread(HANDLE process, LPTHREAD_START_ROUTINE start, LPVOID param) {
    FARPROC ntCreateThreadEx = GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "NtCreateThreadEx");
    if (!ntCreateThreadEx) return nullptr;

    NtCreateThreadEx_t NtCreateThreadEx = (NtCreateThreadEx_t)ntCreateThreadEx;

    HANDLE thread = nullptr;
    NTSTATUS status = NtCreateThreadEx(
        &thread,
        THREAD_ALL_ACCESS,
        nullptr,
        process,
        start,
        param,
        THREAD_CREATE_FLAGS_HIDE_FROM_DEBUGGER,
        0, 0, 0, nullptr
    );

    if (status != 0) return nullptr;
    return thread;
}

void Injector::Cleanup() {
    g_exports.clear();
}
