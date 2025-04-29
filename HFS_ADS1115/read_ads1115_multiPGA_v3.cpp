#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <signal.h>

#define I2C_DEVICE "/dev/i2c-1"
#define ADS1115_ADDR 0x48
#define CONFIG_REG 0x01
#define CONVERSION_REG 0x00

// Définitions pour les PGA
typedef struct {
    double pga;
    uint16_t config;
    double scale;
} pga_config_t;

pga_config_t pgas[] = {
    {6.144, 0x0000, 6.144 / 32768.0},
    {4.096, 0x0200, 4.096 / 32768.0},
    {2.048, 0x0400, 2.048 / 32768.0},
    {1.024, 0x0600, 1.024 / 32768.0},
    {0.512, 0x0800, 0.512 / 32768.0},
    {0.256, 0x0A00, 0.256 / 32768.0}
};

uint8_t channels[] = {0, 1, 2, 3, 4, 5}; // 0: AIN0, 1: AIN1, 2: AIN2, 3: AIN3, 4: AIN0-AIN1, 5: AIN2-AIN3
const char* channel_names[] = {"AIN0", "AIN1", "AIN2", "AIN3", "AIN0-AIN1", "AIN2-AIN3"};

int fd = -1;
FILE *file = NULL;
volatile sig_atomic_t stop = 0;

// Gestion propre de Ctrl+C
void handle_sigint(int sig) {
    stop = 1;
}

// Obtenir l'horodatage actuel
void get_timestamp(char* buffer, size_t size) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", t);
}

// Configurer le PGA
int set_pga(int fd, double pga_value, uint8_t channel) {
    uint16_t mux;
    switch (channel) {
        case 0: mux = 0x4000; break; // AIN0
        case 1: mux = 0x5000; break; // AIN1
        case 2: mux = 0x6000; break; // AIN2
        case 3: mux = 0x7000; break; // AIN3
        case 4: mux = 0x0000; break; // AIN0-AIN1
        case 5: mux = 0x3000; break; // AIN2-AIN3
        default: return -1;
    }

    uint16_t config = 0x8000 | mux | 0x0100 | 0x0083; // Start single conversion, 128SPS, single-shot mode
    for (int i = 0; i < sizeof(pgas)/sizeof(pgas[0]); i++) {
        if (pgas[i].pga == pga_value) {
            config |= pgas[i].config;
            uint8_t buffer[3];
            buffer[0] = CONFIG_REG;
            buffer[1] = config >> 8;
            buffer[2] = config & 0xFF;
            if (write(fd, buffer, 3) != 3) {
                perror("[ERREUR] Ecriture config");
                return -1;
            }
            return 0;
        }
    }
    fprintf(stderr, "[ERREUR] PGA non supporté: %.3f\n", pga_value);
    return -1;
}

// Lire la conversion
int16_t read_conversion(int fd) {
    uint8_t reg = CONVERSION_REG;
    if (write(fd, &reg, 1) != 1) {
        perror("[ERREUR] Sélection registre conversion");
        return -1;
    }
    usleep(12000); // Attendre la conversion (12 ms pour 128SPS)

    uint8_t data[2];
    if (read(fd, data, 2) != 2) {
        perror("[ERREUR] Lecture conversion");
        return -1;
    }
    return (data[0] << 8) | data[1];
}

int main() {
    signal(SIGINT, handle_sigint);

    fd = open(I2C_DEVICE, O_RDWR);
    if (fd < 0) {
        perror("[ERREUR] Ouverture I2C");
        return 1;
    }

    if (ioctl(fd, I2C_SLAVE, ADS1115_ADDR) < 0) {
        perror("[ERREUR] Accès I2C");
        close(fd);
        return 1;
    }

    file = fopen("donnees_ads1115.csv", "w");
    if (!file) {
        perror("[ERREUR] Ouverture fichier CSV");
        close(fd);
        return 1;
    }

    fprintf(file, "Horodatage,PGA,Valeur_brute,Scale (V/bit),Tension (V),Canal\n");

    printf("[INFO] Début des mesures... (Ctrl+C pour arrêter)\n");

    while (!stop) {
        for (int p = 0; p < sizeof(pgas)/sizeof(pgas[0]); p++) {
            double pga = pgas[p].pga;
            double scale = pgas[p].scale;

            for (int c = 0; c < sizeof(channels)/sizeof(channels[0]); c++) {
                if (set_pga(fd, pga, channels[c]) != 0) {
                    continue;
                }

                int16_t raw = read_conversion(fd);
                if (raw == -1) {
                    continue;
                }

                char timestamp[64];
                get_timestamp(timestamp, sizeof(timestamp));

                double voltage = raw * scale;

                fprintf(file, "%s,%.3f,%d,%.10g,%.6f,%s\n",
                        timestamp, pga, raw, scale, voltage, channel_names[c]);
                fflush(file);
            }
        }
        sleep(120); // Pause de 2 min entre deux séries
    }

    fclose(file);
    close(fd);

    printf("\n[INFO] Lecture terminée. Résultats dans donnees_ads1115.csv\n");
    return 0;
}
