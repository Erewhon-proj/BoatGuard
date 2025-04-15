#include <iot_board.h>
#include "EloquentTinyML.h"
#include "Sensors_AI/ormeggio_etichettati.h"
#include <BLEDevice.h>
#include <BLE/BLEUtils.h>
#include <BLEServer.h>
#include <BLEAdvertisedDevice.h>
#include <BLEScan.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include "BLE/BLEUtils.h"
#include "Sensors_AI/sensori.h"
#include "costants.h"
#include <cstdint>
#include <iot_board.h>
#include "LoRaMesh/state_t.h"
#include "LoRaMesh/LoRaMesh.h"

BLEServer *pServer = NULL;
BLECharacteristic *pCharacteristic = NULL;

LoRaMesh_payload_t payload;
String s = "";

const char targaGabbiotto[] = {'A', 'B', '1', '2', '3', 'X', 'Y'};
char targa[7];

extern bool isConfigurated;
extern bool isNearMe;
extern bool isConnected;

extern void setup();

Eloquent::TinyML::TfLite<NUMBER_OF_INPUTS, NUMBER_OF_OUTPUTS, TENSOR_ARENA_SIZE> ml;

// Variabili posizione iniziale
static float posX = 0.0;
static float posY = 0.0;
static float direzione = 0.0; 

StatoBarca stato_attuale = ORMEGGIATA; // Stato iniziale della barca

void onReceive(LoRaMesh_message_t message);

void setCostants();
BLEClient *createBLEClient();

void aggiornaPosizioneBarca(float deltaTimeSec);
bool barcaOrmeggiata();
void startAdvertising();
bool inviaMessaggioLoRa(const char targa_destinatario[7], LoRaMesh_payload_t payload);

void setup()
{
    IoTBoard::init_serial();
    randomSeed(millis());

    Serial.begin(115200);
    setCostants();

    preferences.begin("config", false);
    Serial.println(preferences.getString("targa").c_str());
    strncpy(targa, preferences.getString("targa").c_str(), sizeof(targa));
    targa[sizeof(targa)] = '\0';
    Serial.println("Targa configurata");
    Serial.println(targa);

    //startAdvertising();
    //targa[0] = '\0';

    if (targa[0] == '\0')
    {
        Serial.println("Targa non configurata");
        display->println("Targa non configurata");
        display->display();
        startAdvertising();
        delay(2000);
    }

    display->print("targa: ");
    display->println(targa);

    // Caricamento modello
    if (!ml.begin(ormeggio_model))
    {
        Serial.println("Errore nell'inizializzazione del modello");
    }

    // Inizializzazione LoRa
    if (!LoRaMesh::init(targa, onReceive))
    {
        Serial.println("Errore nell'avvio di LoRa... Riavvio...");
        delay(5000);
    }

    lastOkMsgTime = millis();
}

void handleSwitchState()
{
    switch (stato_attuale)
    {
    case ORMEGGIATA:
    {
        // Verifico se la barca risulta ormeggiata o meno
        bool ormeggiata = barcaOrmeggiata();
        if (!ormeggiata)
        {
            stato_attuale = RUBATA;
            // Aggiorno la posizione
            aggiornaPosizioneBarca(5.0);
            Serial.println("La Barca non è più ormeggiata");
        }
        else
        {
            Serial.println("La Barca risulta ancora ormeggiata");

            // Invia posizione stimata via LoRa
            payload.stato = ORMEGGIATA;
            payload.posX = 0;
            payload.posY = 0;
            payload.direzione = 0;

            unsigned long now = millis();

            // Invio "OK" se è passato INTERVALLO_OK_MS

            if (now - lastOkMsgTime > INTERVALLO_OK_MS)
            {
                lastOkMsgTime = now;
                inviaMessaggioLoRa(targaGabbiotto, payload);
                Serial.println("Inviato OK");
            }

            while (millis() - now < DURATA_CICLO_MS)
            {
                Serial.println("In attesa durante ORMEGGIO");
                LoRaMesh::update();
                delay(5000);
            }
        }
    }
    break;
    case RUBATA:
    {
        LoRaMesh::update();
       
        // Controllo se è il proprietario e portarsi la nave
        // Tramite il telefono del proprietario
       
        createBLEClient();

        delay(5000);

        if (getNearMe())
        {
            Serial.println("Proprietario rilevato vicino -> Cambio stato in IN_MOVIMENTO");
            stato_attuale = IN_MOVIMENTO;
            break;
        }

        aggiornaPosizioneBarca(5.0);
        delay(5000);
    }
    break;
    case IN_MOVIMENTO:
    {
        Serial.println("La Barca è in uso da parte del proprietario\n");
        delay(10000);
    }
    break;
    }
}

void loop()
{
    LoRaMesh::update();
    Serial.print("Stato attuale: ");
    Serial.print(statoToString(stato_attuale));
    handleSwitchState();
}

void onReceive(LoRaMesh_message_t message)
{
    Serial.println("main/onRecive");

    Serial.println(message.message_id);
    Serial.println(message.payload.direzione);
    Serial.println(message.payload.posX);
    Serial.println(message.payload.posY);
    Serial.println(message.payload.stato);
    Serial.println(message.payload.stato);
    Serial.println(message.payload.message_sequence);

    stato_attuale = message.payload.stato;
    Serial.print("Cambio stato in: ");
    Serial.print(statoToString(stato_attuale));

    /*
    if (message.payload.message_sequence == 0 && message.payload.posX == 0 && message.payload.direzione == 0)
    {
        Serial.println("stato");
        Serial.println(message.payload.stato);
    }
        */
}

bool inviaMessaggioLoRa(const char targa_destinatario[7], LoRaMesh_payload_t payload)
{
    // Invio messaggio di allerta
    int ret = LoRaMesh::sendMessage(targaGabbiotto, payload);
    LoRaMesh::update();
    if (ret == LORA_MESH_MESSAGE_QUEUE_FULL)
    {
        Serial.println("Errore - La coda è piena");
        return false;
    }
    else if (ret == LORA_MESH_MESSAGE_SENT_SUCCESS)
    {
        Serial.println("Il messaggio è stato mandato con successo");
        return true;
    }
}

void setCostants()
{
    IoTBoard::init_serial();
    IoTBoard::init_leds();
    IoTBoard::init_display();
    BLEDevice::init("Esp32");

    // isConfigurated
    preferences.begin("config", true);
    isConfigurated = preferences.isKey("targa");
    preferences.end();

    // isNearMe
    isNearMe = false;
}

BLEClient *createBLEClient()
{
    Serial.println("Ckech NearMe");
    display->println("Ckech NearMe");
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
    return pClient;
}

// Rilevazioni barca
bool barcaOrmeggiata()
{
    const int NUM_RILEVAZIONI = 10;
    int count_non_ormeggiata = 0;

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
    Serial.println("Aggiorna posizione barca");
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
    payload.stato = stato_attuale;
    payload.posX = posX;
    payload.posY = posY;
    payload.direzione = direzione;

    inviaMessaggioLoRa(targaGabbiotto, payload);
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
