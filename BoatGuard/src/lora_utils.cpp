#include "lora_utils.h"
#include <iot_board.h>


void inviaMessaggioLoRa(const char *msg)
{

    size_t len = strlen(msg); 

    lora->beginPacket();
    lora->write(0xFF); // Broadcast
    lora->write(0x01); // Indirizzo mittente
    lora->write((uint8_t)len);
    lora->write((const uint8_t *)msg, len);
    lora->endPacket();

    Serial.print("Messaggio LoRa inviato: ");
    Serial.println(msg);
}
