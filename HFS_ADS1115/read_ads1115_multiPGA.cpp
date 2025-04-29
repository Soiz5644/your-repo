#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <thread>
#include <ctime>
#include <unistd.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>

#define ADS1115_ADDRESS 0x48

// Gain (PGA) settings
const float pga_values[] = {6.144, 4.096, 2.048, 1.024, 0.512, 0.256};

// MUX settings for single-ended and differential measurements
const uint16_t mux_settings[] = {
    0x4000, // AIN0
    0x5000, // AIN1
    0x6000, // AIN2
    0x7000, // AIN3
    0x0000, // AIN0 - AIN1
    0x3000  // AIN2 - AIN3
};

const std::string mux_labels[] = {
    "AIN0", "AIN1", "AIN2", "AIN3", "AIN0-AIN1", "AIN2-AIN3"
};

// Fonction pour attendre que la conversion soit terminée
bool wait_for_conversion(int file) {
    while (true) {
        uint8_t reg = 0x01; // Adresse du registre config
        if (write(file, &reg, 1) != 1) {
            std::cerr << "Erreur écriture pour lecture config" << std::endl;
            return false;
        }

        uint8_t config_data[2];
        if (read(file, config_data, 2) != 2) {
            std::cerr << "Erreur lecture config" << std::endl;
            return false;
        }

        uint16_t config_value = (config_data[0] << 8) | config_data[1];
        
        if (config_value & 0x8000) { // Bit OS = 1 -> conversion terminée
            return true;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

// Fonction pour obtenir l'horodatage actuel sous forme de chaîne
std::string current_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_now = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_now), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

int main() {
    const char *i2c_device = "/dev/i2c-1";
    int file = open(i2c_device, O_RDWR);
    if (file < 0) {
        std::cerr << "Erreur ouverture " << i2c_device << std::endl;
        return 1;
    }

    if (ioctl(file, I2C_SLAVE, ADS1115_ADDRESS) < 0) {
        std::cerr << "Erreur accès périphérique I2C" << std::endl;
        return 1;
    }

    std::ofstream fichier("donnees_ads1115.csv");
    fichier << "Horodatage,PGA,Valeur_brute,Scale (V/bit),Tension (V),Canal" << std::endl;

    for (float pga : pga_values) {
        uint16_t pga_config = 0;

        if (pga == 6.144) pga_config = 0x0000;
        else if (pga == 4.096) pga_config = 0x0200;
        else if (pga == 2.048) pga_config = 0x0400;
        else if (pga == 1.024) pga_config = 0x0600;
        else if (pga == 0.512) pga_config = 0x0800;
        else if (pga == 0.256) pga_config = 0x0A00;
        else {
            std::cerr << "PGA non supporté: " << pga << std::endl;
            continue;
        }

        for (size_t i = 0; i < sizeof(mux_settings)/sizeof(mux_settings[0]); ++i) {
            uint16_t config = 0x8000 // OS = 1 (lancer conversion)
                            | mux_settings[i] // sélection du MUX
                            | pga_config // sélection du gain
                            | 0x0100 // mode single-shot
                            | 0x0003; // 128 SPS (vitesse échantillonnage)

            uint8_t config_data[3] = {0x01, (uint8_t)(config >> 8), (uint8_t)(config & 0xFF)};
            if (write(file, config_data, 3) != 3) {
                std::cerr << "Erreur écriture config" << std::endl;
                return 1;
            }

            if (!wait_for_conversion(file)) {
                std::cerr << "Erreur en attente de conversion" << std::endl;
                return 1;
            }

            // Lecture de la valeur convertie
            uint8_t reg = 0x00;
            if (write(file, &reg, 1) != 1) {
                std::cerr << "Erreur écriture pour lecture résultat" << std::endl;
                return 1;
            }

            uint8_t data[2];
            if (read(file, data, 2) != 2) {
                std::cerr << "Erreur lecture résultat" << std::endl;
                return 1;
            }

            int16_t value = (data[0] << 8) | data[1];

            // Calcul du scale
            float scale = pga / 32768.0f;
            float voltage = value * scale;

            fichier << current_timestamp() << ","
                    << pga << ","
                    << value << ","
                    << scale << ","
                    << voltage << ","
                    << mux_labels[i]
                    << std::endl;
        }
    }

    close(file);
    fichier.close();

    std::cout << "Lecture terminée et sauvegardée dans donnees_ads1115.csv" << std::endl;
    return 0;
}
