#include <iostream>
#include <pigpio.h>
#include <fstream>
#include <vector>
#include <unistd.h>

// Définir les pins SPI
#define SPI_CS_PIN 8     // Chip select (CS)
#define DRDY_PIN 25      // Pin GPIO pour DRDY (à adapter selon ton montage)
#define ADS1220_SPI_BUS 0 // Bus SPI 0 (sur Raspberry Pi, 0 ou 1)

#define ADS1220_CMD_START_CONVERSION 0x12  // Commande pour démarrer la conversion (selon datasheet)

// Fonction pour attendre que DRDY soit bas (fin de la conversion)
void waitForDRDY() {
    while (gpioRead(DRDY_PIN) == 1) {  // 1 = HIGH, attendre que DRDY passe à LOW
        usleep(1000);  // Petite pause pour éviter une utilisation excessive du CPU
    }
    std::cout << "DRDY est passé à LOW, conversion terminée." << std::endl;
}

// Fonction pour configurer l'ADS1220 (initialisation du SPI et des paramètres)
int setupADS1220() {
    // Initialiser la bibliothèque pigpio
    if (gpioInitialise() < 0) {
        std::cerr << "Erreur d'initialisation de pigpio!" << std::endl;
        return -1;
    }

    // Configurer le pin DRDY comme entrée
    gpioSetMode(DRDY_PIN, PI_INPUT);

    // Configurer le pin CS (Chip Select) comme sortie
    gpioSetMode(SPI_CS_PIN, PI_OUTPUT);
    gpioWrite(SPI_CS_PIN, 1);  // Initialiser CS à 1 (désactiver le périphérique SPI)

    // Configurer le périphérique SPI
    int handle = spiOpen(ADS1220_SPI_BUS, 1000000, 0);  // Bus SPI, vitesse 1 MHz, mode SPI 0
    if (handle < 0) {
        std::cerr << "Erreur lors de l'ouverture du bus SPI!" << std::endl;
        return -1;
    }

    std::cout << "Périphérique SPI ouvert avec succès." << std::endl;

    return handle;
}

// Fonction pour envoyer et recevoir des données SPI
char spiTransfer(int handle, char data) {
    char rxData;
    spiXfer(handle, &data, &rxData, 1);  // Envoyer et recevoir 1 octet
    return rxData;
}

// Fonction pour démarrer la conversion et lire les résultats
int readADS1220Data(int handle) {
    // Envoi de la commande pour démarrer la conversion
    gpioWrite(SPI_CS_PIN, 0);  // Activer l'ADS1220 (CS = 0)
    std::cout << "Envoi de la commande START_CONVERSION." << std::endl;
    spiTransfer(handle, ADS1220_CMD_START_CONVERSION);  // Commande de démarrage
    gpioWrite(SPI_CS_PIN, 1);  // Désactiver l'ADS1220 (CS = 1)

    // Attendre que DRDY soit bas (fin de la conversion)
    waitForDRDY();

    // Lire les résultats de la conversion
    gpioWrite(SPI_CS_PIN, 0);  // Activer l'ADS1220 (CS = 0)
    char data[3];     // Buffer pour les 3 octets de données
    std::cout << "Lecture des données..." << std::endl;
    for (int i = 0; i < 3; ++i) {
        data[i] = spiTransfer(handle, 0x00);  // Lire un octet
    }
    gpioWrite(SPI_CS_PIN, 1);  // Désactiver l'ADS1220 (CS = 1)

    // Combiner les données pour obtenir la valeur sur 24 bits
    int value = (data[0] << 16) | (data[1] << 8) | data[2];

    // Si le bit de signe est à 1, appliquer la compensation (pour les valeurs négatives)
    if (data[0] & 0x80) {
        value -= 0x1000000;
    }

    std::cout << "Données lues : " << value << std::endl;
    return value;
}

// Fonction pour enregistrer les données dans un fichier CSV
void saveDataToCSV(const std::vector<int>& data) {
    std::ofstream file("ads1220_data.csv", std::ios::app);  // Ouvre le fichier en mode ajout
    if (!file) {
        std::cerr << "Impossible d'ouvrir le fichier CSV!" << std::endl;
        return;
    }

    // Écrire les données dans le fichier CSV
    for (const int& value : data) {
        file << value << ", ";
    }
    file << std::endl;

    file.close();
}

int main() {
    // Initialiser l'ADS1220
    int handle = setupADS1220();
    if (handle == -1) {
        return 1;
    }

	// Petite pause pour être sûr que tout est bien initialisé
	sleep(2);

    // Collecter les données et les enregistrer
    std::vector<int> data;
    for (int i = 0; i < 10; ++i) {  // Exemple : lire 10 mesures
        int value = readADS1220Data(handle);
        if (value != -1) {
            std::cerr << "Erreur lors de la lecture des données de l'ADS1220." << std::endl;
			break;  // Arrêter si une erreur se produit
		}
		std::cout << "Lecture " << i + 1 << ": " << value << std::endl;
		data.push_back(value);
		sleep(1);  // Attente d'une seconde entre les lecturesstd::cout << "Lecture " << i + 1 << ": " << value << std::endl;
    }

    // Sauvegarder les données dans un fichier CSV
    saveDataToCSV(data);

    // Libérer les ressources
    spiClose(handle);
    gpioTerminate();
    return 0;
}
