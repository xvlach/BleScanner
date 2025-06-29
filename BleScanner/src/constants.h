//
// Created by pepiv on 16.05.2025.
//

#ifndef CONSTANTS_H
#define CONSTANTS_H

#pragma once
#include <winrt/base.h>
#include <cstdint>
#include <array>
#include <vector>
#include <string>
#include <utility>

namespace AppConstants {
    inline const bool meastureAllTime = false;

    //––– Symmetric Keys –––//
    inline constexpr std::array<uint8_t, 32> KEY = {{
        0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,
        0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x10,
        0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,
        0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,0x20
    }};

    inline constexpr std::array<uint8_t, 12> NONCE = {{
        0xA1,0xB2,0xC3,0xD4,0xE5,0xF6,0x07,0x18,
        0x29,0x3A,0x4B,0x5C
    }};

    //––– BLE Services & Characteristics –––//
    inline const winrt::guid P2P_SERVICE_UUID {
        0x0000fe40, 0xcc7a, 0x482a, {0x98,0x4a,0x7f,0x2e,0xd5,0xb3,0xe5,0x8f}
    };
    inline const winrt::guid LED_CHARACTERISTIC_UUID {
        0x0000fe41, 0x8e22, 0x4541, {0x9d,0x4c,0x21,0xed,0xae,0x82,0xed,0x19}
    };
    inline const winrt::guid BUTTON_NOTIFY_UUID {
        0x0000fe42, 0x8e22, 0x4541, {0x9d,0x4c,0x21,0xed,0xae,0x82,0xed,0x19}
    };
    inline const winrt::guid DATA_IN_CHARACTERISTIC_UUID {
        0x0000fe43, 0x8e22, 0x4541, {0x9d,0x4c,0x21,0xed,0xae,0x82,0xed,0x19}
    };
    inline const winrt::guid DATA_OUT_CHARACTERISTIC_UUID {
        0x0000fe44, 0x8e22, 0x4541, {0x9d,0x4c,0x21,0xed,0xae,0x82,0xed,0x19}
    };
    inline const winrt::guid DATA_OUT_TIME_CHARACTERISTIC_UUID {
        0x0000fe45, 0x8e22, 0x4541, {0x9d,0x4c,0x21,0xed,0xae,0x82,0xed,0x19}
    };

    //––– Supported Protocols List –––//
    inline const std::vector<std::pair<std::string,uint8_t>> REQUEST_LIST = {
        { "ChaCha20",           0x01 },
        { "ChaCha20-Poly1305",  0x02 },
        { "AES-GCM",            0x03 },
    };

    //––– Predefined Devices –––//
    inline const std::vector<std::pair<std::string,uint64_t>> DEVICE_LIST = {
        { "STM32 #1",      0x0080E127919DULL },
        { "Test-example",  0x001122334455ULL },
        { "Test-example2", 0x001122334456ULL },
    };

}

#endif //CONSTANTS_H
