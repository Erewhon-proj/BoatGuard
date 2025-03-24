#include <iot_board.h>
#include "EloquentTinyML.h"
#include "ormeggio_etichettati.h"
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLEAdvertisedDevice.h>
#include <BLEScan.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include "BLEUtils.h"
#include "sensori.h"
#include "lora_utils.h"
#include "costants.h"

BLEServer *pServer = NULL;
BLECharacteristic *pCharacteristic = NULL;

extern bool isConfigurated;
extern bool isNearMe;
extern bool isConnected;

extern void setup();

Eloquent::TinyML::TfLite<NUMBER_OF_INPUTS, NUMBER_OF_OUTPUTS, TENSOR_ARENA_SIZE> ml;

// Variabili posizione iniziale
static float posX = 0.0;
static float posY = 0.0;
static float direzione = 0.0; // Direzione in radianti

StatoBarca stato_attuale = STATO_ORMEGGIATA; // stato iniziale

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

void setup()
{
    IoTBoard::init_serial();
    randomSeed(millis());

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

    // Caricamento modello
    if (!ml.begin(ormeggio_model))
    {
        Serial.println("Errore nell'inizializzazione del modello");
    }
    else
    {
        Serial.println("Modello caricato");
    }

    // Inizializzazione LoRa
    IoTBoard::init_spi();
    if (!IoTBoard::init_lora())
    {
        Serial.println("LoRa Error");
    }
    else
    {
        Serial.println("LoRa abilitato");
    }

    lastOkMsgTime = millis();
}

// Rilevazioni barca
bool barcaOrmeggiata()
{
    const int NUM_RILEVAZIONI = 10;
    int count_non_ormeggiata = 0;

    Serial.println("Eseguo 10 rilevazioni per inferenza...");

    for (int i = 0; i < NUM_RILEVAZIONI; i++)
    {
        float input[NUMBER_OF_INPUTS];

        const int PROB_MOVIMENTO_PERCENT = 70; // 70% di essere in movimento
        bool in_movimento = (random(0, 100) < PROB_MOVIMENTO_PERCENT);

        // Lettura sensori simulati
        leggi_accelerometro(input[0], input[1], input[2], in_movimento);
        leggi_giroscopio(input[3], input[4], input[5], in_movimento);
        input[6] = leggi_solcometro(in_movimento);

        // Predizione
        float output[NUMBER_OF_OUTPUTS];
        ml.predict(input, output);

        bool is_ormeggiata = (output[0] < 0.5);
        if (!is_ormeggiata)
        {
            count_non_ormeggiata++;
        }

        Serial.print("  Rilevazione ");
        Serial.print(i + 1);
        Serial.print(" -> output=");
        Serial.print(output[0]);
        Serial.print(" => ");
        Serial.println(is_ormeggiata ? "BarcaOrmeggiata" : "BarcaNonOrmeggiata");

        delay(1000);
    }

    Serial.print("Conteggio 'Barca NON Ormeggiata': ");
    Serial.println(count_non_ormeggiata);

    if (count_non_ormeggiata > (NUM_RILEVAZIONI / 2))
    {
        return false; // barca rubata
    }
    else
    {
        return true; // barca ormeggiata
    }
}

void aggiornaPosizioneBarca(float deltaTimeSec)
{
    // Lettura giroscopio
    float gx, gy, gz;
    leggi_giroscopio(gx, gy, gz, true);

    direzione += gz * deltaTimeSec;

    // Lettura solcometro
    float velocita = leggi_solcometro(true);

    // Aggiorniamo la posizione
    posX += velocita * cos(direzione) * deltaTimeSec;
    posY += velocita * sin(direzione) * deltaTimeSec;

    // Invia posizione stimata via LoRa
    char buffer[60];
    snprintf(buffer, sizeof(buffer), "POS X=%.2f Y=%.2f", posX, posY);
    inviaMessaggioLoRa(buffer);

    // Messaggi di debug su Serial
    Serial.print("Nuova direzione (rad) = ");
    Serial.println(direzione);
    Serial.print("Posizione X = ");
    Serial.print(posX);
    Serial.print(", Y = ");
    Serial.println(posY);
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

void loop()
{
    switch (stato_attuale)
    {

    case STATO_ORMEGGIATA:
    {
        // Verifico se la barca risulta ormeggiata o meno
        bool ormeggiata = barcaOrmeggiata();
        if (!ormeggiata)
        {
            Serial.println("La Barca NON è ormeggiata -> ALLARME!");
            inviaMessaggioLoRa("ALLARME");
            stato_attuale = STATO_RUBATA;
        }
        else
        {
            Serial.println("La Barca risulta ormeggiata.");

            // Invio "OK" se è passato INTERVALLO_OK_MS
            unsigned long now = millis();
            if (now - lastOkMsgTime > INTERVALLO_OK_MS)
            {
                inviaMessaggioLoRa("OK");
                lastOkMsgTime = now;
            }

            // Deep sleep per 45 minuti
            esp_sleep_enable_timer_wakeup(sleepTime);
            esp_deep_sleep_start();
        }
    }
    break;

    case STATO_RUBATA:
    {
        // Stato di allarme con aggiornamento posizione

        // Aggiornamento posizione ogni 5s
        aggiornaPosizioneBarca(5.0);
        delay(5000);

        // Meccanisco di uscita tramite reset manuale
    }
    break;
    }
}
