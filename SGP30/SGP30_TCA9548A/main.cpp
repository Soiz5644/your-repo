#include "rpi_tca9548a.h"
#include "sgp30_i2c.h"    // We need to create this based on seeed_sgp30.py
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

    // SGP30 on channel 2
    tca9548a.set_channel(2);

    // Initialize SGP30
    if (sgp30_init() != 0) {
        std::cerr << "Failed to initialize SGP30" << std::endl;
        return -1;
    }

    while (true) {
        uint16_t co2_eq_ppm, tvoc_ppb;
        if (sgp30_read_measurements(&co2_eq_ppm, &tvoc_ppb) != 0) {
            std::cerr << "Error reading SGP30 measurements" << std::endl;
        } else {
            std::cout << "SGP30 tVOC: " << tvoc_ppb << " ppb, CO2eq: " << co2_eq_ppm << " ppm" << std::endl;
        }

        // Add a delay between measurements
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}