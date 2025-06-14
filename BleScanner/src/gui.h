//
// Created by pepiv on 16.05.2025.
//

#ifndef GUI_H
#define GUI_H

#pragma once

#include <functional>
#include <string>
#include "ble_manager.h"    // for AppState
#include "constants.h"      // for REQUEST_LIST, DEVICE_LIST

constexpr int max_data_stm_size = 50000;

/// GUI state – selected indexes, last message, and timing info
struct GuiState {
    int selectedRequest;
    int selectedDevice;
    AppState appState;
    std::string lastMessage;
    double lastTransferTimeMs;
    double lastCipherTimeMs;
    int requestedBytes;
    int wordSize;
    double interChunkDelayMs;
    int countOfBlocks;
};

/// Initializes a GuiState structure (optional if using default-initialized members)
inline void initGuiState(GuiState& s) {
    s.selectedRequest     = 0;
    s.selectedDevice      = 0;
    s.appState            = AppState::Ready;
    s.lastMessage.clear();
    s.lastTransferTimeMs    = 0.0;
    s.lastCipherTimeMs      = 0.0;
    s.requestedBytes        = 250;
    s.wordSize              = 250;
    s.interChunkDelayMs     = 0;
    s.countOfBlocks         = 0;
}

/// Renders the "Controls" window: device & protocol selection + action buttons
/// - onStart() will be called when the Start button is pressed
/// - onStop()  will be called when the Stop button is pressed
void renderControls(GuiState& state,
                    std::function<void()> onStart,
                    std::function<void()> onStop);

/// Renders the "Results" window: displays the last message and transfer timing
void renderResults(const GuiState& state);

/// Renders the status bar at the bottom of the screen based on the current state
void renderStatusBar(AppState state);

#endif // GUI_H