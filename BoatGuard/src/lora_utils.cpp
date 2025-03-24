#include "lora_utils.h"
#include <iot_board.h>

String criptaMessaggio(const char *messaggio)
{
    String criptato = messaggio;
    for (int i = 0; i < criptato.length(); i++)
    {
        criptato[i] ^= 0x5A;
    }
    return criptato;
}

void inviaMessaggioLoRa(const char *msg)
{
    String messaggio = criptaMessaggio(msg);

    lora->beginPacket();
    lora->write(0xFF); // Broadcast
    lora->write(0x01); // Indirizzo mittente
    lora->write(messaggio.length());
    lora->write((const uint8_t *)messaggio.c_str(), messaggio.length());
    lora->endPacket();

    Serial.print("Messaggio LoRa inviato: ");
    Serial.println(messaggio);
}
