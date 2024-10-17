#include "rpi_tca9548a.h"
#include "sgp30_i2c.h"
#include "sgp40_i2c.h"
#include "sgp41_i2c.h"
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

    // Initialize SGP30 on channel 2
    std::cout << "Initializing SGP30 on channel 2..." << std::endl;
    tca9548a.set_channel(2);
    if (sgp30_init() != 0) {
        std::cerr << "Failed to initialize SGP30" << std::endl;
        return -1;
    }

    // Initialize SGP40 on channel 0
    std::cout << "Initializing SGP40 on channel 0..." << std::endl;
    tca9548a.set_channel(0);
    uint16_t sgp40_serial_number[3];
    if (sgp40_get_serial_number(sgp40_serial_number, 3) != 0) {
        std::cerr << "Failed to get SGP40 serial number" << std::endl;
        return -1;
    }
    std::cout << "SGP40 serial number: "
              << sgp40_serial_number[0] << sgp40_serial_number[1] << sgp40_serial_number[2]
              << std::endl;

    // Initialize SGP41 on channel 1
    std::cout << "Initializing SGP41 on channel 1..." << std::endl;
    tca9548a.set_channel(1);
    uint16_t sgp41_serial_number[3];
    if (sgp41_get_serial_number(sgp41_serial_number) != 0) {
        std::cerr << "Failed to get SGP41 serial number" << std::endl;
        return -1;
    }
    std::cout << "SGP41 serial number: "
              << sgp41_serial_number[0] << sgp41_serial_number[1] << sgp41_serial_number[2]
              << std::endl;

    // Execute conditioning for 10 seconds for SGP41
    uint16_t sraw_voc, sraw_nox;
    uint16_t default_rh = 0x8000; // Default relative humidity
    uint16_t default_t = 0x6666;  // Default temperature
    std::cout << "Conditioning SGP41 for 10 seconds..." << std::endl;
    if (sgp41_execute_conditioning(default_rh, default_t, &sraw_voc) != 0) {
        std::cerr << "Error during SGP41 conditioning" << std::endl;
        return -1;
    }
    std::this_thread::sleep_for(std::chrono::seconds(10));

    // Initialize SHT35 on channel 3
    std::cout << "Initializing SHT35 on channel 3..." << std::endl;
    tca9548a.set_channel(3);
    sht3x_init(SHT35_I2C_ADDR_45);
    sht3x_soft_reset();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Read status register of SHT35
    uint16_t status_register = 0;
    if (sht3x_read_status_register(&status_register) != NO_ERROR) {
        std::cerr << "Error reading SHT35 status register" << std::endl;
        return -1;
    }
    std::cout << "SHT35 status register: " << status_register << std::endl;

    // Measure temperature and humidity from SHT35, and raw signals from SGP30, SGP40, and SGP41 in an infinite loop
    float temperature = 0.0;
    float humidity = 0.0;
    uint16_t co2_eq_ppm, tvoc_ppb;
    uint16_t sgp40_sraw_voc;

    while (true) {
        // Measure from SGP30
        tca9548a.set_channel(2);
        if (sgp30_read_measurements(&co2_eq_ppm, &tvoc_ppb) != 0) {
            std::cerr << "Error reading SGP30 measurements" << std::endl;
        } else {
            std::cout << "SGP30 tVOC: " << tvoc_ppb << " ppb, CO2eq: " << co2_eq_ppm << " ppm" << std::endl;
        }

        // Measure from SGP40
        tca9548a.set_channel(0);
        if (sgp40_measure_raw_signal(default_rh, default_t, &sgp40_sraw_voc) != 0) {
            std::cerr << "Error measuring SGP40 raw signal" << std::endl;
        } else {
            std::cout << "SGP40 SRAW VOC: " << sgp40_sraw_voc << std::endl;
        }

        // Measure from SGP41
        tca9548a.set_channel(1);
        if (sgp41_measure_raw_signals(default_rh, default_t, &sraw_voc, &sraw_nox) != 0) {
            std::cerr << "Error measuring SGP41 raw signals" << std::endl;
        } else {
            std::cout << "SGP41 SRAW VOC: " << sraw_voc << ", SRAW NOX: " << sraw_nox << std::endl;
        }

        // Measure from SHT35
        tca9548a.set_channel(3);
        if (sht3x_measure_single_shot(REPEATABILITY_MEDIUM, false, &temperature, &humidity) != NO_ERROR) {
            std::cerr << "Error measuring SHT35" << std::endl;
        } else {
            std::cout << "Temperature: " << temperature << " °C, Humidity: " << humidity << " %" << std::endl;
        }

        // Add a delay between measurements
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}