#ifndef LORA_UTILS_H
#define LORA_UTILS_H

#include <Arduino.h>

String criptaMessaggio(const char *messaggio);
void inviaMessaggioLoRa(const char *msg);

#endif
