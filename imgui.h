#pragma once
// ImGui stub for compilation
#include <cmath>
#include <string>
#include <vector>
#include <functional>

struct ImVec2 { float x, y; ImVec2(float _x = 0, float _y = 0) : x(_x), y(_y) {} };
struct ImVec4 { float x, y, z, w; ImVec4(float _x = 0, float _y = 0, float _z = 0, float _w = 1) : x(_x), y(_y), z(_z), w(_w) {} };
struct ImVec4U32 { unsigned int x, y, z, w; };

struct ImGuiContext;
struct ImGuiIO;
struct ImGuiStyle;
struct ImGuiWindow;
struct ImGuiTabBar;

inline ImGuiContext* CreateContext() { return nullptr; }
inline void DestroyContext() {}
inline void NewFrame() {}
inline void Render() {}
inline void EndFrame() {}
inline void ShowDemoWindow(bool* p_open = nullptr) {}

#define ImGuiWindowFlags_None 0
#define ImGuiWindowFlags_NoCollapse 1
#define ImGuiWindowFlags_NoResize 2
