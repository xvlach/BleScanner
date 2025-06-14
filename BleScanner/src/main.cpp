#define IMGUI_ENABLE_DOCKING

#include "constants.h"
#include "util.h"
#include "crypto.h"
#include "ble_manager.h"
#include "gui.h"
#include "console.h"

// ImGui + GLFW
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>

// WinRT + Windows
#include <winrt/base.h>
#include <windows.h>

int main()
{
    // 1) Initialize WinRT and console handler
    init_apartment(winrt::apartment_type::multi_threaded);
    SetConsoleCtrlHandler(ConsoleHandler, TRUE);

    // 2) Initialize GLFW and create window
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    GLFWwindow* window = glfwCreateWindow(800, 600, "BLE Scanner", nullptr, nullptr);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);

    // 3) Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    SetupStyle();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    // 4) Create "backends" and GUI state
    SimpleConsole console;
    GuiState     guiState;
    initGuiState(guiState);

    CryptoEngine crypto;
    BleManager   ble;

    // 5) Register callbacks from BleManager
    ble.onLog([&](auto const& msg){
        console.AddLog("%s", msg.c_str());
    });
    ble.onStateChanged([&](AppState st){
        guiState.appState = st;
    });

    ble.onCipherTime([&](double cipherMs, int countOfBlocks){
        guiState.lastCipherTimeMs += cipherMs;
        guiState.countOfBlocks += countOfBlocks;
    });

    ble.onData([&](const std::vector<uint8_t>& packet, double rtt){
        console.AddLog("Notification received, RTT = %.2f ms", rtt);

        crypto.init(AppConstants::REQUEST_LIST[guiState.selectedRequest].second);
        double ms = 0.0;
        auto plain = crypto.decrypt(packet, ms);

        std::string gibberish;
        gibberish.reserve(packet.size());
        for (uint8_t b : packet) {
            if (b >= 0x20 && b < 0x7F)
                gibberish += static_cast<char>(b);
            else
                gibberish += '.';
        }
        console.AddLog("Encrypted text: %s", gibberish.c_str());

        std::string s(plain.begin(), plain.end());
        console.AddLog("Decrypted text: %s. Duration %.5f ms.", s.c_str(), ms);
        guiState.lastMessage += s;
        guiState.lastTransferTimeMs = rtt;
    });

    while (!glfwWindowShouldClose(window)) {
        // a) Process input and start new ImGui frame
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // b) Render Controls window
        renderControls(guiState,
            // onStart:
            [&](){
                guiState.appState = AppState::Scanning;
                ble.startScan(
                    AppConstants::DEVICE_LIST[guiState.selectedDevice].second,
                    AppConstants::REQUEST_LIST[guiState.selectedRequest].second,
                    static_cast<uint32_t>(guiState.requestedBytes),
                    static_cast<uint32_t>(guiState.wordSize),
                    guiState.interChunkDelayMs
                );
            },
            // onStop:
            [&](){
                auto bytes = guiState.lastMessage.size();
                auto timeMs = guiState.lastTransferTimeMs + guiState.lastCipherTimeMs;
                double count = guiState.countOfBlocks;
                double countExpected = guiState.requestedBytes / guiState.wordSize;
                double cipherTime = guiState.lastCipherTimeMs;

                console.AddLog("________________________________________________");
                console.AddLog("Transferred bytes: %d B", guiState.lastMessage.size());
                console.AddLog("Transferred time: %.3f ms (%.3f µs, %.3f s)", guiState.lastTransferTimeMs, guiState.lastTransferTimeMs * 1000, guiState.lastTransferTimeMs / 1000);

                if (count < countExpected) {
                    console.AddLog("Expected rounds: %.1f Actual rounds: %.1f", countExpected, count);
                    cipherTime = (cipherTime / count) * countExpected;
                    console.AddLog("Estimated cipher time: %.5f ms (%.5f µs, %.3f s)", cipherTime, cipherTime * 1000, cipherTime / 1000);
                } else {
                    console.AddLog("Cipher time: %.5f ms (%.5f µs, %.3f s)", cipherTime, cipherTime * 1000, cipherTime / 1000);
                }

                if (guiState.requestedBytes > 0) {
                    double success = 100.0 * bytes / guiState.requestedBytes;
                    console.AddLog("Success rate: %.2f%%", success);
                }
                if (timeMs > 0 && bytes > 0) {
                    double speedBps = bytes / (timeMs / 1000);
                    console.AddLog("Transfer speed: %.2f B/s (%.2f kB/s, %.2f kb/s)", speedBps, speedBps / 1024.0, speedBps * 8 / 1000);
                }
                console.AddLog("________________________________________________");

                ble.stopScan();
                guiState.lastMessage.clear();
                guiState.lastTransferTimeMs = 0.0;
                guiState.lastCipherTimeMs   = 0.0;
            }
        );

        // c) Render Results, Console, and StatusBar
        renderResults(guiState);
        console.Draw("BLE Console");
        renderStatusBar(guiState.appState);

        // d) Render ImGui content
        ImGui::Render();
        int w, h;
        glfwGetFramebufferSize(window, &w, &h);
        glViewport(0, 0, w, h);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    // 7) Cleanup
    ble.stopScan();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}