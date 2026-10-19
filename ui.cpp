#include "ui.h"
#include "executor.h"
#include "imgui.h"
#include <thread>
#include <chrono>

// UI stubs for compilation
namespace UI {
    bool g_visible = true;
    bool g_scriptRunning = false;
    std::string g_scriptBuffer = "";
    std::string g_statusMessage = "Ready";
    bool g_showESP = false;
    bool g_showAimbot = false;
    float g_espDistance = 500.0f;

    bool Initialize(HMODULE hModule) { return true; }
    void Shutdown() {}
    void RenderLoop(std::atomic<bool>& running) {
        while (running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    void ToggleVisibility() { g_visible = !g_visible; }
    void ExecuteCurrentScript() {}
}
