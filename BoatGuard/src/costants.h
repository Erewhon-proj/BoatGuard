#define SERVICE_UUID "12345678-1234-1234-1234-123456789012"
#define CHARACTERISTIC_UUID "87654321-4321-4321-4321-210987654321"
#define BLE_periferica 1
#define NUMBER_OF_INPUTS 7
#define NUMBER_OF_OUTPUTS 1
#define TENSOR_ARENA_SIZE 2 * 1024

const char *REQUEST_CONNECTION_MESSAGE = "REQUEST_CONNECTION";
const char *CONFIRM_CONNECTION_MESSAGE = "CONFIRM_CONNECTION";

//const unsigned long INTERVALLO_OK_MS = 6UL * 60UL * 60UL * 1000UL; // 6 ore
const unsigned long INTERVALLO_OK_MS = 10UL * 1000UL; // 10 secondi
const unsigned long sleepTime = 45UL * 60UL * 1000UL;              // 6 ore
unsigned long lastOkMsgTime = 0;                                   // Ultimo OK inviato
//const unsigned long DURATA_CICLO_MS = 45UL * 60UL * 1000UL; // 45 minuti
const unsigned long DURATA_CICLO_MS = 5UL * 1000UL; // 5 secondi