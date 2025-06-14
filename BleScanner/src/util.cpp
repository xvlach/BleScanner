//
// Created by pepiv on 16.05.2025.
//

#include "util.h"
#include "constants.h"
#include "imgui.h"

#include <locale>
#include <cwchar>

BOOL WINAPI ConsoleHandler(DWORD signal)
{
    if (signal == CTRL_C_EVENT){
        extern std::atomic<bool> g_running;
        g_running = false;
        return TRUE;
    }
    return FALSE;
}

std::wstring GuidToString(winrt::guid const& id)
{
    wchar_t buf[64] = {0};
    swprintf_s(buf, _countof(buf),
        L"{%08x-%04x-%04x-%02x%02x-%02x%02x-%02x%02x-%02x%02x}",
        id.Data1, id.Data2, id.Data3,
        id.Data4[0], id.Data4[1], id.Data4[2], id.Data4[3],
        id.Data4[4], id.Data4[5], id.Data4[6], id.Data4[7]
    );
    return std::wstring(buf);
}

void SetupStyle()
{
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* C = style.Colors;

    // background colors
    C[ImGuiCol_WindowBg]      = ImVec4(0.12f, 0.15f, 0.20f, 1.00f);
    C[ImGuiCol_ChildBg]       = ImVec4(0.10f, 0.13f, 0.18f, 1.00f);
    C[ImGuiCol_PopupBg]       = ImVec4(0.12f, 0.15f, 0.20f, 1.00f);

    // accent colors
    C[ImGuiCol_Header]        = ImVec4(0.20f, 0.45f, 0.75f, 1.00f);
    C[ImGuiCol_HeaderHovered] = ImVec4(0.25f, 0.55f, 0.90f, 1.00f);
    C[ImGuiCol_HeaderActive]  = ImVec4(0.15f, 0.35f, 0.65f, 1.00f);

    C[ImGuiCol_FrameBg]       = ImVec4(0.15f, 0.20f, 0.27f, 1.00f);
    C[ImGuiCol_FrameBgHovered]= ImVec4(0.20f, 0.30f, 0.40f, 1.00f);
    C[ImGuiCol_FrameBgActive] = ImVec4(0.18f, 0.27f, 0.36f, 1.00f);

    C[ImGuiCol_Button]        = ImVec4(0.18f, 0.35f, 0.65f, 1.00f);
    C[ImGuiCol_ButtonHovered] = ImVec4(0.22f, 0.45f, 0.85f, 1.00f);
    C[ImGuiCol_ButtonActive]  = ImVec4(0.15f, 0.30f, 0.55f, 1.00f);

    C[ImGuiCol_TitleBg]       = ImVec4(0.10f, 0.14f, 0.18f, 1.00f);
    C[ImGuiCol_TitleBgActive] = ImVec4(0.12f, 0.18f, 0.23f, 1.00f);

    // rounding & padding
    style.WindowRounding    = 6.0f;
    style.FrameRounding     = 4.0f;
    style.GrabRounding      = 4.0f;
    style.WindowPadding     = ImVec2(10, 10);
    style.FramePadding      = ImVec2(6, 4);
    style.ItemSpacing       = ImVec2(8, 6);
    style.ScrollbarSize     = 14.0f;
    style.ScrollbarRounding = 6.0f;
}

// Global running flag used for graceful shutdown
std::atomic<bool> g_running{ true };