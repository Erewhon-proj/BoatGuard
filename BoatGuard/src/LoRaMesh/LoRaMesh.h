#ifndef LORAMESH_H
#define LORAMESH_H

#include <cstdint>
#define INDIRIZZO_BROADCAST 0xff
#define LORA_MESH_MESSAGE_SENT_UNSUCCESS 1
#define LORA_MESH_MESSAGE_SENT_SUCCESS 0
#define LORA_MESH_MESSAGE_QUEUE_FULL -1
#define TARGA_LEN 7
#define KEY_LEN 16

#include <iot_board.h>
#include "CircularQueue/CircularQueue.h"
#include "state_t.h"

typedef struct
{
    uint16_t message_sequence;
    StatoBarca stato;
    /*uint8_t livello_batteria;*/
    float posX;
    float posY;
    float direzione;
} LoRaMesh_payload_t;

typedef struct
{
    char targa_destinatario[7];
    char targa_mittente[7];
    uint16_t message_id;
    LoRaMesh_payload_t payload;
} LoRaMesh_message_t;

class LoRaMesh
{
public:
    LoRaMesh() = delete;

    static bool init(const char targa[7], void (*userOnReceiveCallBack)(LoRaMesh_message_t));
    static int sendMessage(const char targa_destinatario[7], LoRaMesh_payload_t payload);
    static float calculateDistance(int rssi);
    static void update();

private:
    static CircularQueue<uint16_t> queue;
    static char targa[7];
    static void onReceive(int packetSize);
    static void (*userOnReceiveCallBack)(LoRaMesh_message_t message);
    static void sendMessagePrivate(LoRaMesh_message_t message);
    static LoRaMesh_message_t messageToSend;
    static LoRaMesh_message_t messageToRedirect;
    static uint16_t message_sequence;
};

#endif