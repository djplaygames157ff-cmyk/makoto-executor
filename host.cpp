#include <windows.h>
#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <filesystem>

#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "user32.lib")

// ANSI color codes for console
#define COLOR_RED "\033[91m"
#define COLOR_WHITE "\033[97m"
#define COLOR_BLACK "\033[90m"
#define COLOR_RESET "\033[0m"
#define COLOR_BOLD "\033[1m"

#define MAKOTO_LOGO R"(
  __  __          _          __  __
 |  \/  |        | |        / _|/ _|
 | \  / |   ___  | | __ _  | |_| |_   ___
 | |\/| |  / _ \ | |/ _` | |  _|  _| / __|
 | |  | | |  __/ | | (_| | | | | |   \__ \
 |_|  |_|  \___| |_|\__,_| |_| |_|   |___/

        E X E C U T O R  v2.0
)"

// DLL export function types
typedef void (*ExecuteLuaScript_t)(const char* script);
typedef bool (*IsExecutorReady_t)();
typedef void (*ToggleUI_t)();

class MakotoHost {
public:
    MakotoHost() : m_hModule(nullptr), m_executeScript(nullptr),
                   m_isReady(nullptr), m_toggleUI(nullptr) {}

    ~MakotoHost() {
        Unload();
    }

    bool LoadDLL(const std::string& dllPath) {
        m_hModule = LoadLibraryA(dllPath.c_str());
        if (!m_hModule) {
            std::cerr << COLOR_RED << "[MAKOTO] Failed to load DLL: " << dllPath << COLOR_RESET << std::endl;
            std::cerr << COLOR_RED << "[MAKOTO] Error code: " << GetLastError() << COLOR_RESET << std::endl;
            return false;
        }

        m_executeScript = (ExecuteLuaScript_t)GetProcAddress(m_hModule, "ExecuteLuaScript");
        m_isReady = (IsExecutorReady_t)GetProcAddress(m_hModule, "IsExecutorReady");
        m_toggleUI = (ToggleUI_t)GetProcAddress(m_hModule, "ToggleUI");

        if (!m_executeScript || !m_isReady || !m_toggleUI) {
            std::cerr << COLOR_RED << "[MAKOTO] Failed to get export functions" << COLOR_RESET << std::endl;
            return false;
        }

        std::cout << COLOR_WHITE << "[MAKOTO] DLL loaded successfully" << COLOR_RESET << std::endl;
        return true;
    }

    void Unload() {
        if (m_hModule) {
            FreeLibrary(m_hModule);
            m_hModule = nullptr;
        }
    }

    bool IsReady() {
        if (m_isReady) {
            return m_isReady();
        }
        return false;
    }

    void ExecuteScript(const std::string& script) {
        if (m_executeScript) {
            m_executeScript(script.c_str());
            std::cout << COLOR_WHITE << "[MAKOTO] Script executed" << COLOR_RESET << std::endl;
        }
    }

    void ToggleUI() {
        if (m_toggleUI) {
            m_toggleUI();
            std::cout << COLOR_WHITE << "[MAKOTO] UI toggled" << COLOR_RESET << std::endl;
        }
    }

    void PrintMenu() {
        std::cout << "\n" << COLOR_RED << COLOR_BOLD << "╔══════════════════════════════════════╗" << COLOR_RESET << std::endl;
        std::cout << COLOR_RED << COLOR_BOLD << "║        MAKOTO EXECUTOR - MENU        ║" << COLOR_RESET << std::endl;
        std::cout << COLOR_RED << COLOR_BOLD << "╚══════════════════════════════════════╝" << COLOR_RESET << std::endl;
        std::cout << std::endl;
        std::cout << COLOR_WHITE << "  [1]" << COLOR_RESET << " Execute script from file" << std::endl;
        std::cout << COLOR_WHITE << "  [2]" << COLOR_RESET << " Execute inline script" << std::endl;
        std::cout << COLOR_WHITE << "  [3]" << COLOR_RESET << " Load Infinite Yield" << std::endl;
        std::cout << COLOR_WHITE << "  [4]" << COLOR_RESET << " Toggle UI" << std::endl;
        std::cout << COLOR_WHITE << "  [5]" << COLOR_RESET << " Check status" << std::endl;
        std::cout << COLOR_WHITE << "  [6]" << COLOR_RESET << " Execute from URL" << std::endl;
        std::cout << COLOR_WHITE << "  [0]" << COLOR_RESET << " Exit" << std::endl;
        std::cout << std::endl;
        std::cout << COLOR_RED << "  Choice: " << COLOR_RESET;
    }

    void Run() {
        std::cout << COLOR_WHITE << MAKOTO_LOGO << COLOR_RESET << std::endl;
        std::cout << std::endl;

        std::string dllPath;
        std::cout << COLOR_WHITE << "[MAKOTO] Enter DLL path (or press Enter for default 'MakotoExecutor.dll'): " << COLOR_RESET;
        std::getline(std::cin, dllPath);

        if (dllPath.empty()) {
            dllPath = "MakotoExecutor.dll";
        }

        if (!LoadDLL(dllPath)) {
            std::cout << COLOR_RED << "[MAKOTO] Press any key to exit..." << COLOR_RESET << std::endl;
            std::cin.get();
            return;
        }

        std::cout << COLOR_RED << "[MAKOTO] Waiting for Roblox..." << COLOR_RESET << std::endl;

        while (true) {
            if (IsReady()) {
                std::cout << COLOR_WHITE << "[MAKOTO] Connected to Roblox!" << COLOR_RESET << std::endl;
                break;
            }
            Sleep(1000);
        }

        int choice;
        while (true) {
            PrintMenu();
            std::cin >> choice;
            std::cin.ignore();

            switch (choice) {
                case 1: {
                    std::string filePath;
                    std::cout << COLOR_WHITE << "  Script file path: " << COLOR_RESET;
                    std::getline(std::cin, filePath);

                    std::ifstream file(filePath);
                    if (file.is_open()) {
                        std::string script((std::istreambuf_iterator<char>(file)),
                                           std::istreambuf_iterator<char>());
                        ExecuteScript(script);
                    } else {
                        std::cerr << COLOR_RED << "  Failed to open file" << COLOR_RESET << std::endl;
                    }
                    break;
                }

                case 2: {
                    std::cout << COLOR_WHITE << "  Enter script: " << COLOR_RESET;
                    std::string script;
                    std::getline(std::cin, script);
                    ExecuteScript(script);
                    break;
                }

                case 3: {
                    std::string infiniteYield = "loadstring(game:HttpGet('https://raw.githubusercontent.com/EdgeIY/infiniteyield/master/source'))()";
                    ExecuteScript(infiniteYield);
                    break;
                }

                case 4: {
                    ToggleUI();
                    break;
                }

                case 5: {
                    if (IsReady()) {
                        std::cout << COLOR_WHITE << "  Status: Ready" << COLOR_RESET << std::endl;
                    } else {
                        std::cerr << COLOR_RED << "  Status: Not ready" << COLOR_RESET << std::endl;
                    }
                    break;
                }

                case 6: {
                    std::cout << COLOR_WHITE << "  Script URL: " << COLOR_RESET;
                    std::string url;
                    std::getline(std::cin, url);
                    std::string script = "loadstring(game:HttpGet('" + url + "'))()";
                    ExecuteScript(script);
                    break;
                }

                case 0: {
                    std::cout << COLOR_RED << "[MAKOTO] Goodbye!" << COLOR_RESET << std::endl;
                    return;
                }

                default: {
                    std::cerr << COLOR_RED << "  Invalid choice" << COLOR_RESET << std::endl;
                    break;
                }
            }
        }
    }

private:
    HMODULE m_hModule;
    ExecuteLuaScript_t m_executeScript;
    IsExecutorReady_t m_isReady;
    ToggleUI_t m_toggleUI;
};

int main() {
    SetConsoleTitleA("MAKOTO EXECUTOR v2.0");

    MakotoHost host;
    host.Run();

    return 0;
}
