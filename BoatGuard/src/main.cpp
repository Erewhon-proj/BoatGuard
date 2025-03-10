#include <iot_board.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLEAdvertisedDevice.h>
#include <BLEScan.h>
#include <ArduinoJson.h>
#include <Preferences.h> // Include la libreria per NVS

#define SERVICE_UUID "12345678-1234-1234-1234-123456789012"        
#define CHARACTERISTIC_UUID "87654321-4321-4321-4321-210987654321" 

BLEServer *pServer = NULL;
BLECharacteristic *pCharacteristic = NULL;
BLEScan *bleScan = NULL;

#define BLE_periferica 1

const char *REQUEST_CONNECTION_MESSAGE = "REQUEST_CONNECTION";
const char *CONFIRM_CONNECTION_MESSAGE = "CONFIRM_CONNECTION";

bool isConfigurated = false;

Preferences preferences; // Oggetto per gestire le preferenze NVS

String connectedDeviceID = "";




void printPreferences() {
    preferences.begin("config", true);
    if (preferences.isKey("targa")) {
        String targa = preferences.getString("targa");
        String macAddress = preferences.getString("macAddress");
        String dispName = preferences.getString("dispName");
        String lat = preferences.getString("lat");
        String lng = preferences.getString("long");
        String deviceId = preferences.getString("deviceId");

        Serial.println("Informazioni salvate:");
        Serial.print("Targa: "); Serial.println(targa);
        Serial.print("MAC Address: "); Serial.println(macAddress);
        Serial.print("Nome Dispositivo: "); Serial.println(dispName);
        Serial.print("Latitudine: "); Serial.println(lat);
        Serial.print("Longitudine: "); Serial.println(lng);
        Serial.print("Device ID: "); Serial.println(deviceId);
    } else {
        Serial.println("Nessuna informazione salvata.");
    }
    preferences.end();
}

class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) override {
        Serial.println("Client connesso");
        display->print("Client connesso");
        display->display();
        connectedDeviceID = pServer->getConnId();
        Serial.println("Client connesso");
        Serial.println(pServer->getConnId());
    
    }

    void onDisconnect(BLEServer* pServer) override {
        display->clearDisplay();
        display->display();
        Serial.println("Client disconnesso, riavvio scansione...");
        display->println("Client disconnesso, riavvio scansione...");
        display->display();
        bleScan->start(0); 
    }
};

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) override {
        Serial.print("Dispositivo trovato: ");
        Serial.println(advertisedDevice.toString().c_str());

        if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(BLEUUID(SERVICE_UUID))) {
            Serial.println("Dispositivo desiderato trovato, connessione in corso...");
            bleScan->stop();
            BLEDevice::getScan()->stop();
            BLEClient *pClient = BLEDevice::createClient();
            pClient->connect(&advertisedDevice);
            Serial.println("Connesso al dispositivo");
        }
    }
};

JsonObject getPreferences(JsonDocument &doc) {
    preferences.begin("config", true);
    String targa = preferences.getString("targa");
    String macAddress = preferences.getString("macAddress");
    String dispName = preferences.getString("dispName");
    String lat = preferences.getString("lat");
    String lng = preferences.getString("long");
    preferences.end();

    JsonObject data = doc.createNestedObject("data");
    data["targa"] = targa;
    data["macAddress"] = macAddress;
    data["dispName"] = dispName;
    data["lat"] = lat;
    data["long"] = lng;

    return data;
}

class MyCharacteristicCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) override {
        std::string value = std::string(pCharacteristic->getValue().c_str());
        Serial.print("Valore scritto: ");
        Serial.println(value.c_str());

        StaticJsonDocument<256> doc;
        DeserializationError error = deserializeJson(doc, value);
        if (error) {
            Serial.print("Errore nella deserializzazione del messaggio JSON: ");
            Serial.println(error.c_str());
            return;
        }

        const char *type = doc["type"];
        Serial.println(type);

        if (type != nullptr) {
            Serial.print("Tipo di messaggio: ");
            Serial.println(type);
            display->println(type);
            display->display();

            if (strcmp(type, "REQUEST_CONNECTION") == 0) {
                handleRequestConnection(pCharacteristic);
            } else if (strcmp(type, "CONFIRM_CONNECTION") == 0) {
                Serial.println("Connessione confermata");
            } else if (strcmp(type, "GET_INFO") == 0) {
                handleGetInfo(pCharacteristic);
            } else if (strcmp(type, "SEND_INFO") == 0) {
                handleSendInfo(doc);
            } else {
                Serial.println("Tipo di messaggio sconosciuto");
                display->println("Tipo di messaggio sconosciuto");
                display->display();
            }
        }
    }



    void handleRequestConnection(BLECharacteristic *pCharacteristic) {
        char buffer[256];
        StaticJsonDocument<256> confirmDoc;
        confirmDoc["type"] = "CONFIRM_CONNECTION";
        serializeJson(confirmDoc, buffer);

        pCharacteristic->setValue(buffer);
        pCharacteristic->notify();
    }

    void handleGetInfo(BLECharacteristic *pCharacteristic) {
        Serial.println("RICEVUTO MESSAGGIO GET_INFO");

        StaticJsonDocument<256> dataDoc;
        dataDoc["type"] = "GET_INFO";
        JsonObject data = getPreferences(dataDoc);
        data["connectedDeviceID"] = connectedDeviceID;

        char buffer[256];
        serializeJson(dataDoc, buffer);

        Serial.println("Invio informazioni buffer: ");
        Serial.println(buffer);

        pCharacteristic->setValue(buffer);
        pCharacteristic->notify();
    }

    void handleSendInfo(JsonDocument &doc) {
        Serial.println("Richiesta di informazioni ricevuta");

        JsonObject data = doc["data"];

        if (!data.isNull()) {
            Serial.print("Dati: ");
            serializeJson(data, Serial);

            if (!isConfigurated) {
                preferences.begin("config", false);
                preferences.putString("targa", data["targa"].as<const char*>());
                preferences.putString("macAddress", data["macAddress"].as<const char*>());
                preferences.putString("dispName", data["dispName"].as<const char*>());
                preferences.putString("lat", data["lat"].as<const char*>());
                preferences.putString("long", data["long"].as<const char*>());
                preferences.putString("deviceId", connectedDeviceID);
                preferences.end();

                Serial.println("Informazioni salvate in NVS");
                printPreferences();
                //isConfigurated = true;
            }
        }
    }
};


void setup() {
    Serial.begin(115200);

    IoTBoard::init_serial();
    IoTBoard::init_leds();
    IoTBoard::init_display();

    preferences.begin("config", true);
   //isConfigurated = preferences.isKey("targa");
    preferences.end();

    if (isConfigurated) {
        Serial.println("Informazioni già configurate");
        printPreferences();
    }

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
    bleServer->setCallbacks(new MyServerCallbacks());
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

void loop() {
    delay(200);

}
