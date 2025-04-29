#include <iostream>
#include <fstream>
#include <pigpio.h>
#include <unistd.h> // pour sleep
#include <vector>

#define SPI_CHANNEL 0
#define SPI_SPEED 500000  // 500 kHz
#define DRDY_GPIO 25      // GPIO connecté à DRDY du ADS1220
#define CS_GPIO 8         // GPIO pour CS (Chip Select) manuel si besoin
#define CSV_FILE "mesures.csv"

using namespace std;

unsigned char spiTransfer(int handle, unsigned char data) {
    char txBuf[1] = { (char)data };
    char rxBuf[1] = { 0 };
    spiXfer(handle, txBuf, rxBuf, 1);
    return (unsigned char)rxBuf[0];
}

void resetADS1220(int handle) {
    gpioWrite(CS_GPIO, 0);           // CS LOW
    spiTransfer(handle, 0x06);        // RESET command
    gpioWrite(CS_GPIO, 1);            // CS HIGH
    gpioSleep(PI_TIME_RELATIVE, 0, 100000); // 100 ms pour laisser le temps au reset
}

void startADS1220(int handle) {
    gpioWrite(CS_GPIO, 0);           // CS LOW
    spiTransfer(handle, 0x08);        // START command
    gpioWrite(CS_GPIO, 1);            // CS HIGH
}

int readADS1220Data(int handle) {
    unsigned char rawData[3] = {0};

    gpioWrite(CS_GPIO, 0);            // CS LOW
    spiTransfer(handle, 0x10);         // Envoyer RDATA
    for (int i = 0; i < 3; ++i) {
        rawData[i] = spiTransfer(handle, 0x00);
    }
    gpioWrite(CS_GPIO, 1);            // CS HIGH

    // Combiner les 3 octets en un int32 signé
    int32_t result = (rawData[0] << 16) | (rawData[1] << 8) | rawData[2];

    // Correction du signe (24 bits -> 32 bits)
    if (result & 0x800000) {
        result |= 0xFF000000;
    }

    return result;
}

void waitForDRDY() {
    while (gpioRead(DRDY_GPIO) != 0) {
        time_sleep(0.001); // attendre 1 ms pour éviter de bloquer le CPU
    }
}

int main() {
    if (gpioInitialise() < 0) {
        cerr << "Erreur lors de l'initialisation de pigpio." << endl;
        return -1;
    }

    gpioSetMode(DRDY_GPIO, PI_INPUT);       // DRDY en entrée
    gpioSetPullUpDown(DRDY_GPIO, PI_PUD_UP); // pull-up interne

    gpioSetMode(CS_GPIO, PI_OUTPUT);        // CS en sortie
    gpioWrite(CS_GPIO, 1);                  // CS HIGH au repos

    // Attention : MODE 1 pour ADS1220
    int spiHandle = spiOpen(SPI_CHANNEL, SPI_SPEED, 1); // SPI mode 1
    if (spiHandle < 0) {
        cerr << "Erreur lors de l'ouverture du périphérique SPI." << endl;
        gpioTerminate();
        return -1;
    }

    cout << "Périphérique SPI ouvert avec succès." << endl;

    // Reset et démarrage ADS1220
    resetADS1220(spiHandle);
    startADS1220(spiHandle);

    ofstream fichierCSV(CSV_FILE);
    fichierCSV << "Mesure" << endl;

    const int nombreDeLectures = 50;
    for (int i = 0; i < nombreDeLectures; ++i) {
        waitForDRDY();
        int lecture = readADS1220Data(spiHandle);
        cout << "Lecture #" << i << " : " << lecture << endl;
        fichierCSV << lecture << endl;
    }

    fichierCSV.close();
    spiClose(spiHandle);
    gpioTerminate();

    return 0;
}
