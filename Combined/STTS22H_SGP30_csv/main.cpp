#include "sgp30_i2c.h"
#include "stts22h_reg.h"
#include "sensirion_i2c_hal.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <fstream>
#include <ctime>
#include <sys/stat.h>
#include <unistd.h>
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <sstream>

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

int fd; // File descriptor for I2C device

int32_t platform_write(void *handle, uint8_t reg, const uint8_t *bufp, uint16_t len) {
    for (int i = 0; i < len; i++) {
        if (wiringPiI2CWriteReg8(fd, reg + i, bufp[i]) < 0) {
            return -1;
        }
    }
    return 0;
}

int32_t platform_read(void *handle, uint8_t reg, uint8_t *bufp, uint16_t len) {
    for (int i = 0; i < len; i++) {
        int val = wiringPiI2CReadReg8(fd, reg + i);
        if (val < 0) {
            return -1;
        }
        bufp[i] = val;
    }
    return 0;
}

void platform_delay(uint32_t millisec) {
    usleep(millisec * 1000);
}

int main() {
    // Initialize I2C HAL
    sensirion_i2c_hal_init();

    // Initialize SGP30
    SGP30 sgp30;
    if (sgp30.init() != 0) {
        return -1;
    }

    // Initialize STTS22H
    stmdev_ctx_t dev_ctx;
    dev_ctx.write_reg = platform_write;
    dev_ctx.read_reg = platform_read;
    dev_ctx.mdelay = platform_delay;

    // Use the detected I2C address 0x3c
    fd = wiringPiI2CSetup(0x3c);
    if (fd < 0) {
        std::cerr << "Failed to initialize I2C communication." << std::endl;
        return -1;
    }

    uint8_t who_am_i;
    if (stts22h_dev_id_get(&dev_ctx, &who_am_i) != 0 || who_am_i != STTS22H_ID) {
        std::cerr << "Error reading WHO_AM_I or unexpected WHO_AM_I value." << std::endl;
        return -1;
    }

    // Open CSV file for appending data
    std::ofstream data_file;
    bool file_exists_flag = file_exists("data_output.csv");
    data_file.open("data_output.csv", std::ios::out | std::ios::app);
    
    // Write header if file is created new
    if (!file_exists_flag) {
        data_file << "Timestamp,SGP30_tVOC_ppb,SGP30_CO2eq_ppm,SGP30_H2_raw,SGP30_Ethanol_raw,STTS22H_Temperature" << std::endl;
    }

    // Measure temperature from STTS22H, and raw signals from SGP30 in an infinite loop
    float temperature = 0.0;
    uint16_t co2_eq_ppm, tvoc_ppb;

    while (true) {
        std::string timestamp = get_current_time();

        // Measure temperature from STTS22H
        if (stts22h_temp_data_rate_set(&dev_ctx, STTS22H_ONE_SHOT) != 0) {
            std::cerr << "Error setting data rate." << std::endl;
            continue;
        }
        platform_delay(100);
        int16_t temperature_raw;
        if (stts22h_temperature_raw_get(&dev_ctx, &temperature_raw) != 0) {
            std::cerr << "Error reading temperature." << std::endl;
            continue;
        }
        temperature = stts22h_from_lsb_to_celsius(temperature_raw);

        // Measure from SGP30 without compensation
        if (sgp30.measure()) {
            std::cerr << "Error reading SGP30 measurements" << std::endl;
            continue;
        }
        co2_eq_ppm = sgp30.getCO2();
        tvoc_ppb = sgp30.getTVOC();

        // Debugging raw H2 and Ethanol values
        if (sgp30.readRaw()) {
            std::cerr << "Error reading SGP30 raw measurements" << std::endl;
            continue;
        }

        // Write data to CSV file and standard output
        std::ostringstream data_stream;
        data_stream << timestamp << ","
                    << tvoc_ppb << "," << co2_eq_ppm << ","
                    << sgp30.getH2_raw() << "," << sgp30.getEthanol_raw() << ","
                    << temperature;
        std::string data = data_stream.str();

        data_file << data << std::endl;
        std::cout << data << std::endl;
        
        // Add a delay between measurements
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // Close the CSV file
    data_file.close();

    return 0;
}