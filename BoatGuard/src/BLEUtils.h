#ifndef BLEUTILS_H
#define BLEUTILS_H

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLEAdvertisedDevice.h>
#include <BLEScan.h>
#include <ArduinoJson.h>
#include <Preferences.h>

extern Preferences preferences;
extern String connectedDeviceID;
extern BLEScan *bleScan;
extern bool isConfigurated;
extern bool isNearMe;
extern bool isConnected;

void printPreferences();
void connectToTargetDevice();
void startAdvertising();
JsonObject getPreferences(JsonDocument &doc);
String getServiceID();

bool getNearMe();
void setNearMe(bool value);

class MyServerCallbacks : public BLEServerCallbacks
{
    void onConnect(BLEServer *pServer, esp_ble_gatts_cb_param_t *param) override;
    void onDisconnect(BLEServer *pServer) override;
};

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
{
    void onResult(BLEAdvertisedDevice advertisedDevice) override;
};

class MyCharacteristicCallbacks : public BLECharacteristicCallbacks
{
    void onWrite(BLECharacteristic *pCharacteristic) override;
    void handleRequestConnection(BLECharacteristic *pCharacteristic);
    void handleGetInfo(BLECharacteristic *pCharacteristic);
    void handleSendInfo(JsonDocument &doc);
};

#endif // BLEUTILS_H
