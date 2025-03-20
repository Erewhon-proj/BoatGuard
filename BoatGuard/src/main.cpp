#include <iot_board.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLEAdvertisedDevice.h>
#include <BLEScan.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include "BLEUtils.h"

#define SERVICE_UUID "12345678-1234-1234-1234-123456789012"
#define CHARACTERISTIC_UUID "87654321-4321-4321-4321-210987654321"

BLEServer *pServer = NULL;
BLECharacteristic *pCharacteristic = NULL;

#define BLE_periferica 1

const char *REQUEST_CONNECTION_MESSAGE = "REQUEST_CONNECTION";
const char *CONFIRM_CONNECTION_MESSAGE = "CONFIRM_CONNECTION";

// Rimuovi le definizioni duplicate
// Preferences preferences;
// String connectedDeviceID = "";
// String targetDeviceMAC = "";
// BLEScan *bleScan = NULL;
// bool isConfigurated = false;

extern void setup();

void startAdvertising()
{

    display->println("Start ADV...");
    display->display();

    BLEDevice::init("LedBello");
    BLEServer *bleServer = BLEDevice::createServer();
    bleServer->setCallbacks(new MyServerCallbacks());
    BLEService *servizio = bleServer->createService(SERVICE_UUID);
    BLECharacteristic *caratteristica = servizio->createCharacteristic(CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);
    caratteristica->setCallbacks(new MyCharacteristicCallbacks());
    servizio->start();

    BLEAdvertising *bleAdvertising = BLEDevice::getAdvertising();
    bleAdvertising->addServiceUUID(SERVICE_UUID);
    bleAdvertising->setScanResponse(true);
    bleAdvertising->start();
    Serial.print("Configurato ");
    display->println("Ricerca...");
    display->display();
}

void setup()
{
    Serial.begin(115200);

    IoTBoard::init_serial();
    IoTBoard::init_leds();
    IoTBoard::init_display();

    preferences.begin("config", true);
    // isConfigurated = preferences.isKey("targa");
    preferences.end();

    if (isConfigurated)
    {
        Serial.println("Informazioni già configurate");
        printPreferences();
    }

    // startAdvertising();

    if (true)
    {
        Serial.println("Connessione diretta...");
        display->println("Connessione diretta con...");
        display->display();

        BLEClient *pClient = BLEDevice::createClient();

        BLEDevice::init("");
        bleScan = BLEDevice::getScan();
        bleScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
        bleScan->setActiveScan(true);
        BLEDevice::getScan()->setActiveScan(true);
        bleScan->setInterval(100);
        bleScan->setWindow(90);
        bleScan->start(5, false); // Scansione per 5 secondi
        // BLEDevice::deinit(false);

        delay(5000); // Attendi 5 secondi

        if (!pClient->isConnected())
        {
            Serial.println("Dispositivo non trovato, diventando visibile per l'accoppiamento...");
            display->println("Dispositivo non trovato, diventando visibile...");
            display->display();

            startAdvertising();
        }
    }
    else
    {
        Serial.println("Nessun dispositivo target specificato, diventando visibile per l'accoppiamento...");
        display->println("Nessun dispositivo target specificato, diventando visibile...");
        display->display();

        startAdvertising();
    }
}

void loop()
{
    delay(200);
}
