#include "rpi_tca9548a.h"
#include "sht3x_i2c.h"
#include "sensirion_i2c_hal.h"
#include <iostream>
#include <thread>
#include <chrono>

#define NO_ERROR 0  // Define NO_ERROR as 0

int main() {
    // Initialize I2C HAL
    std::cout << "Initializing I2C HAL..." << std::endl;
    sensirion_i2c_hal_init();

    // Initialize TCA9548A at the default I2C address (0x70)
    std::cout << "Initializing TCA9548A..." << std::endl;
    rpi_tca9548a tca9548a;
    if (tca9548a.init(0x70) != 0) {
        std::cerr << "Failed to initialize TCA9548A" << std::endl;
        return -1;
    }

    // Select channel 3 for SHT35
    std::cout << "Selecting channel 3 on TCA9548A..." << std::endl;
    tca9548a.set_channel(3);

    // Initialize SHT35 sensor with I2C address 0x45
    std::cout << "Initializing SHT35 sensor at address 0x45..." << std::endl;
    sht3x_init(SHT35_I2C_ADDR_45);

    // Perform a soft reset
    std::cout << "Performing SHT35 soft reset..." << std::endl;
    sht3x_soft_reset();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Read status register
    std::cout << "Reading SHT35 status register..." << std::endl;
    uint16_t status_register = 0;
    int16_t error = sht3x_read_status_register(&status_register);
    if (error != NO_ERROR) {
        std::cerr << "Error reading SHT35 status register: " << error << std::endl;
        return -1;
    }
    std::cout << "SHT35 status register: " << status_register << std::endl;

    // Measure temperature and humidity in a loop
    float temperature = 0.0;
    float humidity = 0.0;
    for (int i = 0; i < 10; ++i) {
        std::cout << "Measuring temperature and humidity..." << std::endl;
        error = sht3x_measure_single_shot(REPEATABILITY_MEDIUM, false, &temperature, &humidity);
        if (error != NO_ERROR) {
            std::cerr << "Error measuring SHT35: " << error << std::endl;
        } else {
            std::cout << "Temperature: " << temperature << " °C, Humidity: " << humidity << " %" << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}