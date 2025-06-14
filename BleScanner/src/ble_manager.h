//
// Created by pepiv on 16.05.2025.
//

#ifndef BLE_MANAGER_H
#define BLE_MANAGER_H
#pragma once

#include <functional>
#include <vector>
#include <string>
#include <cstdint>
#include <atomic>
#include <thread>

#include <winrt/base.h>
#include <winrt/Windows.Devices.Bluetooth.h>
#include <winrt/Windows.Devices.Bluetooth.Advertisement.h>
#include <winrt/Windows.Devices.Bluetooth.GenericAttributeProfile.h>

/// Application state for BLE
enum class AppState { Ready, Scanning, Connected };

class BleManager {
public:
    BleManager();
    ~BleManager();

    /// Register a logging callback (e.g. to SimpleConsole)
    void onLog(std::function<void(const std::string&)> cb);
    /// Register a state-change callback (Ready/Scanning/Connected)
    void onStateChanged(std::function<void(AppState)> cb);
    /// Register a data callback: (rawPacket, rtt_ms)
    void onData(std::function<void(const std::vector<uint8_t>&, double)> cb);
    /// Register a cipher time callback
    void onCipherTime(std::function<void(double, int)> cb);

    /// Start scanning for a single device address, then connect + notify
    /// @param address     64-bit BLE address
    /// @param requestType 0x01, 0x02, 0x3 ...
    /// @param bytesToRequest 0 - 20000 B data length
    /// @param wordSize cipher word size
    /// @param interChunkDelayMs inter chunk delay
    void startScan(uint64_t address, uint8_t requestType, uint32_t bytesToRequest, uint32_t wordSize, double interChunkDelayMs);

    /// Stop scanning / disconnect if connected
    void stopScan();

private:
    void connectToDevice(uint64_t address);
    void enableDataNotifications(winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattDeviceService const& svc);
    void enableTimingNotifications(winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattDeviceService const& svc);
    void sendDataToDevice(winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattDeviceService const& svc, uint8_t requestType, uint16_t bytesToRequest);

    winrt::Windows::Devices::Bluetooth::Advertisement::BluetoothLEAdvertisementWatcher _watcher{ nullptr };
    winrt::Windows::Devices::Bluetooth::BluetoothLEDevice _device{ nullptr };
    winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattCharacteristic _dataOutChar{ nullptr };
    winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattCharacteristic _buttonChar{ nullptr };
    winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattCharacteristic _timingChar{ nullptr };
    winrt::event_token    _timingToken{};
    winrt::event_token    _dataOutToken{};
    winrt::event_token    _buttonToken{};
    std::thread           _scanThread;
    std::atomic<bool>     _running{ false };
    uint8_t               _requestType = 0x01;
    std::chrono::steady_clock::time_point _startTime;
    std::function<void(const std::string&)> _logCb{};
    std::function<void(AppState)> _stateCb{};
    std::function<void(const std::vector<uint8_t>&, double)> _dataCb{};
    std::function<void(double, int)> _cipherCb;
    AppState              _state = AppState::Ready;
    uint32_t              _bytesToRequest;
    uint32_t              _wordSize;
    double                _interChunkDelayMs;
};

#endif //BLE_MANAGER_H
