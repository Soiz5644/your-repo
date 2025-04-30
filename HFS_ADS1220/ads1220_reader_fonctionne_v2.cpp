#include <iostream>
#include <fstream>
#include <pigpio.h>
#include <unistd.h> // pour sleep
#include <cstring>  // pour memset

#define SPI_CHANNEL 0  // SPI0 CE0
#define SPI_SPEED 500000  // 500 kHz, très sûr pour ADS1220

#define CSV_FILE "ads1220_data.csv"

// Commandes ADS1220
#define CMD_RESET   0x06
#define CMD_START   0x08
#define CMD_READ_DATA 0x10
#define CMD_WREG    0x40

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

// Fonction pour écrire dans les registres
void writeRegister(uint8_t reg, uint8_t value) {
    char txBuf[2];
    txBuf[0] = CMD_WREG | (reg << 2); // Commande WREG avec décalage pour l'adresse du registre
    txBuf[1] = value; // Valeur à écrire
    spiWrite(handle, txBuf, 2);
    gpioDelay(1000); // Petit délai pour assurer l'écriture
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
    gpioDelay(5000); // attendre 5 ms pour laisser le reset se faire

    // Configurer les registres pour une application spécifique
    // Exemple : AINP = AIN1, AINN = AIN0, gain = 8, PGA activé
    writeRegister(0x00, 0x61); // Register 0: MUX = 0001 (AIN1-AIN0), Gain = 8, PGA activé
    writeRegister(0x01, 0x04); // Register 1: DR = 20 SPS, mode normal, conversion continue
    writeRegister(0x02, 0x40); // Register 2: Référence interne, pas de rejet 50/60 Hz
    writeRegister(0x03, 0x00); // Register 3: Pas d'IDAC utilisé
}

int main() {
    if (gpioInitialise() < 0) {
        errorExit("Échec de l'initialisation de pigpio.");
    }

    handle = spiOpen(SPI_CHANNEL, SPI_SPEED, 1); // mode=1 pour SPI
    if (handle < 0) {
        errorExit("Erreur ouverture SPI.");
    }

    std::cout << "Périphérique SPI ouvert avec succès." << std::endl;

    setupADS1220();

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
        gpioDelay(50000); // 50 ms

        // Lire la donnée
        spiWriteByte(CMD_READ_DATA);
        int32_t adcData = readADCData();

        std::cout << "Lecture #" << index << " : " << adcData << std::endl;

        file << index << "," << adcData << std::endl;
        file.flush();

        index++;

        sleep(120); // Attendre 2 min avant nouvelle lecture
    }

    file.close();
    spiClose(handle);
    gpioTerminate();

    return 0;
}