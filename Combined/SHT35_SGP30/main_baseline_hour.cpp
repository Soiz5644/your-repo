#include "rpi_tca9548a.h"
#include "sgp30_i2c.h"
#include "sensirion_i2c_hal.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <fstream>
#include <cstdlib> // for system()
#include <ctime>   // for std::time_t, std::tm, std::localtime, std::strftime

#define BASELINE_FILE "sgp30_baseline.txt"
#define HOURS_TO_RUN 16
#define DEBUG_FILE "debug_log.txt"

// Function to log debug information
void log_debug_info(const std::string& message) {
    std::ofstream debug_file(DEBUG_FILE, std::ios::app);
    if (debug_file.is_open()) {
        // Get current time
        std::time_t now = std::time(nullptr);
        std::tm* local_time = std::localtime(&now);
        char time_str[100];
        std::strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", local_time);

        debug_file << "[" << time_str << "] " << message << std::endl;
        debug_file.close();
    } else {
        std::cerr << "Unable to open debug log file" << std::endl;
    }
}

// Function to read baseline from file
bool read_baseline(uint16_t &co2, uint16_t &tvoc) {
    std::ifstream infile(BASELINE_FILE);
    if (infile.good()) {
        infile >> co2 >> tvoc;
        infile.close();
        log_debug_info("Baseline read from file: CO2 = " + std::to_string(co2) + ", TVOC = " + std::to_string(tvoc));
        return true;
    }
    log_debug_info("No baseline file found.");
    return false;
}

// Function to write baseline to file
void write_baseline(uint16_t co2, uint16_t tvoc) {
    std::ofstream outfile(BASELINE_FILE);
    if (outfile.is_open()) {
        outfile << co2 << " " << tvoc;
        outfile.close();
        log_debug_info("Baseline written to file: CO2 = " + std::to_string(co2) + ", TVOC = " + std::to_string(tvoc));
    } else {
        log_debug_info("Failed to open baseline file for writing.");
    }
}

int main() {
    log_debug_info("Program started.");
    sensirion_i2c_hal_init();

    rpi_tca9548a tca9548a;
    if (tca9548a.init(0x70) != 0) {
        std::cerr << "Failed to initialize TCA9548A" << std::endl;
        log_debug_info("Failed to initialize TCA9548A.");
        return -1;
    }

    tca9548a.set_channel(2);

    if (sgp30_init() != 0) {
        std::cerr << "Failed to initialize SGP30" << std::endl;
        log_debug_info("Failed to initialize SGP30.");
        return -1;
    }

    uint16_t co2_baseline, tvoc_baseline;
    if (!read_baseline(co2_baseline, tvoc_baseline)) {
        // Initialize baseline if not found
        co2_baseline = 0; // or some appropriate initial value
        tvoc_baseline = 0; // or some appropriate initial value
        write_baseline(co2_baseline, tvoc_baseline);
        log_debug_info("Baseline initialized: CO2 = " + std::to_string(co2_baseline) + ", TVOC = " + std::to_string(tvoc_baseline));
    } else {
        sgp30_set_baseline(co2_baseline, tvoc_baseline);
    }

    auto last_baseline_time = std::chrono::steady_clock::now();
    int hours_elapsed = 0;

    while (hours_elapsed < HOURS_TO_RUN) {
        uint16_t co2_eq_ppm, tvoc_ppb, tvoc_sraw;
        if (sgp30_read_measurements(&co2_eq_ppm, &tvoc_ppb) != 0) {
            std::cerr << "Error reading SGP30 measurements" << std::endl;
            log_debug_info("Error reading SGP30 measurements.");
        } else {
            if (sgp30_get_tvoc_inceptive(&tvoc_sraw) != 0) {
                std::cerr << "Error reading SGP30 SRAW measurements" << std::endl;
                log_debug_info("Error reading SGP30 SRAW measurements.");
            } else {
                std::cout << "SGP30 tVOC: " << tvoc_ppb << " ppb, CO2eq: " << co2_eq_ppm << " ppm, tVOC SRAW: " << tvoc_sraw << std::endl;
                log_debug_info("SGP30 tVOC: " + std::to_string(tvoc_ppb) + " ppb, CO2eq: " + std::to_string(co2_eq_ppm) + " ppm, tVOC SRAW: " + std::to_string(tvoc_sraw));
            }
        }

        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::hours>(now - last_baseline_time).count() >= 1) {
            uint16_t co2, tvoc;
            if (sgp30_get_baseline(&co2, &tvoc) == 0) {
                write_baseline(co2, tvoc);
                log_debug_info("Baseline retrieved and written: CO2 = " + std::to_string(co2) + ", TVOC = " + std::to_string(tvoc));
                last_baseline_time = now;
                hours_elapsed++;
            } else {
                log_debug_info("Failed to retrieve baseline.");
            }
        }

        std::this_thread::sleep_for(std::chrono::seconds(10)); // Sleep for 10 seconds between measurements
    }

    std::cout << "16 hours elapsed. Shutting down the system." << std::endl;
    log_debug_info("16 hours elapsed. Shutting down the system.");
    system("sudo shutdown -h now");

    return 0;
}