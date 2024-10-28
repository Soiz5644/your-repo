#include "rpi_tca9548a.h"
#include "sgp30_i2c.h"
#include "sensirion_i2c_hal.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <fstream>
#include <cstdlib> // for system()

#define BASELINE_FILE "sgp30_baseline.txt"
#define HOURS_TO_RUN 16

// Function to read baseline from file
bool read_baseline(uint16_t &co2, uint16_t &tvoc) {
    std::ifstream infile(BASELINE_FILE);
    if (infile.good()) {
        infile >> co2 >> tvoc;
        infile.close();
        return true;
    }
    return false;
}

// Function to write baseline to file
void write_baseline(uint16_t co2, uint16_t tvoc) {
    std::ofstream outfile(BASELINE_FILE);
    outfile << co2 << " " << tvoc;
    outfile.close();
}

int main() {
    sensirion_i2c_hal_init();

    rpi_tca9548a tca9548a;
    if (tca9548a.init(0x70) != 0) {
        std::cerr << "Failed to initialize TCA9548A" << std::endl;
        return -1;
    }

    tca9548a.set_channel(2);

    if (sgp30_init() != 0) {
        std::cerr << "Failed to initialize SGP30" << std::endl;
        return -1;
    }

    uint16_t co2_baseline, tvoc_baseline;
    if (read_baseline(co2_baseline, tvoc_baseline)) {
        sgp30_set_baseline(co2_baseline, tvoc_baseline);
    }

    auto last_baseline_time = std::chrono::steady_clock::now();
    int hours_elapsed = 0;

    while (hours_elapsed < HOURS_TO_RUN) {
        uint16_t co2_eq_ppm, tvoc_ppb;
        if (sgp30_read_measurements(&co2_eq_ppm, &tvoc_ppb) != 0) {
            std::cerr << "Error reading SGP30 measurements" << std::endl;
        } else {
            std::cout << "SGP30 tVOC: " << tvoc_ppb << " ppb, CO2eq: " << co2_eq_ppm << " ppm" << std::endl;
        }

        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::hours>(now - last_baseline_time).count() >= 1) {
            uint16_t co2, tvoc;
            if (sgp30_get_baseline(&co2, &tvoc) == 0) {
                write_baseline(co2, tvoc);
                last_baseline_time = now;
                hours_elapsed++;
            }
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    std::cout << "16 hours elapsed. Shutting down the system." << std::endl;
    system("sudo shutdown -h now");

    return 0;
}