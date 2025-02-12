#include "sgp30_i2c.h"
#include "sht4x_i2c.h"
#include "sensirion_i2c_hal.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <fstream>
#include <ctime>
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>
#include <wiringPi.h>
#include <wiringPiI2C.h>

#define NO_ERROR 0  // Define NO_ERROR as 0

std::string get_current_time() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    char buffer[100];
    std::strftime(buffer, sizeof(buffer), "%d/%m/%y %H:%M:%S", std::localtime(&now_time));
    return std::string(buffer);
}

bool file_exists(const std::string& filename) {
    struct stat buffer;
    return (stat(filename.c_str(), &buffer) == 0);
}

void scan_i2c_bus() {
    std::cout << "Scanning I2C bus..." << std::endl;
    for (int addr = 0; addr <= 0x7F; ++addr) {
        int fd = wiringPiI2CSetup(addr);
        if (fd != -1) {
            if (wiringPiI2CRead(fd) != -1) {
                std::cout << "I2C device found at address 0x" << std::hex << addr << std::dec << std::endl;
            }
            close(fd);
        }
    }
}

int main() {
    // Initialize I2C HAL
    std::cout << "Initializing I2C HAL..." << std::endl;
    sensirion_i2c_hal_init();

    // Scan I2C bus for devices
    scan_i2c_bus();

    // Initialize SGP30
    std::cout << "Initializing SGP30..." << std::endl;
    SGP30 sgp30;
    if (sgp30.init() != 0) {
        std::cerr << "Failed to initialize SGP30" << std::endl;
        return -1;
    }

    // Initialize SHT40
    std::cout << "Initializing SHT40..." << std::endl;
    sht4x_init(SHT40_I2C_ADDR_44);

    // Perform Soft Reset on SHT40
    std::cout << "Performing soft reset on SHT40..." << std::endl;
    sht4x_soft_reset();
    sensirion_i2c_hal_sleep_usec(10000);  // Wait for 10ms

    // Verify SHT40 communication
    uint32_t serial_number = 0;
    int16_t serial_error = sht4x_serial_number(&serial_number);
    if (serial_error != NO_ERROR) {
        std::cerr << "Failed to read SHT40 serial number, error code: " << serial_error << std::endl;
        return -1;
    }
    std::cout << "SHT40 Serial Number: " << serial_number << std::endl;

    // Open CSV file for appending data
    std::ofstream data_file;
    bool file_exists_flag = file_exists("SGP30_output.csv");
    data_file.open("SGP30_output.csv", std::ios::out | std::ios::app);
    
    // Write header if file is created new
    if (!file_exists_flag) {
        data_file << "Timestamp,SGP30_tVOC_ppb,SGP30_CO2eq_ppm,SGP30_H2_raw,SGP30_Ethanol_raw,Temperature,Humidity" << std::endl;
    }

    // Measure raw signals from SGP30 and SHT40 in an infinite loop
    uint16_t co2_eq_ppm, tvoc_ppb;
    float temperature = 0.0, humidity = 0.0;

    while (true) {
        std::string timestamp = get_current_time();

        // Measure temperature and humidity from SHT40
        int16_t measure_error = sht4x_measure_high_precision(&temperature, &humidity);
        if (measure_error != NO_ERROR) {
            std::cerr << "Error measuring SHT40, error code: " << measure_error << std::endl;
        } else {
            std::cout << "Measured SHT40 Temperature: " << temperature << ", Humidity: " << humidity << std::endl;
        }

        // Measure from SGP30 with compensation
        sgp30.set_relative_humidity(temperature, humidity);
        if (sgp30.measure()) {
            std::cerr << "Error reading SGP30 measurements" << std::endl;
        } else {
            std::cout << "SGP30 CO2eq: " << sgp30.getCO2() << ", TVOC: " << sgp30.getTVOC() << std::endl;
        }
        co2_eq_ppm = sgp30.getCO2();
        tvoc_ppb = sgp30.getTVOC();

        // Debugging raw H2 and Ethanol values
        if (sgp30.readRaw()) {
            std::cerr << "Error reading SGP30 raw measurements" << std::endl;
        } else {
            std::cout << "SGP30 Raw H2: " << sgp30.getH2_raw() << ", Raw Ethanol: " << sgp30.getEthanol_raw() << std::endl;
        }

        // Write data to CSV file
        data_file << timestamp << ","
                  << tvoc_ppb << "," << co2_eq_ppm << ","
                  << sgp30.getH2_raw() << "," << sgp30.getEthanol_raw() << ","
                  << temperature << "," << humidity
                  << std::endl;

        // Add a delay between measurements
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // Close the CSV file
    data_file.close();

    return 0;
}