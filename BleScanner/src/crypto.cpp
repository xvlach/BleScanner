//
// Created by pepiv on 16.05.2025.
//

#include "crypto.h"
#include "constants.h"         // KEY, NONCE
#include <mbedtls/chacha20.h>
#include <mbedtls/chachapoly.h>
#include <chrono>
#include <stdexcept>

CryptoEngine::CryptoEngine() {
    // No initialization here – ChaCha-Poly is only initialized when requestType == 0x02
}

CryptoEngine::~CryptoEngine() {
    if (_polyInited) {
        mbedtls_chachapoly_free(&_chachapoly);
    }
}

void CryptoEngine::init(uint8_t requestType) {
    // If switching to Poly and not yet initialized:
    if (requestType == 0x02 && !_polyInited) {
        mbedtls_chachapoly_init(&_chachapoly);
        mbedtls_chachapoly_setkey(&_chachapoly, AppConstants::KEY.data());
        _polyInited = true;
    }
    // If switching from Poly to something else, free the context:
    else if (requestType != 0x02 && _polyInited) {
        mbedtls_chachapoly_free(&_chachapoly);
        _polyInited = false;
    }

    // AES-GCM initialization
    if (requestType == 0x03 && !_gcmInited) {
        mbedtls_gcm_init(&_gcm);
        // 256-bit key = 32 * 8 bits
        mbedtls_gcm_setkey(&_gcm, MBEDTLS_CIPHER_ID_AES, AppConstants::KEY.data(), (int)AppConstants::KEY.size() * 8);
        _gcmInited = true;
    }
    else if (requestType != 0x03 && _gcmInited) {
        mbedtls_gcm_free(&_gcm);
        _gcmInited = false;
    }

    _currentRequest = requestType;
}

std::vector<uint8_t> CryptoEngine::decrypt(const std::vector<uint8_t>& packet,
                                           double& outMs)
{
    using clock = std::chrono::steady_clock;
    auto t0 = clock::now();
    std::vector<uint8_t> plaintext;

    switch (_currentRequest) {
      case 0x01: {
        // ChaCha20
        size_t len = packet.size();
        plaintext.resize(len);
        if (mbedtls_chacha20_crypt(
                AppConstants::KEY.data(),
                AppConstants::NONCE.data(),
                1, len,
                packet.data(), plaintext.data()) != 0)
            throw std::runtime_error("ChaCha20 decrypt failed");
        break;
      }

      case 0x02: {
        // ChaCha20-Poly1305
        if (!_polyInited)
            throw std::runtime_error("ChaChaPoly not initialized");
        if (packet.size() < 16)
            throw std::runtime_error("Packet too small for Poly1305 tag");
        size_t ctLen = packet.size() - 16;
        const uint8_t* tag = packet.data();
        const uint8_t* ct  = packet.data() + 16;
        plaintext.resize(ctLen);
        if (mbedtls_chachapoly_auth_decrypt(
                &_chachapoly, ctLen,
                AppConstants::NONCE.data(), nullptr, 0,
                tag, ct, plaintext.data()) != 0)
            throw std::runtime_error("ChaChaPoly decrypt failed");
        break;
      }

      case 0x03: {
        // AES-GCM
        if (!_gcmInited)
            throw std::runtime_error("GCM not initialized");
        if (packet.size() < 16)
            throw std::runtime_error("Packet too small for GCM tag");
        const size_t tagLen = 16;
        const size_t ctLen = packet.size() - tagLen;

        const uint8_t* ct  = packet.data();
        const uint8_t* tag = packet.data() + ctLen;
        plaintext.resize(ctLen);
        if (mbedtls_gcm_auth_decrypt(&_gcm, ctLen,
            AppConstants::NONCE.data(), (int)AppConstants::NONCE.size(),
            nullptr, 0,   // no AAD
            tag, (int)tagLen,
            ct, plaintext.data()) != 0)
        throw std::runtime_error("AES-GCM decrypt failed or tag invalid");
        break;
      }

      default:
        throw std::runtime_error("Unknown requestType");
    }

    auto t1 = clock::now();
    outMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    return plaintext;
}