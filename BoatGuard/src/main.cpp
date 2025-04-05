#include <iot_board.h>
#include "EloquentTinyML.h"
#include "Sensors_AI/ormeggio_etichettati.h"
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLEAdvertisedDevice.h>
#include <BLEScan.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include "BLEUtils.h"
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
int counterBatteria = 0;
int livelloBatteria;

const char targa[] = {'A', 'B', '1', '2', '3', 'X', 'Y'};
const char targaGabbiotto[] = {'A', 'B', '1', '2', '3', 'X', 'Y'};

extern bool isConfigurated;
extern bool isNearMe;
extern bool isConnected;

extern void setup();

Eloquent::TinyML::TfLite<NUMBER_OF_INPUTS, NUMBER_OF_OUTPUTS, TENSOR_ARENA_SIZE> ml;

// Variabili posizione iniziale
static float posX = 0.0;
static float posY = 0.0;
static float direzione = 0.0; // Direzione in radianti

StatoBarca stato_attuale = ORMEGGIATA; // Stato iniziale della barca

void onReceive(LoRaMesh_message_t message);

void setCostants();
BLEClient *createBLEClient();

// Rilevazioni barca
bool barcaOrmeggiata();
void aggiornaPosizioneBarca(float deltaTimeSec);
void startAdvertising();
bool inviaMessaggioLoRa(const char targa_destinatario[7], LoRaMesh_payload_t payload);

void setup()
{
    IoTBoard::init_serial();
    randomSeed(millis());

    Serial.begin(115200);
    setCostants();

    BLEClient *pClient = createBLEClient();

    if (isConfigurated)
    {
        Serial.println("Informazioni già configurate");
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
    if (!LoRaMesh::init(targa, onReceive))
    {
        Serial.println("Errore nell'avvio di LoRa");
        ESP.restart();
    }

    livelloBatteria = random(50, 100);
    display->println("Livello batteria corrente: " + String(livelloBatteria) + "%");
    display->display();

    lastOkMsgTime = millis();
    payload.stato = ORMEGGIATA;
    payload.livello_batteria = livelloBatteria;
}

void loop()
{
    switch (stato_attuale)
    {

    case ORMEGGIATA:
    {
        // Verifico se la barca risulta ormeggiata o meno
        bool ormeggiata = barcaOrmeggiata();
        if (!ormeggiata)
        {
            Serial.println("La Barca risulta NON è ormeggiata");

            stato_attuale = RUBATA;

            // Aggiorno la posizione
            aggiornaPosizioneBarca(5.0);
        }
        else
        {
            Serial.println("La Barca risulta ormeggiata");

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
                Serial.print("In attesa durante ORMEGGIO");
                LoRaMesh::update();
                delay(5000);
            }
        }
    }
    break;

    case RUBATA:
    {
        // Stato di allarme con aggiornamento posizione

        aggiornaPosizioneBarca(5.0);
        delay(5000);
    }
    case IN_MOVIMENTO:
    {
        printf("La Barca è in uso da parte del proprietario\n");
    }
    break;
    }
}

void onReceive(LoRaMesh_message_t message)
{
    display->clearDisplay();
    display->setCursor(0, 0);
    display->printf("Targa mittente: ");
    for (int i = 0; i < TARGA_LEN; i++)
    {
        display->printf("%c", message.targa_mittente[i]);
    }
    display->printf("\n");
    display->printf("Id Messaggio: %d\n", (message.message_id));
}

bool inviaMessaggioLoRa(const char targa_destinatario[7], LoRaMesh_payload_t payload)
{
    // Invio messaggio di allerta`
    int ret = LoRaMesh::sendMessage(targaGabbiotto, payload);
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

BLEClient *createBLEClient()
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

void aggiornaPosizioneBarca(float deltaTimeSec, LoRaMesh_payload_t payload)
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
    payload.stato = stato_attuale;
    payload.posX = posX;
    payload.posY = posY;
    payload.direzione = direzione;

    inviaMessaggioLoRa(targaGabbiotto, payload);

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
