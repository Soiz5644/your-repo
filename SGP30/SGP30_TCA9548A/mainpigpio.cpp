#include "rpi_tca9548apigpio.h"
#include "sgp30_i2c.h"
#include "sensirion_i2c_hal.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <pigpio.h>
#include <csignal>

volatile sig_atomic_t stop = 0;

void handle_sigint(int signum) {
    std::cout << "SIGINT received, terminating..." << std::endl;
    stop = 1;
}

int main() {
    // Register signal handler
    std::signal(SIGINT, handle_sigint);

    // Initialize Pigpio
    if (gpioInitialise() < 0) {
        std::cerr << "Pigpio initialization failed" << std::endl;
        return -1;
    }

    // Initialize I2C HAL
    sensirion_i2c_hal_init();

    // Initialize TCA9548A at the default I2C address (0x70)
    rpi_tca9548apigpio tca9548a;
    if (tca9548a.init(0x70) != 0) {
        std::cerr << "Failed to initialize TCA9548A" << std::endl;
        gpioTerminate();
        return -1;
    }

    // SGP30 on channel 2
    tca9548a.set_channel(2);

    // Initialize SGP30
    if (sgp30_init() != 0) {
        std::cerr << "Failed to initialize SGP30" << std::endl;
        gpioTerminate();
        return -1;
    }

    while (!stop) {
        uint16_t co2_eq_ppm, tvoc_ppb;
        if (sgp30_read_measurements(&co2_eq_ppm, &tvoc_ppb) != 0) {
            std::cerr << "Error reading SGP30 measurements" << std::endl;
        } else {
            std::cout << "SGP30 tVOC: " << tvoc_ppb << " ppb, CO2eq: " << co2_eq_ppm << " ppm (Raw tVOC: " << tvoc_ppb << ", Raw CO2eq: " << co2_eq_ppm << ")" << std::endl;
        }

        // Add a delay between measurements
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    gpioTerminate();
    std::cout << "Terminated gracefully" << std::endl;
    return 0;
}