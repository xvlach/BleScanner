//
// Created by pepiv on 16.05.2025.
//

#include "gui.h"
#include "imgui.h"

void renderControls(GuiState& state,
                    std::function<void()> onStart,
                    std::function<void()> onStop)
{
    ImGui::Begin("Controls", nullptr, ImGuiWindowFlags_NoCollapse);

    // — Encryption method selection —
    ImGui::Text("Select encryption method:");
    if (ImGui::BeginCombo("##reqCombo", AppConstants::REQUEST_LIST[state.selectedRequest].first.c_str())) {
        for (int i = 0; i < (int)AppConstants::REQUEST_LIST.size(); ++i) {
            bool selected = (i == state.selectedRequest);
            if (state.selectedRequest == 0) {
                state.wordSize = 475;
            }
            else {
                state.wordSize = 400;
            }
            if (ImGui::Selectable(AppConstants::REQUEST_LIST[i].first.c_str(), selected)) {
                state.selectedRequest = i;
                auto method = AppConstants::REQUEST_LIST[i].first;
            }
            if (selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    // — Device selection —
    ImGui::Text("Select device:");
    if (ImGui::BeginCombo("##devCombo", AppConstants::DEVICE_LIST[state.selectedDevice].first.c_str())) {
        for (int i = 0; i < (int)AppConstants::DEVICE_LIST.size(); ++i) {
            bool selected = (i == state.selectedDevice);
            if (ImGui::Selectable(AppConstants::DEVICE_LIST[i].first.c_str(), selected)) {
                state.selectedDevice = i;
            }
            if (selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    ImGui::Text("Requested [B]");
    ImGui::SameLine();
    ImGui::Text("Word size [B]");
    ImGui::SameLine();
    ImGui::Text("Delay [ms]");

    ImGui::PushItemWidth(100);
    ImGui::InputInt("##reqBytes", &state.requestedBytes);
    if (state.requestedBytes < 0)    state.requestedBytes = 0;
    if (state.requestedBytes > max_data_stm_size) state.requestedBytes = max_data_stm_size;

    ImGui::SameLine();
    ImGui::InputInt("##wordSize", &state.wordSize);
    if (state.wordSize < 1)             state.wordSize = 1;
    if (state.wordSize > max_data_stm_size)
        state.wordSize = max_data_stm_size;

    ImGui::SameLine();
    ImGui::InputDouble("##delayMs", &state.interChunkDelayMs, 0.5f, 1.0f, "%.1f");
    if (state.interChunkDelayMs < 0) state.interChunkDelayMs = 0;
    ImGui::PopItemWidth();

    if (ImGui::Button("Start BLE", ImVec2(-1, 0))) {
        onStart();
    }
    ImGui::Dummy(ImVec2(0,5));
    if (ImGui::Button("Stop BLE", ImVec2(-1, 0))) {
        onStop();
    }

    // — Status text —
    const char* statusText = "Unknown";
    switch (state.appState) {
        case AppState::Ready:    statusText = "Ready";    break;
        case AppState::Scanning: statusText = "Scanning"; break;
        case AppState::Connected:statusText = "Connected";break;
    }
    ImGui::Text("Status: %s", statusText);

    ImGui::End();
}

void renderResults(const GuiState& state)
{
    ImGui::Begin("Results", nullptr, ImGuiWindowFlags_NoCollapse);

    ImGui::Text("Message:");
    ImGui::BeginChild("TransMsgBox", ImVec2(0, 100), true);
    ImGui::TextWrapped("%s", state.lastMessage.c_str());
    ImGui::EndChild();

    ImGui::Text("Message transfer time: %.2f ms. Cipher time: %.3f ms.",
                state.lastTransferTimeMs - state.lastCipherTimeMs,
                state.lastCipherTimeMs);

    ImGui::End();
}

void renderStatusBar(AppState state)
{
    // Position at the bottom-left corner
    ImGui::SetNextWindowPos(ImVec2(0, ImGui::GetIO().DisplaySize.y - 24));
    ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x, 24));

    ImGui::Begin("StatusBar", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs);

    const char* barText = (state == AppState::Connected) ? "Connected"
                        : (state == AppState::Scanning) ? "Scanning"
                        : "Stopped";
    ImGui::Text("BLE Controller — %s", barText);

    ImGui::End();
}