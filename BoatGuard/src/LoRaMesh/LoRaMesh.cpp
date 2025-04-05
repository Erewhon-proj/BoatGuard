#include "LoRaMesh/LoRaMesh.h"
#include "Crypto/CryptoUtils.h"


char LoRaMesh::targa[TARGA_LEN] = {0,0,0,0,0,0,0};

// Chiave che hanno tutti in comune
const uint8_t commonKey[KEY_LEN] = {
    0xDE, 0xAD, 0xBE, 0xEF,
    0x01, 0x23, 0x45, 0x67,
    0x89, 0xAB, 0xCD, 0xEF,
    0x11, 0x22, 0x33, 0x44
};

// Chiave specifica della nave
const uint8_t privateKey[KEY_LEN] = {
    0xBA, 0xAD, 0xF0, 0x0D,
    0xFE, 0xED, 0xDE, 0xAD,
    0x12, 0x34, 0x56, 0x78,
    0x9A, 0xBC, 0xDE, 0xF0
};


LoRaMesh_message_t LoRaMesh::messageToSend = {0};
LoRaMesh_message_t LoRaMesh::messageToRedirect = {0};
CircularQueue<uint16_t> LoRaMesh::queue;
void (*LoRaMesh::userOnReceiveCallBack)(LoRaMesh_message_t) = nullptr;

bool LoRaMesh::init(const char targa[7], void (*userOnReceiveCallBack)(LoRaMesh_message_t)) {
    for(int i = 0; i < 7; i++) {
        LoRaMesh::targa[i] = targa[i];

    }
    LoRaMesh::messageToSend = {0};
    LoRaMesh::messageToRedirect = {0};
    LoRaMesh::userOnReceiveCallBack = userOnReceiveCallBack;
    LoRaMesh::queue = {};
    IoTBoard::init_spi();
    if(!IoTBoard::init_lora()) {
        return 0;
    }
    lora->onReceive(LoRaMesh::onReceive);
    lora->receive();
    return 1;
}

void LoRaMesh::update() {
    lora->receive();
    if (messageToSend.message_id != 0) {
        sendMessagePrivate(messageToSend);
        messageToSend = {0};
    }
    lora->receive();
    if (messageToRedirect.message_id != 0) {
        sendMessagePrivate(messageToRedirect);
        messageToRedirect = {0};
    }
}

void LoRaMesh::onReceive(int packetSize) {
    // Il pacchetto è vuoto
    if (packetSize == 0 || packetSize != sizeof(LoRaMesh_message_t)) {
        return;
    }

    // Leggiamo il pacchetto cifrato
    LoRaMesh_message_t encryptedMessage;
    lora->readBytes((uint8_t*)&encryptedMessage, sizeof(LoRaMesh_message_t));

    // Evitiamo duplicati
    if (queue.getIndex(encryptedMessage.message_id) >= 0) return;
    queue.push(encryptedMessage.message_id);

    // Creiamo una copia per decriptarlo
    LoRaMesh_message_t decryptedMessage = encryptedMessage;

    // Decriptiamo con la chiave comune
    xorBuffer(&decryptedMessage, sizeof(LoRaMesh_message_t), commonKey, sizeof(commonKey));

    // Il messaggio non è per noi, lo reinviamo
    if (memcmp(decryptedMessage.targa_destinatario, LoRaMesh::targa, 7) != 0) {
        messageToRedirect = encryptedMessage;  // messaggio originale criptato
        return;
    }
    
    // Sennò decriptiamo il payload
    xorBuffer(&decryptedMessage.payload, sizeof(LoRaMesh_payload_t), privateKey, sizeof(privateKey));

    userOnReceiveCallBack(decryptedMessage);
}

int LoRaMesh::sendMessage(const char targa_destinatario[7], LoRaMesh_payload_t payload) {
    // Stiamo già inviando un messaggio. non conviene inviare altri messaggi
    if(messageToSend.message_id != 0) {
        return LORA_MESH_MESSAGE_QUEUE_FULL;
    }

    xorBuffer(&payload, sizeof(LoRaMesh_payload_t), privateKey, sizeof(privateKey));


    messageToSend = {
        .message_id = (uint16_t)random(1, 65535),
        .payload = payload,
    };

    for(int i = 0; i < 7; i++) {
        messageToSend.targa_destinatario[i] =  targa_destinatario[i];
        messageToSend.targa_mittente[i] = LoRaMesh::targa[i];
    }

    xorBuffer(&messageToSend, sizeof(LoRaMesh_message_t), commonKey, sizeof(commonKey));

    return LORA_MESH_MESSAGE_SENT_SUCCESS;
}

void LoRaMesh::sendMessagePrivate(LoRaMesh_message_t message) {
    lora->beginPacket();
    lora->write((uint8_t*)&message, sizeof(LoRaMesh_message_t));
    lora->endPacket();
}

