#pragma once
#include <Windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <string>
#include <vector>
#include <mutex>
#include <atomic>
#include <functional>

struct ImFont;
struct ImGuiContext;
struct ImGuiIO;
struct ImGuiStyle;

namespace UI {
    bool Initialize(HMODULE hModule);
    void Shutdown();
    void RenderLoop(std::atomic<bool>& running);

    // Internal
    extern ImGuiContext* g_ctx;
    extern ID3D11Device* g_device;
    extern ID3D11DeviceContext* g_context;
    extern IDXGISwapChain* g_swapChain;

    void ExecuteCurrentScript();
    void ToggleVisibility();

    extern bool g_visible;
    extern bool g_scriptRunning;
    extern std::string g_scriptBuffer;
    extern std::string g_statusMessage;

    // Script library
    extern std::vector<std::pair<std::string, std::string>> g_scripts;

    // Drawing overlay
    extern bool g_showESP;
    extern bool g_showAimbot;
    extern float g_espDistance;
}
