#include <iot_board.h>
#include "EloquentTinyML.h"
#include "ormeggio_etichettati.h"

#define NUMBER_OF_INPUTS 7   
#define NUMBER_OF_OUTPUTS 1  
#define TENSOR_ARENA_SIZE 2*1024  

Eloquent::TinyML::TfLite<NUMBER_OF_INPUTS, NUMBER_OF_OUTPUTS, TENSOR_ARENA_SIZE> ml;


const unsigned long INTERVALLO_OK_MS = 6UL * 60UL * 60UL * 1000UL; // 6 ore
unsigned long lastOkMsgTime = 0;  // Ultimo OK inviato
const char CHIAVE_CIFRATURA = 0x5A; // Chiave di cifratura


// Letture Sensori Simulate
void leggi_accelerometro(float &x, float &y, float &z, bool in_movimento);
void leggi_giroscopio(float &x, float &y, float &z, bool in_movimento);
float leggi_solcometro(bool in_movimento);

// Lora
String criptaMessaggio(const char* messaggio);
void inviaMessaggioLoRa(const char* tipo);


void setup() {
    IoTBoard::init_serial();  
    randomSeed(millis()); 

    // Caricamento modello
    if (!ml.begin(ormeggio_model)) {  
        Serial.println("Errore nell'inizializzazione del modello");
    } else {
        Serial.println("Modello caricato"); 
    }

    // Inizializzazione LoRa
    IoTBoard::init_spi();
    if (!IoTBoard::init_lora()) {
        Serial.println("LoRa Error");
    } else {
        Serial.println("LoRa abilitato");
    }

    lastOkMsgTime = millis();
}

void loop() {
    const int NUM_RILEVAZIONI = 10;
    int count_non_ormeggiata = 0;

    Serial.println("10 rilevazioni in corso...");
    
    for (int i = 0; i < NUM_RILEVAZIONI; i++) {
        
        float input[NUMBER_OF_INPUTS];  
        bool in_movimento = random(0, 2); 

        // Lettura sensori simulati
        leggi_accelerometro(input[0], input[1], input[2], in_movimento);
        leggi_giroscopio(input[3], input[4], input[5], in_movimento);
        input[6] = leggi_solcometro(in_movimento);

        // Predizione 
        float output[NUMBER_OF_OUTPUTS];  
        ml.predict(input, output);  

        // Interpretazione dell'output: < 0.5 => ormeggiata
        bool is_ormeggiata = (output[0] < 0.5);


        if (!is_ormeggiata) {
            count_non_ormeggiata++;
        }

        Serial.print("Rilevazione "); Serial.print(i + 1);
        Serial.print(" - Predizione: "); Serial.print(output[0]);
        Serial.print(" -> ");
        Serial.println(is_ormeggiata ? "Barca Ormeggiata" : "Barca Non Ormeggiata");

        delay(5000);  // 5 secondi di attesa 
    }

  
    Serial.print("Conteggio 'Barca Non Ormeggiata': ");
    Serial.println(count_non_ormeggiata);

    if (count_non_ormeggiata > (NUM_RILEVAZIONI / 2)) {
        Serial.println("La Barca NON è ormeggiata");
        inviaMessaggioLoRa("ALLARME");
    } else {
        Serial.println("La Barca risulta ormeggiata");

        // Controllo se mandare OK
        unsigned long now = millis();
        if (now - lastOkMsgTime > INTERVALLO_OK_MS) {
            inviaMessaggioLoRa("OK");
            lastOkMsgTime = now;
        }

    }

    // Attesa 45 minuti 
    Serial.println("Attesa 45 minuti...");
    delay(45UL * 60UL * 1000UL);
}


String criptaMessaggio(const char* messaggio) {
    String criptato = messaggio;
    for (int i = 0; i < criptato.length(); i++) {
        criptato[i] ^= CHIAVE_CIFRATURA;  // XOR con la chiave
    }
    return criptato;
}

// Letture Sensori Simulate
void leggi_accelerometro(float &x, float &y, float &z, bool in_movimento) {
    if (in_movimento) {
        x = random(-500, 500) / 1000.0;  // [-0.5, 0.5]
        y = random(-500, 500) / 1000.0;
        z = random(900, 980) / 100.0;    // [9.0, 9.8]
    } else {
        x = random(-50, 50) / 1000.0;    // [-0.05, 0.05]
        y = random(-50, 50) / 1000.0;
        z = random(975, 985) / 100.0;    // [9.75, 9.85]
    }
}

void leggi_giroscopio(float &x, float &y, float &z, bool in_movimento) {
    if (in_movimento) {
        x = random(-500, 500) / 1000.0;  // [-0.5, 0.5]
        y = random(-500, 500) / 1000.0;
        z = random(-500, 500) / 1000.0;
    } else {
        x = random(-20, 20) / 1000.0;    // [-0.02, 0.02]
        y = random(-20, 20) / 1000.0;
        z = random(-20, 20) / 1000.0;
    }
}

float leggi_solcometro(bool in_movimento) {
    if (in_movimento) {
        return random(100, 1500) / 100.0;  // [1.0, 15.0]
    } else {
        return random(0, 5) / 100.0;       // [0.00, 0.05]
    }
}


void inviaMessaggioLoRa(const char* tipo) {

    String messaggio = criptaMessaggio(tipo);  // Cifriamo "OK" o "ALARM"

    lora->beginPacket();
    lora->write(0xFF);  // Broadcast
    lora->write(0x01);  // Indirizzo mittente
    lora->write(messaggio.length());
    lora->write((const uint8_t*)messaggio.c_str(), messaggio.length());
    lora->endPacket();
    
    Serial.print("Messaggio LoRa inviato (cifrato): ");
    Serial.println(messaggio);
}