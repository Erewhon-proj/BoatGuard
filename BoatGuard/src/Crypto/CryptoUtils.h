#ifndef CRYPTO_UTILS_H
#define CRYPTO_UTILS_H

#include <stddef.h>
#include <stdint.h>

void xorBuffer(void* data, size_t len, const uint8_t* key, size_t key_len);

#endif // CRYPTO_UTILS_H
