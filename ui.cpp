#include "ui.h"
#include "executor.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <thread>
#include <chrono>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace UI {
    ImGuiContext* g_ctx = nullptr;
    ID3D11Device* g_device = nullptr;
    ID3D11DeviceContext* g_context = nullptr;
    IDXGISwapChain* g_swapChain = nullptr;
    bool g_visible = true;
    bool g_scriptRunning = false;
    std::string g_scriptBuffer = "-- Enter Lua script here...";
    std::string g_statusMessage = "Ready";
    bool g_showESP = false;
    bool g_showAimbot = false;
    float g_espDistance = 500.0f;

    std::vector<std::pair<std::string, std::string>> g_scripts = {
        {"Infinite Yield", "loadstring(game:HttpGet('https://raw.githubusercontent.com/EdgeIY/infiniteyield/master/source'))()"},
        {"Simple ESP", "loadstring(game:HttpGet('https://example.com/esp.lua'))()"},
        {"Speed Boost", "local p = game.Players.LocalPlayer; local h = p.Character and p.Character:FindFirstChild('Humanoid'); if h then h.WalkSpeed = 32 end"}
    };

    static HWND g_hwnd = nullptr;
    static WNDCLASSEX wc = {};
    static bool g_initialized = false;
    static bool g_running = false;
    static std::mutex g_renderMutex;
    static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

    // MAKOTO Theme Colors
    static ImVec4 MAKOTO_RED = ImVec4(0.90f, 0.15f, 0.15f, 1.0f);
    static ImVec4 MAKOTO_DARK_RED = ImVec4(0.65f, 0.05f, 0.05f, 1.0f);
    static ImVec4 MAKOTO_BLACK = ImVec4(0.05f, 0.05f, 0.05f, 1.0f);
    static ImVec4 MAKOTO_DARK = ImVec4(0.08f, 0.08f, 0.08f, 1.0f);
    static ImVec4 MAKOTO_LIGHT = ImVec4(0.12f, 0.12f, 0.12f, 1.0f);
    static ImVec4 MAKOTO_WHITE = ImVec4(0.95f, 0.95f, 0.95f, 1.0f);
    static ImVec4 MAKOTO_GRAY = ImVec4(0.4f, 0.4f, 0.4f, 1.0f);

    static LRESULT WINAPI WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        if (g_ctx && ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
            return true;

        switch (msg) {
            case WM_SIZE:
                if (g_device != nullptr && wParam != SIZE_MINIMIZED) {
                    g_swapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
                }
                return 0;
            case WM_SYSCOMMAND:
                if ((wParam & 0xfff0) == SC_KEYMENU)
                    return 0;
                break;
            case WM_DESTROY:
                g_running = false;
                return 0;
        }
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    static void CleanupRenderTarget() {
        if (g_mainRenderTargetView) {
            g_mainRenderTargetView->Release();
            g_mainRenderTargetView = nullptr;
        }
    }

    static void CreateRenderTarget() {
        ID3D11Texture2D* pBackBuffer = nullptr;
        g_swapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
        if (pBackBuffer) {
            g_device->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
            pBackBuffer->Release();
        }
    }

    static bool CreateDeviceD3D(HWND hwnd) {
        DXGI_SWAP_CHAIN_DESC sd;
        ZeroMemory(&sd, sizeof(sd));
        sd.BufferCount = 2;
        sd.BufferDesc.Width = 0;
        sd.BufferDesc.Height = 0;
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferDesc.RefreshRate.Numerator = 60;
        sd.BufferDesc.RefreshRate.Denominator = 1;
        sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow = hwnd;
        sd.SampleDesc.Count = 1;
        sd.SampleDesc.Quality = 0;
        sd.Windowed = TRUE;
        sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

        D3D_FEATURE_LEVEL featureLevel;
        const D3D_FEATURE_LEVEL featureLevelArray[2] = {
            D3D_FEATURE_LEVEL_11_0,
            D3D_FEATURE_LEVEL_10_0,
        };

        HRESULT res = D3D11CreateDeviceAndSwapChain(
            nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
            featureLevelArray, 2, D3D11_SDK_VERSION,
            &sd, &g_swapChain, &g_device, &featureLevel, &g_context
        );

        if (res == DXGI_ERROR_UNSUPPORTED) {
            res = D3D11CreateDeviceAndSwapChain(
                nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0,
                featureLevelArray, 2, D3D11_SDK_VERSION,
                &sd, &g_swapChain, &g_device, &featureLevel, &g_context
            );
        }

        if (res != S_OK) return false;

        CreateRenderTarget();
        return true;
    }

    static void CleanupDeviceD3D() {
        CleanupRenderTarget();
        if (g_swapChain) { g_swapChain->Release(); g_swapChain = nullptr; }
        if (g_context) { g_context->Release(); g_context = nullptr; }
        if (g_device) { g_device->Release(); g_device = nullptr; }
    }

    bool Initialize(HMODULE hModule) {
        if (g_initialized) return true;

        wc.cbSize = sizeof(WNDCLASSEX);
        wc.style = CS_CLASSDC;
        wc.lpfnWndProc = WndProc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        wc.hInstance = hModule;
        wc.hIcon = nullptr;
        wc.hCursor = nullptr;
        wc.hbrBackground = nullptr;
        wc.lpszMenuName = nullptr;
        wc.lpszClassName = L"MakotoExecutor";
        wc.hIconSm = nullptr;

        if (!RegisterClassExW(&wc)) return false;

        g_hwnd = CreateWindowExW(
            WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW,
            wc.lpszClassName, L"MAKOTO EXECUTOR",
            WS_POPUP | WS_VISIBLE,
            0, 0, 900, 650,
            nullptr, nullptr, hModule, nullptr
        );

        if (!g_hwnd) return false;

        if (!CreateDeviceD3D(g_hwnd)) {
            CleanupDeviceD3D();
            return false;
        }

        ImGui::CreateContext();
        g_ctx = ImGui::GetCurrentContext();

        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

        // MAKOTO THEME
        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowRounding = 10.0f;
        style.FrameRounding = 6.0f;
        style.PopupRounding = 6.0f;
        style.ScrollbarRounding = 6.0f;
        style.GrabRounding = 6.0f;
        style.TabRounding = 6.0f;
        style.ChildRounding = 8.0f;
        style.WindowBorderSize = 1.0f;
        style.FrameBorderSize = 1.0f;
        style.PopupBorderSize = 1.0f;
        style.TabBorderSize = 1.0f;

        // Window
        style.Colors[ImGuiCol_WindowBg] = MAKOTO_BLACK;
        style.Colors[ImGuiCol_ChildBg] = MAKOTO_DARK;
        style.Colors[ImGuiCol_PopupBg] = MAKOTO_DARK;

        // Text
        style.Colors[ImGuiCol_Text] = MAKOTO_WHITE;
        style.Colors[ImGuiCol_TextDisabled] = MAKOTO_GRAY;

        // Frame (buttons, inputs, etc)
        style.Colors[ImGuiCol_FrameBg] = MAKOTO_LIGHT;
        style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.18f, 0.18f, 0.18f, 1.0f);
        style.Colors[ImGuiCol_FrameBgActive] = MAKOTO_RED;

        // Buttons
        style.Colors[ImGuiCol_Button] = MAKOTO_RED;
        style.Colors[ImGuiCol_ButtonHovered] = ImVec4(1.0f, 0.25f, 0.25f, 1.0f);
        style.Colors[ImGuiCol_ButtonActive] = MAKOTO_DARK_RED;

        // Header
        style.Colors[ImGuiCol_Header] = MAKOTO_DARK_RED;
        style.Colors[ImGuiCol_HeaderHovered] = MAKOTO_RED;
        style.Colors[ImGuiCol_HeaderActive] = MAKOTO_RED;

        // Title bar
        style.Colors[ImGuiCol_TitleBg] = MAKOTO_DARK_RED;
        style.Colors[ImGuiCol_TitleBgActive] = MAKOTO_RED;
        style.Colors[ImGuiCol_TitleBgCollapsed] = MAKOTO_DARK;

        // Tabs
        style.Colors[ImGuiCol_Tab] = MAKOTO_DARK;
        style.Colors[ImGuiCol_TabHovered] = MAKOTO_RED;
        style.Colors[ImGuiCol_TabActive] = MAKOTO_RED;
        style.Colors[ImGuiCol_TabUnfocused] = MAKOTO_DARK;
        style.Colors[ImGuiCol_TabUnfocusedActive] = MAKOTO_DARK_RED;

        // Slider
        style.Colors[ImGuiCol_SliderGrab] = MAKOTO_RED;
        style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(1.0f, 0.2f, 0.2f, 1.0f);

        // Check mark
        style.Colors[ImGuiCol_CheckMark] = MAKOTO_RED;

        // Scrollbar
        style.Colors[ImGuiCol_ScrollbarBg] = MAKOTO_BLACK;
        style.Colors[ImGuiCol_ScrollbarGrab] = MAKOTO_DARK_RED;
        style.Colors[ImGuiCol_ScrollbarGrabHovered] = MAKOTO_RED;
        style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(1.0f, 0.2f, 0.2f, 1.0f);

        // Border
        style.Colors[ImGuiCol_Border] = MAKOTO_DARK_RED;
        style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

        // Separator
        style.Colors[ImGuiCol_Separator] = MAKOTO_DARK_RED;
        style.Colors[ImGuiCol_SeparatorHovered] = MAKOTO_RED;
        style.Colors[ImGuiCol_SeparatorActive] = MAKOTO_RED;

        // Resize grip
        style.Colors[ImGuiCol_ResizeGrip] = MAKOTO_DARK_RED;
        style.Colors[ImGuiCol_ResizeGripHovered] = MAKOTO_RED;
        style.Colors[ImGuiCol_ResizeGripActive] = MAKOTO_RED;

        // Plot
        style.Colors[ImGuiCol_PlotHistogram] = MAKOTO_RED;
        style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.0f, 0.3f, 0.3f, 1.0f);

        // Text selection
        style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.65f, 0.05f, 0.05f, 0.5f);

        // Navigation
        style.Colors[ImGuiCol_NavHighlight] = MAKOTO_RED;
        style.Colors[ImGuiCol_NavWindowingHighlight] = MAKOTO_RED;
        style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.5f);

        // Modal window dimming
        style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.6f);

        // Docking
        style.Colors[ImGuiCol_DockingPreview] = MAKOTO_RED;
        style.Colors[ImGuiCol_DockingEmptyBg] = MAKOTO_DARK;

        // Window shadow
        style.WindowShadowSize = 0.0f;

        ImGui_ImplWin32_Init(g_hwnd);
        ImGui_ImplDX11_Init(g_device, g_context);

        SetLayeredWindowAttributes(g_hwnd, RGB(0, 0, 0), 255, LWA_ALPHA);
        ShowWindow(g_hwnd, SW_SHOW);
        UpdateWindow(g_hwnd);

        g_initialized = true;
        g_running = true;
        return true;
    }

    void Shutdown() {
        if (!g_initialized) return;

        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();

        CleanupDeviceD3D();
        DestroyWindow(g_hwnd);
        UnregisterClassW(wc.lpszClassName, wc.hInstance);
        g_initialized = false;
    }

    void ToggleVisibility() {
        g_visible = !g_visible;
        if (g_hwnd) {
            ShowWindow(g_hwnd, g_visible ? SW_SHOW : SW_HIDE);
        }
    }

    void ExecuteCurrentScript() {
        if (g_scriptRunning) return;
        g_scriptRunning = true;
        g_statusMessage = "Executing...";

        std::string script = g_scriptBuffer;
        std::thread([script]() {
            // Get executor instance and execute
            // This would call into the executor
            g_statusMessage = "Executed";
            g_scriptRunning = false;
        }).detach();
    }

    void RenderLoop(std::atomic<bool>& running) {
        while (running && g_running) {
            MSG msg;
            while (PeekMessageW(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }

            if (!g_running) break;

            ImGui_ImplDX11_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();

            if (g_visible) {
                ImGui::SetNextWindowSize(ImVec2(900, 650), ImGuiCond_FirstUseEver);
                ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_FirstUseEver);

                // Custom window flags for clean look
                ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoCollapse;

                ImGui::Begin("MAKOTO EXECUTOR", &g_visible, windowFlags);

                // Header with logo
                ImVec2 windowSize = ImGui::GetWindowSize();
                float headerHeight = 50.0f;

                // Background gradient effect
                ImDrawList* drawList = ImGui::GetWindowDrawList();
                ImVec2 p0 = ImGui::GetWindowPos();
                ImVec2 p1 = ImVec2(p0.x + windowSize.x, p0.y + headerHeight);

                // Red gradient header
                drawList->AddRectFilledMultiColor(
                    p0, p1,
                    ImGui::ColorConvertFloat4ToU32(MAKOTO_RED),
                    ImGui::ColorConvertFloat4ToU32(MAKOTO_RED),
                    ImGui::ColorConvertFloat4ToU32(MAKOTO_DARK_RED),
                    ImGui::ColorConvertFloat4ToU32(MAKOTO_DARK_RED)
                );

                // Logo text
                ImGui::SetCursorPos(ImVec2(15, 10));
                ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
                ImGui::TextColored(MAKOTO_WHITE, "MAKOTO EXECUTOR");
                ImGui::PopFont();

                // Version tag
                ImGui::SameLine(windowSize.x - 80);
                ImGui::TextColored(ImVec4(1, 1, 1, 0.7f), "v2.0");

                ImGui::SetCursorPosY(headerHeight + 10);

                // Status bar
                ImVec4 statusColor = g_scriptRunning ?
                    ImVec4(1.0f, 0.8f, 0.2f, 1.0f) :
                    ImVec4(0.2f, 0.9f, 0.3f, 1.0f);

                ImGui::PushStyleColor(ImGuiCol_FrameBg, MAKOTO_LIGHT);
                ImGui::PushStyleColor(ImGuiCol_Text, statusColor);
                ImGui::Text("  %s", g_statusMessage.c_str());
                ImGui::PopStyleColor(2);

                ImGui::Separator();
                ImGui::Spacing();

                // Tab bar
                ImGuiTabBarFlags tabFlags = ImGuiTabBarFlags_None;
                if (ImGui::BeginTabBar("MakotoTabs", tabFlags)) {

                    // Editor tab
                    if (ImGui::BeginTabItem("  Editor  ")) {
                        ImGui::Spacing();

                        // Script input area
                        ImGui::PushStyleColor(ImGuiCol_FrameBg, MAKOTO_BLACK);
                        ImGui::PushStyleColor(ImGuiCol_Text, MAKOTO_WHITE);
                        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 6));

                        ImGui::InputTextMultiline("##script_editor", &g_scriptBuffer,
                            ImVec2(-1, 220),
                            ImGuiInputTextFlags_AllowTabInput);

                        ImGui::PopStyleVar();
                        ImGui::PopStyleColor(2);

                        ImGui::Spacing();

                        // Action buttons
                        ImVec2 btnSize = ImVec2(120, 35);

                        if (ImGui::Button("  Execute  ", btnSize) && !g_scriptRunning) {
                            ExecuteCurrentScript();
                        }
                        ImGui::SameLine();

                        if (ImGui::Button("  Clear  ", btnSize)) {
                            g_scriptBuffer = "";
                        }
                        ImGui::SameLine();

                        if (ImGui::Button("  Load Example  ", btnSize)) {
                            g_scriptBuffer = "print('Hello from MAKOTO!')";
                        }

                        ImGui::EndTabItem();
                    }

                    // Library tab
                    if (ImGui::BeginTabItem("  Library  ")) {
                        ImGui::Spacing();
                        ImGui::TextColored(MAKOTO_RED, "Quick Scripts");
                        ImGui::Separator();

                        for (auto& script : g_scripts) {
                            if (ImGui::Button(script.first.c_str(), ImVec2(200, 30))) {
                                g_scriptBuffer = script.second;
                                ExecuteCurrentScript();
                            }
                            ImGui::SameLine();
                            ImGui::TextColored(MAKOTO_GRAY, "Execute this script");
                            ImGui::Spacing();
                        }

                        ImGui::EndTabItem();
                    }

                    // Settings tab
                    if (ImGui::BeginTabItem("  Settings  ")) {
                        ImGui::Spacing();

                        ImGui::TextColored(MAKOTO_RED, "Visual Settings");
                        ImGui::Separator();

                        ImGui::Checkbox("Show ESP", &g_showESP);
                        ImGui::Checkbox("Show Aimbot", &g_showAimbot);

                        if (ImGui::SliderFloat("ESP Distance", &g_espDistance, 100.0f, 2000.0f)) {
                            // Apply ESP distance
                        }

                        ImGui::Spacing();
                        ImGui::Separator();

                        ImGui::TextColored(MAKOTO_RED, "Execution Settings");
                        ImGui::Separator();

                        static bool autoExec = false;
                        static bool bypassChecks = true;
                        static int executionDelay = 0;

                        ImGui::Checkbox("Auto Execute on Attach", &autoExec);
                        ImGui::Checkbox("Bypass Script Checks", &bypassChecks);

                        ImGui::SliderInt("Execution Delay (ms)", &executionDelay, 0, 5000);

                        ImGui::EndTabItem();
                    }

                    // Credits tab
                    if (ImGui::BeginTabItem("  About  ")) {
                        ImGui::Spacing();

                        ImGui::TextColored(MAKOTO_RED, "MAKOTO EXECUTOR");
                        ImGui::Text("Version: 2.0.0");
                        ImGui::Text("Developer: djplaygames157ff");
                        ImGui::Spacing();
                        ImGui::Separator();
                        ImGui::Spacing();

                        ImGui::Text("Features:");
                        ImGui::BulletText("Dynamic script execution");
                        ImGui::BulletText("Infinite Yield support");
                        ImGui::BulletText("ESP & Aimbot modules");
                        ImGui::BulletText("Anti-detection system");
                        ImGui::BulletText("Custom UI with ImGui");

                        ImGui::EndTabItem();
                    }

                    ImGui::EndTabBar();
                }

                ImGui::Spacing();
                ImGui::Separator();

                // Footer
                ImGui::TextColored(MAKOTO_GRAY, "Roblox Process: %s", "Connected");
                ImGui::SameLine(windowSize.x - 200);
                ImGui::TextColored(MAKOTO_GRAY, "FPS: %.1f", ImGui::GetIO().Framerate);

                ImGui::End();
            }

            ImGui::Render();
            g_context->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
            float clearColor[4] = {0.02f, 0.02f, 0.02f, 1.0f};
            g_context->ClearRenderTargetView(g_mainRenderTargetView, clearColor);
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

            if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
                ImGui::UpdatePlatformWindows();
                ImGui::RenderPlatformWindowsDefault();
            }

            g_swapChain->Present(1, 0);

            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
    }
}
