#include <iostream>
#include <fstream>
#include <unistd.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <cstdint>
#include <chrono>
#include <thread>
#include <ctime>

#define ADS1115_ADDRESS 0x48
#define I2C_DEV "/dev/i2c-1"
#define SENSITIVITY_MV_PER_W_CM2 24.21

int main() {
    int file;
    if ((file = open(I2C_DEV, O_RDWR)) < 0) {
        std::cerr << "Erreur ouverture I2C" << std::endl;
        return 1;
    }

    if (ioctl(file, I2C_SLAVE, ADS1115_ADDRESS) < 0) {
        std::cerr << "Erreur ioctl" << std::endl;
        return 1;
    }

    // Ouvrir le fichier CSV en mode ajout
    std::ofstream fichier("flux_thermique.csv", std::ios::app);

    if (!fichier.is_open()) {
        std::cerr << "Erreur ouverture fichier CSV" << std::endl;
        return 1;
    }

    // Écrire l'en-tête du fichier CSV si vide
    if (fichier.tellp() == 0) {
        fichier << "Horodatage,Tension (V),Flux thermique (W/cm²)" << std::endl;
    }

    while (true) {
        // Configurer l'ADS1115 pour une lecture en mode single-shot
        uint8_t config[3];
        config[0] = 0x01; // registre de config

        config[1] =
			0b11000011; // MUX=000 (AIN0-AIN1), PGA=001 (±2.048 V)
        config[2] =
            0b11100011; // DR=111 (860SPS), comparator off

        if (write(file, config, 3) != 3) {
            std::cerr << "Erreur d'écriture de config" << std::endl;
            return 1;
        }

        usleep(2000); // attendre conversion (~2 ms pour 860 SPS)

        // Lire la conversion
        uint8_t reg[1] = {0x00};
        if (write(file, reg, 1) != 1) {
            std::cerr << "Erreur pointeur registre" << std::endl;
            return 1;
        }

        uint8_t data[2];
        if (read(file, data, 2) != 2) {
            std::cerr << "Erreur lecture conversion" << std::endl;
            return 1;
        }

        int16_t raw = (data[0] << 8) | data[1];
		float voltage = raw * 2.048f / 32768.0f;
        float heat_flux = (voltage * 1000.0f) / SENSITIVITY_MV_PER_W_CM2; // en W/cm²

        // Récupérer l'heure actuelle pour l'horodatage
        auto now = std::chrono::system_clock::now();
        std::time_t horodatage = std::chrono::system_clock::to_time_t(now);
        char buffer[100];
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", std::localtime(&horodatage));

        // Afficher les résultats sur la console
        std::cout << buffer
                  << " | Tension : " << voltage << " V"
                  << " | Flux de chaleur : " << heat_flux << " W/cm²" << std::endl;

        // Écrire les données dans le fichier CSV
        fichier << buffer << ","
                << voltage << ","
                << heat_flux << std::endl;

        fichier.flush(); // Assurer que les données sont bien écrites dans le fichier

        // Attendre 1 seconde avant la prochaine lecture
        std::this_thread::sleep_for(std::chrono::seconds(60));
    }

    close(file);
    fichier.close(); // Fermer le fichier CSV
    return 0;
}
