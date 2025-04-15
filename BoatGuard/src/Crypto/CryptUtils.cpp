#include "CryptoUtils.h"
#include <iot_board.h>

void xorBuffer(void *data, size_t len, const uint8_t *key, size_t key_len, bool recive)
{
    if (recive == true)
    {
        Serial.println("xorBuffer/Private Key (string): /6  ");
        Serial.println((const char *)key);
    }

    uint8_t *byteData = static_cast<uint8_t *>(data);
    for (size_t i = 0; i < len; ++i)
    {
        byteData[i] ^= key[i % key_len];
    }

    if (recive == true)
    {
        Serial.println("xorBuffer/Private Key (string): /6  ");
        Serial.println((const char *)key);
    }

    Serial.println("XORed Effettuato");
}
