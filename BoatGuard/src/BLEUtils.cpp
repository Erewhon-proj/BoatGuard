#include "BLEUtils.h"
#include <iot_board.h>
#include <BLEDevice.h>
#include <BLEUtils.h>

#define SERVICE_UUID "12345678-1234-1234-1234-123456789012"
#define CHARACTERISTIC_UUID "87654321-4321-4321-4321-210987654321"

Preferences preferences;
String connectedDeviceID = "";

BLEScan *bleScan = NULL;

bool isConfigurated = false;
bool isNearMe = false;
bool isConnected = false;

bool getNearMe()
{
    return isNearMe;
}

void setNearMe(bool value)
{
    isNearMe = value;
    display->println("Nearme trovato");
    display->display();
}

void printPreferences()
{
    preferences.begin("config", true);
    if (preferences.isKey("targa"))
    {
        String targa = preferences.getString("targa");
        String dispName = preferences.getString("dispName");
        String lat = preferences.getString("lat");
        String lng = preferences.getString("long");
        String serviceID = preferences.getString("serviceID");
        String macBle = preferences.getString("macBle");

        Serial.println("Informazioni salvate:");
        Serial.print("Targa: ");
        Serial.println(targa);
        Serial.print("Nome Dispositivo: ");
        Serial.println(dispName);
        Serial.print("Latitudine: ");
        Serial.println(lat);
        Serial.print("Longitudine: ");
        Serial.println(lng);
        Serial.print("Service ID: ");
        Serial.println(serviceID);
        Serial.print("Mac Ble: ");
        Serial.println(macBle);
    }
    else
    {
        Serial.println("Nessuna informazione salvata.");
    }
    preferences.end();
}

JsonObject getPreferences(JsonDocument &doc)
{
    preferences.begin("config", true);
    String targa = preferences.getString("targa");
    String dispName = preferences.getString("dispName");
    String lat = preferences.getString("lat");
    String lng = preferences.getString("long");
    String serviceID = preferences.getString("serviceID");
    String macBle = preferences.getString("macBle");
    preferences.end();

    JsonObject data = doc.createNestedObject("data");
    data["targa"] = targa;
    data["dispName"] = dispName;
    data["lat"] = lat;
    data["long"] = lng;
    data["serviceID"] = serviceID;
    data["macBle"] = macBle;

    return data;
}

String getServiceID()
{
    preferences.begin("config", true);
    String serviceID = preferences.getString("serviceID");
    preferences.end();
    return serviceID;
}

void MyServerCallbacks::onConnect(BLEServer *pServer, esp_ble_gatts_cb_param_t *param)
{
    isConnected = true;
    Serial.println("Client connesso");
    display->print("Client connesso");
    display->display();
    Serial.println("Client connesso");
    Serial.println(pServer->getConnId());
}

void MyServerCallbacks::onDisconnect(BLEServer *pServer)
{
    isConnected = false;
    display->clearDisplay();
    display->display();
    Serial.println("Client disconnesso, riavvio...");
    display->println("Client disconnesso, riavvio...");
    display->display();
    // setup();
}

void MyAdvertisedDeviceCallbacks::onResult(BLEAdvertisedDevice advertisedDevice)
{
    Serial.print("Dispositivo trovato: ");
    Serial.println(advertisedDevice.toString().c_str());

    String savedServiceID = getServiceID();

    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(BLEUUID(savedServiceID.c_str())))
    {
        Serial.println("Dispositivo desiderato trovato, connessione in corso...");
        setNearMe(true);
        bleScan->stop();
        BLEDevice::getScan()->stop();
        // startAdvertising();
    }
}

void MyCharacteristicCallbacks::onWrite(BLECharacteristic *pCharacteristic)
{
    std::string value = std::string(pCharacteristic->getValue().c_str());
    Serial.print("Valore scritto: ");
    Serial.println(value.c_str());

    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, value);
    if (error)
    {
        Serial.print("Errore nella deserializzazione del messaggio JSON: ");
        Serial.println(error.c_str());
        return;
    }

    const char *type = doc["type"];
    Serial.println(type);

    if (type != nullptr)
    {
        Serial.print("Tipo di messaggio: ");
        Serial.println(type);
        display->println(type);
        display->display();

        if (strcmp(type, "REQUEST_CONNECTION") == 0)
        {
            handleRequestConnection(pCharacteristic);
        }
        else if (strcmp(type, "CONFIRM_CONNECTION") == 0)
        {
            Serial.println("Connessione confermata");
        }
        else if (strcmp(type, "GET_INFO") == 0)
        {
            handleGetInfo(pCharacteristic);
        }
        else if (strcmp(type, "SEND_INFO") == 0)
        {
            handleSendInfo(doc);
        }
        else if (strcmp(type, "SET_POSITION") == 0)
        {
            handleSetPosition(doc);
        }
        else
        {
            Serial.println("Tipo di messaggio sconosciuto");
            display->println("Tipo di messaggio sconosciuto");
            display->display();
        }
    }
}

void MyCharacteristicCallbacks::handleRequestConnection(BLECharacteristic *pCharacteristic)
{
    char buffer[256];
    StaticJsonDocument<256> confirmDoc;
    confirmDoc["type"] = "CONFIRM_CONNECTION";

    Serial.print("Invio conferma di connessione: ");

    serializeJson(confirmDoc, buffer);
    pCharacteristic->setValue(buffer);
    pCharacteristic->notify();
}

void MyCharacteristicCallbacks::handleGetInfo(BLECharacteristic *pCharacteristic)
{
    Serial.println("RICEVUTO MESSAGGIO GET_INFO");

    StaticJsonDocument<256> dataDoc;
    dataDoc["type"] = "GET_INFO";
    JsonObject data = getPreferences(dataDoc);

    char buffer[256];
    serializeJson(dataDoc, buffer);

    Serial.println("Invio informazioni buffer: ");
    Serial.println(buffer);

    pCharacteristic->setValue(buffer);
    pCharacteristic->notify();
}

void MyCharacteristicCallbacks::handleSendInfo(JsonDocument &doc)
{
    Serial.println("Richiesta di informazioni ricevuta");
    Serial.println("Ricevo le seguenti INFO");

    JsonObject data = doc["data"];
    if (!data.isNull())
    {
        Serial.print("Dati: ");
        serializeJson(data, Serial);

        if (!isConfigurated)
        {
            preferences.begin("config", false);
            preferences.putString("targa", data["targa"].as<const char *>());
            preferences.putString("dispName", data["dispName"].as<const char *>());
            preferences.putString("lat", data["lat"].as<const char *>());
            preferences.putString("long", data["long"].as<const char *>());
            preferences.putString("serviceID", data["serviceID"].as<const char *>());
            preferences.end();

            Serial.println("Informazioni salvate in NVS");
            printPreferences();
            // isConfigurated = true;
        }
    }
}

void MyCharacteristicCallbacks::handleSetPosition(JsonDocument &doc)
{
    JsonObject data = doc["data"];
    Serial.println("Ricevuto messaggio setPosition");

    if (!data.isNull())
    {
        double lat = data["lat"];
        double lng = data["long"];
        Serial.print("Set position: lat = ");
        Serial.print(lat);
        Serial.print(", lng = ");
        Serial.println(lng);
        display->println("Posizione impostata:");
        display->print("lat: ");
        display->println(lat);
        display->print("lng: ");
        display->println(lng);
        display->display();
    }
    else
    {
        Serial.println("Messaggio setPosition senza dati di posizione");
        display->println("Errore setPosition");
        display->display();
    }
}
