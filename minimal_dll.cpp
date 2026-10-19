#include <windows.h>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <queue>
#include <atomic>
#include <chrono>
#include <cstring>
#include <functional>

// ============================================================
// MAKOTO EXECUTOR v2.0 - Full Lua Executor DLL
// Memory scanning + lua_State hooking for real script execution
// ============================================================

// ---- Script execution queue ----
static std::queue<std::string> g_scriptQueue;
static std::mutex g_queueMutex;
static std::atomic<bool> g_running(false);
static std::thread g_workerThread;
static bool g_injected = false;

// ---- Lua state & function pointers ----
typedef void* lua_State;
typedef int (*lua_CFunction)(lua_State* L);
typedef const char* (*lua_tolstring_t)(lua_State* L, int idx, size_t* len);
typedef void (*lua_pushcclosure_t)(lua_State* L, lua_CFunction fn, int n);
typedef void (*lua_getfield_t)(lua_State* L, int idx, const char* k);
typedef void (*lua_setfield_t)(lua_State* L, int idx, const char* k);
typedef void (*lua_settop_t)(lua_State* L, int idx);
typedef int (*lua_pcall_t)(lua_State* L, int nargs, int nresults, int errfunc);
typedef int (*lua_loadbuffer_t)(lua_State* L, const char* buff, size_t sz, const char* name);
typedef int (*lua_error_t)(lua_State* L);

static lua_State* g_robloxState = nullptr;
static lua_loadbuffer_t g_originalLoadbuffer = nullptr;
static lua_pcall_t g_originalPcall = nullptr;
static std::atomic<bool> g_luaFound(false);

// ---- Memory scanning helpers ----
static bool IsValidMemory(void* addr) {
    MEMORY_BASIC_INFORMATION mbi;
    if (VirtualQuery(addr, &mbi, sizeof(mbi)) == 0) return false;
    if (mbi.State != MEM_COMMIT) return false;
    if (mbi.Protect == PAGE_NOACCESS) return false;
    if (mbi.Protect & PAGE_GUARD) return false;
    return true;
}

static bool IsRobloxMemory(void* addr) {
    MEMORY_BASIC_INFORMATION mbi;
    if (VirtualQuery(addr, &mbi, sizeof(mbi)) == 0) return false;
    if (mbi.Type == MEM_IMAGE) return false;
    if (mbi.RegionSize > 0x7FFFFFFF) return false;
    return true;
}

// ---- Pattern scanner ----
static BYTE* FindPattern(BYTE* start, SIZE_T size, const BYTE* pattern, const char* mask) {
    SIZE_T patternLen = strlen(mask);
    for (SIZE_T i = 0; i <= size - patternLen; i++) {
        bool found = true;
        for (SIZE_T j = 0; j < patternLen; j++) {
            if (mask[j] == 'x' && start[i + j] != pattern[j]) {
                found = false;
                break;
            }
        }
        if (found) return &start[i];
    }
    return nullptr;
}

// ---- Section enumeration ----
static void* GetModuleSection(HMODULE hModule, const char* sectionName, SIZE_T* outSize) {
    PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)hModule;
    PIMAGE_NT_HEADERS ntHeaders = (PIMAGE_NT_HEADERS)((BYTE*)hModule + dosHeader->e_lfanew);
    PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(ntHeaders);

    for (WORD i = 0; i < ntHeaders->FileHeader.NumberOfSections; i++, section++) {
        if (memcmp(section->Name, sectionName, 8) == 0) {
            if (outSize) *outSize = section->Misc.VirtualSize;
            return (void*)((BYTE*)hModule + section->VirtualAddress);
        }
    }
    return nullptr;
}

// ---- Find lua_State in Roblox ----
static lua_State* FindLuaState() {
    HMODULE roblox = GetModuleHandleA("WindowsPlayer.dll");
    if (!roblox) roblox = GetModuleHandleA("RobloxPlayerBeta.exe");
    if (!roblox) return nullptr;

    SIZE_T textSize = 0;
    BYTE* textBase = (BYTE*)GetModuleSection(roblox, ".text", &textSize);
    if (!textBase) return nullptr;

    // Search for lua_State pointers - look for common patterns
    // Pattern 1: pointer to heap-allocated lua_State with specific markers
    BYTE* scanStart = textBase;
    BYTE* scanEnd = textBase + textSize;

    for (BYTE* p = scanStart; p < scanEnd - 8; p++) {
        if (!IsValidMemory(p)) continue;

        // Check for pointer to heap memory that looks like lua_State
        void* candidate = *(void**)p;
        if (!candidate || !IsValidMemory(candidate)) continue;
        if (!IsRobloxMemory(candidate)) continue;

        // Check for lua_State magic: first field is usually a pointer to another valid address
        void* field1 = *(void**)candidate;
        if (!field1 || !IsValidMemory(field1)) continue;

        // Check for stack size indicator (often 0x40 or similar)
        int stackSize = *(int*)((BYTE*)candidate + 0x10);
        if (stackSize < 0x20 || stackSize > 0x10000) continue;

        // Check for global table pointer
        void* globalPtr = *(void**)((BYTE*)candidate + 0x20);
        if (!globalPtr || !IsValidMemory(globalPtr)) continue;

        return (lua_State*)candidate;
    }

    return nullptr;
}

// ---- Alternative: Find via Lua C API exports ----
static lua_State* FindLuaStateViaExports() {
    HMODULE roblox = GetModuleHandleA("WindowsPlayer.dll");
    if (!roblox) roblox = GetModuleHandleA("RobloxPlayerBeta.exe");
    if (!roblox) return nullptr;

    // Look for lua_newstate or similar exports
    FARPROC luaNewState = GetProcAddress(roblox, "lua_newstate");
    if (luaNewState) {
        // The lua_State is created internally - we need to hook into it
        // For now, scan for the state created at startup
    }

    return nullptr;
}

// ---- Execute Lua script ----
static bool ExecuteLuaScriptInternal(const std::string& script) {
    if (!g_robloxState || !g_originalLoadbuffer || !g_originalPcall) {
        return false;
    }

    int loadResult = g_originalLoadbuffer(g_robloxState, script.c_str(), script.length(), "=makoto");
    if (loadResult != 0) {
        // Error loading - pop error message
        lua_tolstring_t lua_tolstring = (lua_tolstring_t)GetProcAddress(
            GetModuleHandleA("WindowsPlayer.dll"), "lua_tolstring");
        if (lua_tolstring) {
            const char* err = lua_tolstring(g_robloxState, -1, nullptr);
            if (err) {
                OutputDebugStringA("[MAKOTO] Script error: ");
                OutputDebugStringA(err);
                OutputDebugStringA("\n");
            }
        }

        lua_settop_t lua_settop = (lua_settop_t)GetProcAddress(
            GetModuleHandleA("WindowsPlayer.dll"), "lua_settop");
        if (lua_settop) lua_settop(g_robloxState, -2);
        return false;
    }

    int callResult = g_originalPcall(g_robloxState, 0, -1, 0);
    if (callResult != 0) {
        OutputDebugStringA("[MAKOTO] Script execution failed\n");

        lua_settop_t lua_settop = (lua_settop_t)GetProcAddress(
            GetModuleHandleA("WindowsPlayer.dll"), "lua_settop");
        if (lua_settop) lua_settop(g_robloxState, -2);
        return false;
    }

    return true;
}

// ---- Worker thread: find lua_State and process script queue ----
static void WorkerThread() {
    OutputDebugStringA("[MAKOTO] Worker thread started, scanning for lua_State...\n");

    // Wait for lua_State to become available
    while (g_running && !g_luaFound) {
        g_robloxState = FindLuaState();
        if (g_robloxState) {
            g_luaFound = true;
            OutputDebugStringA("[MAKOTO] lua_State found!\n");

            // Get function pointers
            HMODULE roblox = GetModuleHandleA("WindowsPlayer.dll");
            if (!roblox) roblox = GetModuleHandleA("RobloxPlayerBeta.exe");

            if (roblox) {
                g_originalLoadbuffer = (lua_loadbuffer_t)GetProcAddress(roblox, "luaL_loadbufferx");
                if (!g_originalLoadbuffer) {
                    g_originalLoadbuffer = (lua_loadbuffer_t)GetProcAddress(roblox, "luaL_loadbuffer");
                }
                if (!g_originalLoadbuffer) {
                    g_originalLoadbuffer = (lua_loadbuffer_t)GetProcAddress(roblox, "lua_loadbufferx");
                }

                g_originalPcall = (lua_pcall_t)GetProcAddress(roblox, "lua_pcallk");
                if (!g_originalPcall) {
                    g_originalPcall = (lua_pcall_t)GetProcAddress(roblox, "lua_pcall");
                }

                if (g_originalLoadbuffer && g_originalPcall) {
                    OutputDebugStringA("[MAKOTO] Lua functions resolved!\n");
                } else {
                    OutputDebugStringA("[MAKOTO] Failed to resolve some Lua functions\n");
                }
            }
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    // Process script queue
    while (g_running) {
        std::string script;
        {
            std::lock_guard<std::mutex> lock(g_queueMutex);
            if (!g_scriptQueue.empty()) {
                script = g_scriptQueue.front();
                g_scriptQueue.pop();
            }
        }

        if (!script.empty()) {
            if (g_luaFound && g_originalLoadbuffer && g_originalPcall) {
                ExecuteLuaScriptInternal(script);
            } else {
                OutputDebugStringA("[MAKOTO] Lua not ready, script queued for retry\n");
                std::lock_guard<std::mutex> lock(g_queueMutex);
                g_scriptQueue.push(script);
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}

// ---- Exports ----
extern "C" {

__declspec(dllexport) void ExecuteLuaScript(const char* script) {
    if (script) {
        std::lock_guard<std::mutex> lock(g_queueMutex);
        g_scriptQueue.push(std::string(script));
    }
}

__declspec(dllexport) bool IsExecutorReady() {
    return g_injected;
}

__declspec(dllexport) bool IsLuaReady() {
    return g_luaFound.load();
}

__declspec(dllexport) void ToggleUI() {
    // UI toggle placeholder
}

__declspec(dllexport) int GetQueueSize() {
    std::lock_guard<std::mutex> lock(g_queueMutex);
    return (int)g_scriptQueue.size();
}

__declspec(dllexport) bool IsRunning() {
    return g_running.load();
}

__declspec(dllexport) const char* GetMakotoVersion() {
    return "MAKOTO EXECUTOR v2.0";
}

__declspec(dllexport) const char* GetExecutorName() {
    return "MakotoExecutor";
}

__declspec(dllexport) void Shutdown() {
    g_running = false;
    if (g_workerThread.joinable()) {
        g_workerThread.join();
    }
}

}

// ---- Stealth thread creation ----
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

#define THREAD_CREATE_FLAGS_HIDE_FROM_DEBUGGER 0x00000004

static HANDLE StealthCreateThread(HANDLE process, LPTHREAD_START_ROUTINE start, LPVOID param) {
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

// ---- Evasion: Disable ETW ----
static bool DisableETW() {
    HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    if (!ntdll) return false;

    FARPROC etwEventWrite = GetProcAddress(ntdll, "EtwEventWrite");
    if (!etwEventWrite) return false;

    DWORD oldProtect;
    VirtualProtect((LPVOID)etwEventWrite, 1, PAGE_EXECUTE_READWRITE, &oldProtect);
    *(BYTE*)etwEventWrite = 0xC3; // ret
    VirtualProtect((LPVOID)etwEventWrite, 1, oldProtect, &oldProtect);

    return true;
}

// ---- Evasion: Disable AMSI ----
static bool DisableAMSI() {
    HMODULE amsi = LoadLibraryW(L"amsi.dll");
    if (!amsi) return false;

    FARPROC amsiScan = GetProcAddress(amsi, "AmsiScanBuffer");
    if (!amsiScan) return false;

    DWORD oldProtect;
    VirtualProtect((LPVOID)amsiScan, 1, PAGE_EXECUTE_READWRITE, &oldProtect);
    *(BYTE*)amsiScan = 0xC3; // ret
    VirtualProtect((LPVOID)amsiScan, 1, oldProtect, &oldProtect);

    return true;
}

// ---- Evasion: Hide from debugger ----
static bool HideFromDebugger() {
    // PEB BeingDebugged flag
    #ifdef _WIN64
    BYTE* peb = (BYTE*)__readgsqword(0x60);
    *(peb + 0x02) = 0; // BeingDebugged = false
    #else
    BYTE* peb = (BYTE*)__readfsdword(0x30);
    *(peb + 0x02) = 0;
    #endif

    // NtQueryInformationProcess hook would go here
    return true;
}

// ---- DLL Entry Point ----
BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved) {
    switch (reason) {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hModule);
            DisableETW();
            DisableAMSI();
            HideFromDebugger();
            g_injected = true;
            g_running = true;
            g_workerThread = std::thread(WorkerThread);
            OutputDebugStringA("[MAKOTO] DLL attached, worker thread started\n");
            return TRUE;

        case DLL_PROCESS_DETACH:
            g_running = false;
            if (g_workerThread.joinable()) {
                g_workerThread.join();
            }
            OutputDebugStringA("[MAKOTO] DLL detached\n");
            return TRUE;

        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
            return TRUE;
    }
    return TRUE;
}
