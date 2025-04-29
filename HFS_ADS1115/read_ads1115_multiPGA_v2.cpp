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
#include <cmath> // Pour fabs()

#define ADS1115_ADDRESS 0x48

const float pga_values[] = {6.144, 4.096, 2.048, 1.024, 0.512, 0.256};

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

bool wait_for_conversion(int file) {
    while (true) {
        uint8_t reg = 0x01;
        if (write(file, &reg, 1) != 1) {
            std::cerr << "[ERREUR] Impossible d'écrire pour lire config" << std::endl;
            return false;
        }

        uint8_t config_data[2];
        if (read(file, config_data, 2) != 2) {
            std::cerr << "[ERREUR] Impossible de lire config" << std::endl;
            return false;
        }

        uint16_t config_value = (config_data[0] << 8) | config_data[1];
        if (config_value & 0x8000) {
            return true;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

std::string current_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_now = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_now), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

uint16_t get_pga_config(float pga) {
    // Fonction pour éviter de comparer directement des floats
    if (std::fabs(pga - 6.144) < 0.01) return 0x0000;
    if (std::fabs(pga - 4.096) < 0.01) return 0x0200;
    if (std::fabs(pga - 2.048) < 0.01) return 0x0400;
    if (std::fabs(pga - 1.024) < 0.01) return 0x0600;
    if (std::fabs(pga - 0.512) < 0.01) return 0x0800;
    if (std::fabs(pga - 0.256) < 0.01) return 0x0A00;
    return 0xFFFF; // valeur d'erreur
}

int main() {
    const char *i2c_device = "/dev/i2c-1";
    int file = open(i2c_device, O_RDWR);
    if (file < 0) {
        std::cerr << "[ERREUR] Ouverture I2C échouée" << std::endl;
        return 1;
    }

    if (ioctl(file, I2C_SLAVE, ADS1115_ADDRESS) < 0) {
        std::cerr << "[ERREUR] Sélection du périphérique I2C échouée" << std::endl;
        return 1;
    }

    std::ofstream fichier("donnees_ads1115.csv");
    if (!fichier) {
        std::cerr << "[ERREUR] Impossible de créer le fichier CSV" << std::endl;
        return 1;
    }
    fichier << "Horodatage,PGA,Valeur_brute,Scale (V/bit),Tension (V),Canal" << std::endl;

    for (float pga : pga_values) {
        uint16_t pga_config = get_pga_config(pga);

        if (pga_config == 0xFFFF) {
            std::cerr << "[ERREUR] PGA non supporté: " << pga << std::endl;
            continue;
        }

        for (size_t i = 0; i < sizeof(mux_settings)/sizeof(mux_settings[0]); ++i) {
            uint16_t config = 0x8000
                            | mux_settings[i]
                            | pga_config
                            | 0x0100
                            | 0x0003;

            uint8_t config_data[3] = {0x01, (uint8_t)(config >> 8), (uint8_t)(config & 0xFF)};
            if (write(file, config_data, 3) != 3) {
                std::cerr << "[ERREUR] Ecriture config échouée pour PGA " << pga << ", canal " << mux_labels[i] << std::endl;
                continue;
            }

            if (!wait_for_conversion(file)) {
                std::cerr << "[ERREUR] Timeout attente conversion pour PGA " << pga << ", canal " << mux_labels[i] << std::endl;
                continue;
            }

            uint8_t reg = 0x00;
            if (write(file, &reg, 1) != 1) {
                std::cerr << "[ERREUR] Ecriture demande lecture résultat échouée" << std::endl;
                continue;
            }

            uint8_t data[2];
            if (read(file, data, 2) != 2) {
                std::cerr << "[ERREUR] Lecture résultat échouée" << std::endl;
                continue;
            }

            int16_t value = (data[0] << 8) | data[1];
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

    std::cout << "[INFO] Lecture terminée. Résultats dans donnees_ads1115.csv" << std::endl;
    return 0;
}
