#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <iostream>

using namespace std;

int main() {
    // Initialisation de wiringPi
    if (wiringPiSetup() == -1) {
        cerr << "Erreur d'initialisation de wiringPi." << endl;
        return 1;
    }

    // Adresse I2C du périphérique (ex : 0x48 pour ADS1115 ou TMP102)
    int deviceAddress = 0x48;

    // Ouvrir la connexion I2C
    int fd = wiringPiI2CSetup(deviceAddress);
    if (fd == -1) {
        cerr << "Erreur lors de l'ouverture de l'I2C." << endl;
        return 1;
    }

    // Sélection du registre 0x00 (souvent registre de conversion ou de température)
    if (wiringPiI2CWrite(fd, 0x00) == -1) {
        cerr << "Erreur lors de la sélection du registre." << endl;
        return 1;
    }

    // Lecture de 16 bits : lecture du registre 0x00 (attention à l'ordre des octets)
    int msb = wiringPiI2CRead(fd);
    int lsb = wiringPiI2CRead(fd);

    if (msb == -1 || lsb == -1) {
        cerr << "Erreur de lecture I2C." << endl;
        return 1;
    }

    // Assemblage des deux octets (MSB d'abord, comme souvent sur les ADC)
    int16_t value = (msb << 8) | lsb;

    cout << "Valeur lue du périphérique : " << value << endl;

    return 0;
}
