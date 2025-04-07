#include "CryptoUtils.h"
#include <iot_board.h>


void xorBuffer(void* data, size_t len, const uint8_t* key, size_t key_len) {
    uint8_t* byteData = static_cast<uint8_t*>(data);
    for (size_t i = 0; i < len; ++i) {
        byteData[i] ^= key[i % key_len];
    }

    Serial.println("XORed DONE");
}
