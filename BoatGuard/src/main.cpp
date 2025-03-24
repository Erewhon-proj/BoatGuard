#include <iot_board.h>
#include "EloquentTinyML.h"
#include "ormeggio_etichettati.h"
#include "esp_sleep.h"

#define NUMBER_OF_INPUTS 7
#define NUMBER_OF_OUTPUTS 1
#define TENSOR_ARENA_SIZE 2 * 1024

Eloquent::TinyML::TfLite<NUMBER_OF_INPUTS, NUMBER_OF_OUTPUTS, TENSOR_ARENA_SIZE> ml;

const unsigned long INTERVALLO_OK_MS = 6UL * 60UL * 60UL * 1000UL; // 6 ore
const unsigned long sleepTime = 45UL * 60UL * 1000UL;              // 6 ore
unsigned long lastOkMsgTime = 0;                                   // Ultimo OK inviato
const char CHIAVE_CIFRATURA = 0x5A;                                // Chiave di cifratura

// Variabili posizione iniziale
static float posX = 0.0;
static float posY = 0.0;
static float direzione = 0.0; // Direzione in radianti

// Definizione stati barca
enum StatoBarca
{
    STATO_ORMEGGIATA,
    STATO_RUBATA
};

StatoBarca stato_attuale = STATO_ORMEGGIATA; // stato iniziale

// Letture Sensori Simulate & Predizione
void leggi_accelerometro(float &x, float &y, float &z, bool in_movimento);
void leggi_giroscopio(float &x, float &y, float &z, bool in_movimento);
float leggi_solcometro(bool in_movimento);

bool barcaOrmeggiata();
void aggiornaPosizioneBarca(float deltaTimeSec);

// Lora
String criptaMessaggio(const char *messaggio);
void inviaMessaggioLoRa(const char *msg);

void setup()
{
    IoTBoard::init_serial();
    randomSeed(millis());

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

// Letture Sensori Simulate
void leggi_accelerometro(float &x, float &y, float &z, bool in_movimento)
{
    if (in_movimento)
    {
        x = random(-500, 500) / 1000.0; // [-0.5, 0.5]
        y = random(-500, 500) / 1000.0;
        z = random(900, 980) / 100.0; // [9.0, 9.8]
    }
    else
    {
        x = random(-50, 50) / 1000.0; // [-0.05, 0.05]
        y = random(-50, 50) / 1000.0;
        z = random(975, 985) / 100.0; // [9.75, 9.85]
    }
}

void leggi_giroscopio(float &x, float &y, float &z, bool in_movimento)
{
    if (in_movimento)
    {
        x = random(-500, 500) / 1000.0; // [-0.5, 0.5]
        y = random(-500, 500) / 1000.0;
        z = random(-500, 500) / 1000.0;
    }
    else
    {
        x = random(-20, 20) / 1000.0; // [-0.02, 0.02]
        y = random(-20, 20) / 1000.0;
        z = random(-20, 20) / 1000.0;
    }
}

float leggi_solcometro(bool in_movimento)
{
    if (in_movimento)
    {
        return random(100, 1500) / 100.0; // [1.0, 15.0]
    }
    else
    {
        return random(0, 5) / 100.0; // [0.00, 0.05]
    }
}

// Lora & Cifratura
String criptaMessaggio(const char *messaggio)
{
    String criptato = messaggio;
    for (int i = 0; i < criptato.length(); i++)
    {
        criptato[i] ^= CHIAVE_CIFRATURA; // XOR con la chiave
    }
    return criptato;
}

void inviaMessaggioLoRa(const char *msg)
{

    String messaggio = criptaMessaggio(msg); // Cifriamo "OK" o "ALARM"

    lora->beginPacket();
    lora->write(0xFF); // Broadcast
    lora->write(0x01); // Indirizzo mittente
    lora->write(messaggio.length());
    lora->write((const uint8_t *)messaggio.c_str(), messaggio.length());
    lora->endPacket();

    Serial.print("Messaggio LoRa inviato: ");
    Serial.println(messaggio);
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
