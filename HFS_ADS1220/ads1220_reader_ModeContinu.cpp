#include <iostream>
#include <fstream>
#include <pigpio.h>
#include <unistd.h> // pour sleep
#include <cstring>  // pour memset

#define SPI_CHANNEL 0  // SPI0 CE0
#define SPI_SPEED 500000  // 500 kHz, très sûr pour ADS1220

#define CSV_FILE "ads1220_data.csv"

// Commandes ADS1220
#define CMD_RESET     0x06
#define CMD_START     0x08
#define CMD_READ_DATA 0x10

int handle; // SPI handle global

void errorExit(const std::string& message) {
    std::cerr << message << std::endl;
    gpioTerminate();
    exit(1);
}

// Fonction pour envoyer une commande SPI simple (1 octet)
void spiWriteByte(unsigned char data) {
    char txBuf[1];
    txBuf[0] = static_cast<char>(data);
    spiWrite(handle, txBuf, 1);
}

// Fonction pour lire 3 octets de données (24 bits)
int32_t readADCData() {
    char txBuf[3] = {0x00, 0x00, 0x00};
    char rxBuf[3];
    spiXfer(handle, txBuf, rxBuf, 3);

    int32_t result = (static_cast<uint8_t>(rxBuf[0]) << 16) |
                     (static_cast<uint8_t>(rxBuf[1]) << 8) |
                     (static_cast<uint8_t>(rxBuf[2]));

    // Gestion du signe (donnée sur 24 bits, signed int)
    if (result & 0x800000) {
        result |= 0xFF000000;
    }

    return result;
}

// Fonction pour configurer registre 0 (choix des entrées AIN0-AIN1)
void writeRegister0() {
    char txBuf[2];
    txBuf[0] = 0x40; // Commande WRITE to Reg0 (address=0)
    txBuf[1] = 0x00; // MUX = 0000 (AIN0 - AIN1), PGA bypassé
    spiWrite(handle, txBuf, 2);
    gpioDelay(10000); // 10 ms d'attente
}

// Fonction pour configurer registre 1 (mode continu)
void writeRegister1() {
    char txBuf[2];
    txBuf[0] = 0x41; // Commande WRITE to Reg1 (address=1)
    txBuf[1] = 0x40; // CONTINUOUS mode activé
    spiWrite(handle, txBuf, 2);
    gpioDelay(10000); // 10 ms d'attente
}

// Fonction pour lire et afficher le registre 0 (debug)
void readRegister0() {
    char txBuf[2];
    char rxBuf[2];
    txBuf[0] = 0x20; // Commande READ from Reg0
    txBuf[1] = 0x00; // Lecture de 1 registre

    spiXfer(handle, txBuf, rxBuf, 2);
    std::cout << "Registre 0 = 0x" << std::hex << (int)(rxBuf[1]) << std::dec << std::endl;
}

void setupADS1220() {
    // RESET
    spiWriteByte(CMD_RESET);
    gpioDelay(100000); // attendre 100 ms pour laisser le reset se faire

    // Configurer les registres
    writeRegister0(); // AIN0 - AIN1
    writeRegister1(); // Mode continu
}

int main() {
    if (gpioInitialise() < 0) {
        errorExit("Échec de l'initialisation de pigpio.");
    }

    handle = spiOpen(SPI_CHANNEL, SPI_SPEED, 1); // mode=1 pour SPI avec CPOL=0, CPHA=1
    if (handle < 0) {
        errorExit("Erreur ouverture SPI.");
    }

    std::cout << "Périphérique SPI ouvert avec succès." << std::endl;

    setupADS1220();

    // Debug : lecture du registre 0 pour vérif
    readRegister0();

    std::ofstream file(CSV_FILE);
    if (!file.is_open()) {
        errorExit("Erreur ouverture fichier CSV.");
    }

    file << "Index,Raw Value" << std::endl;

    int index = 0;

    while (true) {
        gpioDelay(12000); // Petite attente pour être sûr que la conversion est prête (~12ms)

        // Lire la donnée
        spiWriteByte(CMD_READ_DATA);
        int32_t adcData = readADCData();

        std::cout << "Lecture #" << index << " : " << adcData << std::endl;

        file << index << "," << adcData << std::endl;
        file.flush();

        index++;

        sleep(120); // Attendre 2 minutes avant nouvelle lecture
    }

    file.close();
    spiClose(handle);
    gpioTerminate();

    return 0;
}
