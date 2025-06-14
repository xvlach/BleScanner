//
// Created by pepiv on 16.05.2025.
//

#ifndef UTIL_H
#define UTIL_H
#pragma once

#include <windows.h>            // for ConsoleHandler
#include <winrt/base.h>         // for winrt::guid
#include <string>
#include <atomic>

/// Console handler for CTRL+C and other signals
BOOL WINAPI ConsoleHandler(DWORD signal);

/// Converts a winrt::guid to a std::wstring representation
std::wstring GuidToString(winrt::guid const& id);

/// Applies global ImGui style (colors, rounding, padding, etc.)
void SetupStyle();

/// Global control flag for application state (used in shutdown handling)
extern std::atomic<bool> g_running;

#endif // UTIL_H