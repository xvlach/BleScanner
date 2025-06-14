//
// Created by pepiv on 16.05.2025.
//
#include "ble_manager.h"
#include "constants.h"
#include <winrt/Windows.Devices.Radios.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/base.h>
#include <windows.h>
#include <sstream>
#include <chrono>
#include <winrt/Windows.Foundation.Collections.h>

using namespace winrt;
using namespace Windows::Devices::Bluetooth;
using namespace Windows::Devices::Bluetooth::Advertisement;
using namespace Windows::Devices::Bluetooth::GenericAttributeProfile;
using namespace Windows::Storage::Streams;
using namespace Windows::Devices::Radios;
using namespace Windows::Foundation::Collections;

BleManager::BleManager() = default;

BleManager::~BleManager() {
    stopScan();
}

void BleManager::onLog(std::function<void(const std::string&)> cb) {
    _logCb = std::move(cb);
}
void BleManager::onStateChanged(std::function<void(AppState)> cb) {
    _stateCb = std::move(cb);
}
void BleManager::onData(std::function<void(const std::vector<uint8_t>&, double)> cb) {
    _dataCb = std::move(cb);
}
void BleManager::onCipherTime(std::function<void(double, int)> cb) {
    _cipherCb = std::move(cb);
}

void BleManager::startScan(uint64_t address, uint8_t requestType, uint32_t bytesToRequest, uint32_t wordSize, double interChunkDelayMs) {
    if (_running) return;
    // Clean up any previous thread
    if (_scanThread.joinable()) _scanThread.join();

    _requestType = requestType;
    _bytesToRequest = bytesToRequest;
    _interChunkDelayMs = interChunkDelayMs;
    _wordSize = wordSize;
    _running = true;
    _state = AppState::Scanning;
    if (_stateCb) _stateCb(_state);
    if (_logCb) _logCb("Starting scan");

    _scanThread = std::thread([this, address]() {
        init_apartment(apartment_type::multi_threaded);

        // 1) Check BT radio
        {
            auto radios = Radio::GetRadiosAsync().get();
            bool ok = false;
            for (auto const& r : radios) {
                if (r.Kind() == RadioKind::Bluetooth && r.State() == RadioState::On) {
                    ok = true; break;
                }
            }
            if (!ok) {
                if (_logCb) _logCb("Bluetooth not enabled");
                _running = false;
                _state = AppState::Ready;
                if (_stateCb) _stateCb(_state);
                return;
            }
        }

        // 2) Prepare watcher
        _watcher = BluetoothLEAdvertisementWatcher();
        _watcher.Received([this, address](auto const&, auto const& args) {
            uint64_t addr = args.BluetoothAddress();
            if (_logCb) {
                char buf[64];
                sprintf_s(buf, "Advertisement %016llX RSSI %d", addr, args.RawSignalStrengthInDBm());
                _logCb(buf);
            }
            if (addr == address) {
                _watcher.Stop();
                if (_logCb) _logCb("Found target, connecting…");
                connectToDevice(addr);
            }
        });

        _watcher.Start();
        if (_logCb) _logCb("Watcher started");
        // 3) Poll until stopped
        while (_running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        _watcher.Stop();
        if (_logCb) _logCb("Watcher stopped");
    });
}

void BleManager::stopScan() {
    if (!_running && !_device) return;
    _running = false;
    if (_scanThread.joinable()) _scanThread.join();
    if (_logCb) _logCb("Scan thread joined");

    // Disconnect if connected
    if (_device) {
        if (_logCb) _logCb("Disconnecting device");
        // unregister notifications
        if (_dataOutChar) {
            _dataOutChar.ValueChanged(_dataOutToken);
            _dataOutChar = nullptr;
        }
        if (_buttonChar) {
            _buttonChar.ValueChanged(_buttonToken);
            _buttonChar = nullptr;
        }
        if (_timingChar) {
            _timingChar.ValueChanged(_timingToken);
            _timingChar = nullptr;
        }
        _device.Close();
        _device = nullptr;
    }
    _state = AppState::Ready;
    if (_stateCb) _stateCb(_state);
}

void BleManager::connectToDevice(uint64_t address) {
    auto dev = BluetoothLEDevice::FromBluetoothAddressAsync(address).get();
    if (!dev) {
        if (_logCb) _logCb("Failed to connect");
        _running = false;
        _state = AppState::Ready;
        if (_stateCb) _stateCb(_state);
        return;
    }
    _device = dev;
    _state = AppState::Connected;
    if (_stateCb) _stateCb(_state);
    if (_logCb) {
        std::wstring name = dev.Name().c_str();
        std::string s(name.begin(), name.end());
        _logCb("Connected to: " + s);
    }

    // Discover P2P service
    auto sr = dev.GetGattServicesForUuidAsync(AppConstants::P2P_SERVICE_UUID, BluetoothCacheMode::Uncached).get();
    auto svcs = sr.Services();
    if (svcs.Size() == 0) {
        if (_logCb) _logCb("P2P service not found");
        return;
    }
    auto svc = svcs.GetAt(0);

    {
        auto all = svc.GetCharacteristicsAsync(BluetoothCacheMode::Uncached).get();
        if (_logCb) _logCb("Service characteristics:");
        for (auto const& c : all.Characteristics()) {
            // převedeme winrt::guid na std::wstring a pak do UTF-8
            std::wstring wuuid = winrt::to_hstring(c.Uuid()).c_str();
            std::string  suuid( wuuid.begin(), wuuid.end() );

            auto props = static_cast<unsigned>(c.CharacteristicProperties());
            char buf[128];
            sprintf_s(buf,
                      "  • UUID: %s, props: 0x%02X",
                      suuid.c_str(),
                      props);
            if (_logCb) _logCb(buf);
        }
    }

    if (AppConstants::meastureAllTime) {
        _startTime = std::chrono::steady_clock::now();
    }
    enableDataNotifications(svc);
    enableTimingNotifications(svc);

    uint32_t total     = _bytesToRequest;
    uint32_t sentSoFar = 0;

    while (sentSoFar < total) {
        uint32_t remaining = total - sentSoFar;
        uint32_t thisChunk = (remaining < _wordSize) ? remaining : _wordSize;
        sendDataToDevice(svc, _requestType, thisChunk);
        sentSoFar += thisChunk;
        std::this_thread::sleep_for(std::chrono::milliseconds(7));
    }
}

void BleManager::enableDataNotifications(GattDeviceService const& svc) {
    auto csr = svc.GetCharacteristicsForUuidAsync(AppConstants::DATA_OUT_CHARACTERISTIC_UUID, BluetoothCacheMode::Uncached).get();
    if (csr.Characteristics().Size() == 0) {
        if (_logCb) _logCb("Data-out characteristic not found");
        return;
    }
    _dataOutChar = csr.Characteristics().GetAt(0);

    _dataOutToken = _dataOutChar.ValueChanged([this](auto const&, auto const& args) {
        auto end = std::chrono::steady_clock::now();
        double diffMs = std::chrono::duration<double, std::milli>(end - _startTime).count();

        DataReader reader = DataReader::FromBuffer(args.CharacteristicValue());
        std::vector<uint8_t> buf(reader.UnconsumedBufferLength());
        reader.ReadBytes(buf);

        if (_dataCb) _dataCb(buf, diffMs);
    });

    _dataOutChar.WriteClientCharacteristicConfigurationDescriptorAsync(
        GattClientCharacteristicConfigurationDescriptorValue::Notify).get();
    if (_logCb) _logCb("Data notifications enabled");
}

void BleManager::enableTimingNotifications(GattDeviceService const& svc) {
    auto csr = svc.GetCharacteristicsForUuidAsync(
                   AppConstants::DATA_OUT_TIME_CHARACTERISTIC_UUID,
                   BluetoothCacheMode::Uncached).get();
    if (csr.Characteristics().Size() == 0) {
        if (_logCb) _logCb("Timing characteristic not found");
        return;
    }
    _timingChar = csr.Characteristics().GetAt(0);
    _timingToken = _timingChar.ValueChanged([this](auto const&, auto const& args) {
        DataReader reader = DataReader::FromBuffer(args.CharacteristicValue());
        auto len = reader.UnconsumedBufferLength();
        std::vector<uint8_t> buf(len);
        reader.ReadBytes(buf);

        if (buf.size() < 4) {
            if (_logCb) _logCb("Timing: payload too small for uint32");
            return;
        }

        uint32_t us =
            static_cast<uint32_t>(buf[0]) |
            (static_cast<uint32_t>(buf[1]) << 8) |
            (static_cast<uint32_t>(buf[2]) << 16) |
            (static_cast<uint32_t>(buf[3]) << 24);

        double ms = us / 1000.0;

        if (_logCb) {
            char msg[64];
            std::snprintf(msg, sizeof(msg),
                          "Cipher time on MCU: %u µs → %.3f ms", us, ms);
            _logCb(msg);
        }
        if (_cipherCb) {
            _cipherCb(ms, 1);
        }
    });

    _timingChar.WriteClientCharacteristicConfigurationDescriptorAsync(GattClientCharacteristicConfigurationDescriptorValue::Notify).get();

    if (_logCb) _logCb("Timing notifications enabled");
}

void BleManager::sendDataToDevice(GattDeviceService const& svc, uint8_t requestType, uint16_t bytesToRequest) {
    auto csr = svc.GetCharacteristicsForUuidAsync(AppConstants::DATA_IN_CHARACTERISTIC_UUID, BluetoothCacheMode::Uncached).get();
    if (csr.Characteristics().Size() == 0) {
        if (_logCb) _logCb("Data-in characteristic not found");
        return;
    }
    auto inChar = csr.Characteristics().GetAt(0);

    if (!AppConstants::meastureAllTime) {
        _startTime = std::chrono::steady_clock::now();
    }

    DataWriter writer;
    writer.WriteByte(requestType);
    writer.WriteUInt16(bytesToRequest);
    auto buf = writer.DetachBuffer();
    inChar.WriteValueAsync(buf, GattWriteOption::WriteWithoutResponse).get();
    
    if (_logCb) {
                char logBuf[64];
                std::snprintf(logBuf, sizeof(logBuf),
                              "Request sent, start timer (bytesToRequest=%u)",
                              static_cast<unsigned>(bytesToRequest));
                _logCb(logBuf);
            }
}