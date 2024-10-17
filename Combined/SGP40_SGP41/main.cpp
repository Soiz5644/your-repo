#include "rpi_tca9548a.h"
#include "sgp40_i2c.h"
#include "sgp41_i2c.h"
#include "sensirion_i2c_hal.h"
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    // Initialize I2C HAL
    sensirion_i2c_hal_init();

    // Initialize TCA9548A at the default I2C address (0x70)
    rpi_tca9548a tca9548a;
    if (tca9548a.init(0x70) != 0) {
        std::cerr << "Failed to initialize TCA9548A" << std::endl;
        return -1;
    }

    // SGP40 on channel 0
    tca9548a.set_channel(0);
    uint16_t serial_number_40[3];
    if (sgp40_get_serial_number(serial_number_40, 3) != 0) {
        std::cerr << "Error getting SGP40 serial number" << std::endl;
    } else {
        std::cout << "SGP40 serial number: " << serial_number_40[0] << serial_number_40[1] << serial_number_40[2] << std::endl;
    }

    // SGP41 on channel 1
    tca9548a.set_channel(1);
    uint16_t serial_number_41[3];
    if (sgp41_get_serial_number(serial_number_41) != 0) {
        std::cerr << "Error getting SGP41 serial number" << std::endl;
    } else {
        std::cout << "SGP41 serial number: " << serial_number_41[0] << serial_number_41[1] << serial_number_41[2] << std::endl;
    }

    // Add a 10-second delay before starting any measurements
    std::cout << "Waiting for 10 seconds before starting measurements..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(10));

    uint16_t sraw_voc_40, sraw_voc_41, sraw_nox_41;
    uint16_t default_rh = 0x8000; // Default relative humidity
    uint16_t default_t = 0x6666;  // Default temperature

    while (true) {
        // Measure SGP40
        tca9548a.set_channel(0);
        if (sgp40_measure_raw_signal(default_rh, default_t, &sraw_voc_40) != 0) {
            std::cerr << "Error measuring SGP40 raw signal" << std::endl;
        } else {
            std::cout << "SGP40 SRAW VOC: " << sraw_voc_40 << std::endl;
        }

        // Measure SGP41
        tca9548a.set_channel(1);
        if (sgp41_measure_raw_signals(default_rh, default_t, &sraw_voc_41, &sraw_nox_41) != 0) {
            std::cerr << "Error measuring SGP41 raw signals" << std::endl;
        } else {
            std::cout << "SGP41 SRAW VOC: " << sraw_voc_41 << " SRAW NOX: " << sraw_nox_41 << std::endl;
        }

        // Add a delay between measurements
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}