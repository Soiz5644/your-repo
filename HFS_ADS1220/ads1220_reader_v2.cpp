#include <iostream>
#include <fstream>
#include <pigpio.h>
#include <unistd.h> // pour sleep
#include <cstring>  // pour memset

#define SPI_CHANNEL 0  // SPI0 CE0
#define SPI_SPEED 100000  // 100 kHz, très sûr pour ADS1220

#define CSV_FILE "ads1220_data.csv"

// Commandes ADS1220
#define CMD_RESET   0x06
#define CMD_START   0x08
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

void setupADS1220() {
    // RESET
    spiWriteByte(CMD_RESET);
    gpioDelay(100000); // attendre 100 ms pour laisser le reset se faire

    // Config minimale :
    // - Register 0 : MUX[3:0] = 0000 (AIN0-AIN1)
    // - PGA bypassé
    // - Gain = 1
    // - Mode = normal
    // - DR = 20 SPS (par défaut)
    // donc on peut ne pas toucher aux registres pour l'instant
}

// Lire registre 0 (contient le MUX)
void readRegister0() {
    char tx[2] = {0x20, 0x00}; // 0x20 = RREG(0, 0)
    char rx[2] = {0, 0};
    spiXfer(handle, tx, rx, 2);
    std::cout << "Registre 0 = 0x" << std::hex << (int)rx[1] << std::dec << std::endl;
}

int main() {
    if (gpioInitialise() < 0) {
        errorExit("Échec de l'initialisation de pigpio.");
    }

    handle = spiOpen(SPI_CHANNEL, SPI_SPEED, 1); // mode=0 pour SPI
    if (handle < 0) {
        errorExit("Erreur ouverture SPI.");
    }

    std::cout << "Périphérique SPI ouvert avec succès." << std::endl;

    setupADS1220();

    readRegister0();

    std::ofstream file(CSV_FILE);
    if (!file.is_open()) {
        errorExit("Erreur ouverture fichier CSV.");
    }

    file << "Index,Raw Value" << std::endl;

    int index = 0;

    while (true) {
        // Lancer une conversion
        spiWriteByte(CMD_START);

        // Attendre un peu (temps de conversion du ADS1220 ~ 50 ms max à basse vitesse)
        gpioDelay(12000); // 12 ms

        // Lire la donnée
        spiWriteByte(CMD_READ_DATA);
        int32_t adcData = readADCData();

        std::cout << "Lecture #" << index << " : " << adcData << std::endl;

        file << index << "," << adcData << std::endl;
        file.flush();

        index++;

        sleep(120); // Attendre 2min avant nouvelle lecture
    }

    file.close();
    spiClose(handle);
    gpioTerminate();

    return 0;
}
