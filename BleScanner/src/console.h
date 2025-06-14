//
// Created by pepiv on 16.05.2025.
//

#ifndef CONSOLE_H
#define CONSOLE_H

#pragma once

#include <vector>
#include <string>
#include <cstdarg>
#include <imgui.h>

/// Very simple ImGui‐based console widget
struct SimpleConsole {
    std::vector<std::string> Items;
    bool AutoScroll = true;

    void Clear() { Items.clear(); }

    void AddLog(const char* fmt, ...) IM_FMTARGS(2) {
        va_list args; va_start(args, fmt);
        char buf[1024]; vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);
        Items.emplace_back(buf);
    }

    void Draw(const char* title, ImGuiWindowFlags flags = 0) {
        ImGui::Begin(title, nullptr, flags);
        if (ImGui::Button("Clear")) Clear();
        ImGui::SameLine();
        ImGui::Checkbox("Auto-scroll", &AutoScroll);
        ImGui::Separator();
        ImGui::BeginChild("ScrollingRegion", ImVec2(0,0), false, ImGuiWindowFlags_HorizontalScrollbar);
        for (auto& line : Items)
            ImGui::TextUnformatted(line.c_str());
        if (AutoScroll && ImGui::GetScrollMaxY() > ImGui::GetScrollY())
            ImGui::SetScrollHereY(1.0f);
        ImGui::EndChild();
        ImGui::End();
    }
};

#endif //CONSOLE_H
