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

extern bool isConfigurated;
extern bool isNearMe;
extern bool isConnected;

extern void setup();

void setCostants()
{
    IoTBoard::init_serial();
    IoTBoard::init_leds();
    IoTBoard::init_display();
    BLEDevice::init("Esp32");

    preferences.begin("config", false);
    if (!preferences.isKey("macBle"))
    {
        String mac = BLEDevice::getAddress().toString();
        preferences.putString("macBle", mac);
        Serial.print("Salvato MAC BLE: ");
        Serial.println(mac);
    }
    preferences.end();

    // set isConfigurated
    preferences.begin("config", true);
    isConfigurated = preferences.isKey("targa");
    preferences.end();

    // set isNearMe
    isNearMe = getNearMe();
}

void startAdvertising()
{

    display->println("Start ADV...");
    display->display();

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
    Serial.print("Attendo Connessione...");
    display->println("Attendo Connessione...");
    display->display();
}

BLEClient *bleClient()
{
    Serial.println("Scansione in corso...");
    display->println("Scansione in corso...");
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
    return pClient;
}

void setup()
{
    Serial.begin(115200);
    setCostants();

    BLEClient *pClient = bleClient();

    if (isConfigurated)
    {
        Serial.println("Informazioni già configurate");
        // printPreferences();
    }

    if (!isConnected)
    {
        startAdvertising();
    }
}

void loop()
{

    /*
    BLEClient *pClient = bleClient();

    if (isConfigurated)
    {
        Serial.println("Informazioni già configurate");
        // printPreferences();
    }
    if (!isConnected)
    {
        startAdvertising();
    }

    delay(200);
    */
}
