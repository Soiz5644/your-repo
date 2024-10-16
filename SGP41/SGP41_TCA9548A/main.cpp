#include "rpi_tca9548a.h"
#include "sgp41_i2c.h"
#include "sensirion_i2c_hal.h"
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    sensirion_i2c_hal_init();

    rpi_tca9548a tca9548a;
    if (tca9548a.init(0x70) != 0) {
        std::cerr << "Failed to initialize TCA9548A" << std::endl;
        return -1;
    }

    // SGP41 on channel 1
    tca9548a.set_channel(1);

    uint16_t serial_number[3];
    int16_t error = sgp41_get_serial_number(serial_number);
    if (error) {
        std::cerr << "Error getting SGP41 serial number: " << error << std::endl;
    } else {
        std::cout << "SGP41 serial number: " 
                  << serial_number[0] << serial_number[1] << serial_number[2] 
                  << std::endl;
    }

    // Add a delay before the first measurement
    std::this_thread::sleep_for(std::chrono::seconds(1));

    uint16_t sraw_voc, sraw_nox;
    uint16_t default_rh = 0x8000; // Default relative humidity
    uint16_t default_t = 0x6666;  // Default temperature

    // Execute conditioning for 10 seconds
    std::cout << "Conditioning for 10 seconds..." << std::endl;
    error = sgp41_execute_conditioning(default_rh, default_t, &sraw_voc);
    if (error) {
        std::cerr << "Error during conditioning: " << error << std::endl;
    } else {
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }

    while (true) {
        error = sgp41_measure_raw_signals(default_rh, default_t, &sraw_voc, &sraw_nox);
        if (error) {
            std::cerr << "Error measuring SGP41 raw signals: " << error << std::endl;
        } else {
            std::cout << "SGP41 SRAW VOC: " << sraw_voc << " SRAW NOX: " << sraw_nox << std::endl;
        }

        // Add a delay between measurements
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}