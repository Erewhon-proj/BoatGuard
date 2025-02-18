#include <iot_board.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLEAdvertisedDevice.h>
#include <BLEScan.h>
#include <ArduinoJson.h>

#define SERVICE_UUID "12345678-1234-1234-1234-123456789012"        // Sostituisci con il tuo UUID del servizio
#define CHARACTERISTIC_UUID "87654321-4321-4321-4321-210987654321" // Sostituisci con il tuo UUID della caratteristica

BLEServer *pServer = NULL;
BLECharacteristic *pCharacteristic = NULL;
BLEScan *bleScan = NULL;

#define BLE_periferica 1

const char *REQUEST_CONNECTION_MESSAGE = "REQUEST_CONNECTION";
const char *CONFIRM_CONNECTION_MESSAGE = "CONFIRM_CONNECTION";


/* 
class MyClientCallback : public BLEClientCallbacks {
    void onConnect(BLEClient* pClient) {
        Serial.println("Client connesso");
        display->print("Client connesso");
        display->display();
    }

    void onDisconnect(BLEClient* pClient) {
        display->clearDisplay();
        display->display();
        Serial.println("Client disconnesso, riavvio scansione...");
        display->println("Client disconnesso, riavvio scansione...");
        display->display();
        bleScan->start(0); // Riavvia la scansione
    }
};
*/

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
{
    void onResult(BLEAdvertisedDevice advertisedDevice)
    {
        Serial.print("Dispositivo trovato: ");
        Serial.println(advertisedDevice.toString().c_str());
        display->println("Dispositivo trovato: ");
        display->display();

        // Controlla se il dispositivo trovato è quello che stai cercando
        if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(BLEUUID(SERVICE_UUID)))
        {
            Serial.println("Dispositivo desiderato trovato, connessione in corso...");
            bleScan->stop();
            BLEDevice::getScan()->stop();
            BLEClient *pClient = BLEDevice::createClient();
            pClient->connect(&advertisedDevice);

            Serial.println("Connesso al dispositivo");
            display->println("Connesso al dispositivo");
            display->display();
        }
    }
};

class MyCharacteristicCallbacks : public BLECharacteristicCallbacks
{
    void onWrite(BLECharacteristic *pCharacteristic)
    {
        std::string value = std::string(pCharacteristic->getValue().c_str());
        Serial.print("Valore scritto: ");
        Serial.println(value.c_str());

        display->println(value.c_str());
        display->display();

        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, value);
        if (error)
        {
            Serial.print("Errore nella deserializzazione del messaggio JSON: ");
            Serial.println(error.c_str());
            display->println("Errore nella deserializzazione del messaggio JSON");
            display->display();
            return;
        }

        const char *type = doc["type"];

        if (type != nullptr)
        {
            Serial.print("Tipo di messaggio: ");
            Serial.println(type);
            display->println("Tipo di messaggio: ");
            display->println(type);
            display->display();


            JsonDocument confirmDoc;
            confirmDoc["type"] = "CONFIRM_CONNECTION";
            char buffer[256];
            serializeJson(confirmDoc, buffer);



            if (strcmp(type, "REQUEST_CONNECTION") == 0)
            {
                Serial.println("Richiesta di connessione ricevuta");
                display->println("Richiesta di connessione ricevuta");
                display->display();
                pCharacteristic->setValue(buffer);
                pCharacteristic->notify();
            }
            else if (strcmp(type, "CONFIRM_CONNECTION") == 0)
            {
                Serial.println("Connessione confermata");
                display->println("Connessione confermata");
                display->display();
            }
            else
            {
                Serial.println("Tipo di messaggio sconosciuto");
                display->println("Tipo di messaggio sconosciuto");
                display->display();
            }
        }
    }
};

void setup()
{
    Serial.begin(115200);

    IoTBoard::init_serial();
    IoTBoard::init_leds();
    IoTBoard::init_display();
    display->println("Display enabled");
    display->display();

#ifndef BLE_periferica
    BLEDevice::init("");
    bleScan = BLEDevice::getScan();
    bleScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    bleScan->setActiveScan(true);
    bleScan->setInterval(100);
    bleScan->setWindow(90);
    bleScan->start(5000, true);
#endif

#ifdef BLE_periferica
    BLEDevice::init("LedBello");
    BLEServer *bleServer = BLEDevice::createServer();
    BLEService *servizio = bleServer->createService(SERVICE_UUID);
    BLECharacteristic *caratteristica = servizio->createCharacteristic(CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);
    caratteristica->setValue("off");
    caratteristica->setCallbacks(new MyCharacteristicCallbacks());
    servizio->start();

    BLEAdvertising *bleAdvertising = BLEDevice::getAdvertising();
    bleAdvertising->addServiceUUID(SERVICE_UUID);
    bleAdvertising->setScanResponse(true);
    bleAdvertising->start();
    Serial.print("Configurato ");
    display->println("Ricerca...");
    display->display();
#endif
}

void loop()
{

    // Serial.print("Configurato ");

    delay(200); // Utilizza delay invece di sleep per Arduino
}
