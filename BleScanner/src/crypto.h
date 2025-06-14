//
// Created by pepiv on 16.05.2025.
//

#ifndef CRYPTO_H
#define CRYPTO_H
#pragma once

#include <vector>
#include <cstdint>
#include <mbedtls/chachapoly.h>
#include <mbedtls/gcm.h>
#include <chrono>

/// Decryption engine for ChaCha20, ChaCha20-Poly1305, and AES-GCM
class CryptoEngine {
public:
    CryptoEngine();
    ~CryptoEngine();

    /// (Re)initializes the engine based on selected method:
    /// 0x01 = ChaCha20, 0x02 = ChaCha20-Poly1305, 0x03 = AES-GCM
    void init(uint8_t requestType);

    /// Decrypts the given packet and measures decryption time (in milliseconds).
    /// @param packet  Input buffer (ciphertext [+ tag for Poly/GCM]).
    /// @param outMs   Output variable for time spent (in ms).
    /// @return        Decrypted plaintext as a vector<uint8_t>.
    std::vector<uint8_t> decrypt(const std::vector<uint8_t>& packet,
                                 double& outMs);

private:
    mbedtls_chachapoly_context _chachapoly;
    bool                       _polyInited    = false;
    mbedtls_gcm_context        _gcm;
    bool                       _gcmInited     = false;
    uint8_t                    _currentRequest = 0x00;
};

#endif //CRYPTO_H
