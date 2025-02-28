#include "sgp30_i2c.h"
#include "stts22h.h"  // Include the STTS22H sensor header file
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

int main() {
    // Initialize I2C HAL
    sensirion_i2c_hal_init();

    // Initialize SGP30
    SGP30 sgp30;
    if (sgp30.init() != 0) {
        return -1;
    }

    // Initialize STTS22H
    if (stts22h_init() != 0) {
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
        if (stts22h_measure_temperature(&temperature) != NO_ERROR) {
            // Error handling
        }

        // Measure from SGP30 with compensation
        sgp30.set_relative_humidity(temperature, 0.0);  // STTS22H does not measure humidity
        if (sgp30.measure()) {
            // Error handling
        } else {
            co2_eq_ppm = sgp30.getCO2();
            tvoc_ppb = sgp30.getTVOC();
        }

        // Debugging raw H2 and Ethanol values
        if (sgp30.readRaw()) {
            // Error handling
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