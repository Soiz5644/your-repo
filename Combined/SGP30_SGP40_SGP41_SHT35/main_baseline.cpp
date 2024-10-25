#include "rpi_tca9548a.h"
#include "sgp30_i2c.h"
#include "sgp40_i2c.h"
#include "sgp41_i2c.h"
#include "sht3x_i2c.h"
#include "sensirion_i2c_hal.h"
#include "sensirion_gas_index_algorithm.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <fstream>
#include <ctime>
#include <sys/stat.h>
#include <mariadb/mysql.h>  // Include MySQL library

#define NO_ERROR 0  // Define NO_ERROR as 0

std::string get_current_time() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    char buffer[100];
    std::strftime(buffer, sizeof(buffer), "%d/%m/%y - %H:%M:%S", std::localtime(&now_time));
    return std::string(buffer);
}

bool file_exists(const std::string& filename) {
    struct stat buffer;
    return (stat(filename.c_str(), &buffer) == 0);
}

void save_baseline(uint16_t baseline, const std::string& filename) {
    std::ofstream file(filename, std::ios::out | std::ios::trunc);
    if (file.is_open()) {
        file << baseline;
        file.close();
    } else {
        std::cerr << "Unable to open file to save baseline" << std::endl;
    }
}

bool load_baseline(uint16_t& baseline, const std::string& filename) {
    std::ifstream file(filename);
    if (file.is_open()) {
        file >> baseline;
        file.close();
        return true;
    } else {
        std::cerr << "Unable to open file to load baseline" << std::endl;
        return false;
    }
}

bool is_first_power_up() {
    return !file_exists("first_power_up_flag.txt");
}

void set_first_power_up_flag() {
    std::ofstream file("first_power_up_flag.txt", std::ios::out | std::ios::trunc);
    if (file.is_open()) {
        file << "false";
        file.close();
    } else {
        std::cerr << "Unable to open file to set first power-up flag" << std::endl;
    }
}

void finish_with_error(MYSQL *con) {
    std::cerr << mysql_error(con) << std::endl;
    mysql_close(con);
    exit(1);
}

void insert_sensor_data(MYSQL *con, int sensor_id, double temperature, double humidity, int humidity_available) {
    char query[256];

    if (humidity_available) {
        snprintf(query, sizeof(query),
                 "INSERT INTO sensor_readings(sensor_id, temperature, humidity, reading_date) "
                 "VALUES(%d, %.2lf, %.2lf, NOW())",
                 sensor_id, temperature, humidity);
    } else {
        snprintf(query, sizeof(query),
                 "INSERT INTO sensor_readings(sensor_id, temperature, reading_date) "
                 "VALUES(%d, %.2lf, NOW())",
                 sensor_id, temperature);
    }

    if (mysql_query(con, query)) {
        finish_with_error(con);
    }
}

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

    // Initialize Gas Index Algorithm for VOC
    GasIndexAlgorithmParams sgp40_voc_params, sgp41_voc_params;
    GasIndexAlgorithm_init(&sgp40_voc_params, GasIndexAlgorithm_ALGORITHM_TYPE_VOC);
    GasIndexAlgorithm_init(&sgp41_voc_params, GasIndexAlgorithm_ALGORITHM_TYPE_VOC);

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

    // Connect to MySQL database
    MYSQL *con = mysql_init(NULL);
    if (con == NULL) {
        std::cerr << "mysql_init() failed" << std::endl;
        return -1;
    }
    if (mysql_real_connect(con, "15.188.64.253", "vm", "Admin123%", "sensor_data", 0, NULL, 0) == NULL) {
        finish_with_error(con);
    }

    // Open CSV file for appending data
    std::ofstream data_file;
    bool file_exists_flag = file_exists("sensor_data.csv");
    data_file.open("sensor_data.csv", std::ios::out | std::ios::app);
    
    // Write header if file is created new
    if (!file_exists_flag) {
        data_file << "Timestamp,SGP30_tVOC_ppb,SGP30_CO2eq_ppm,SGP40_SRAW_VOC,SGP41_SRAW_VOC,SGP41_SRAW_NOX,sgp40_voc_index,sgp41_voc_index,Temperature_C,Humidity_%" << std::endl;
    }

    // Measure temperature and humidity from SHT35, and raw signals from SGP30, SGP40, and SGP41 in an infinite loop
    float temperature = 0.0;
    float humidity = 0.0;
    uint16_t co2_eq_ppm, tvoc_ppb;
    uint16_t sgp40_sraw_voc;

    auto start_time = std::chrono::system_clock::now();
    bool first_power_up = is_first_power_up();
    while (true) {
        std::string timestamp = get_current_time();

        // Measure from SGP30
        tca9548a.set_channel(2);
        if (sgp30_read_measurements(&co2_eq_ppm, &tvoc_ppb) != 0) {
            std::cerr << "Error reading SGP30 measurements" << std::endl;
        }

        // Measure from SGP40
        tca9548a.set_channel(0);
        if (sgp40_measure_raw_signal(default_rh, default_t, &sgp40_sraw_voc) != 0) {
            std::cerr << "Error measuring SGP40 raw signal" << std::endl;
        }
        int32_t sgp40_voc_index;
        GasIndexAlgorithm_process(&sgp40_voc_params, sgp40_sraw_voc, &sgp40_voc_index);

        // Measure from SGP41
        tca9548a.set_channel(1);
        if (sgp41_measure_raw_signals(default_rh, default_t, &sraw_voc, &sraw_nox) != 0) {
            std::cerr << "Error measuring SGP41 raw signals" << std::endl;
        }
        int32_t sgp41_voc_index;
        GasIndexAlgorithm_process(&sgp41_voc_params, sraw_voc, &sgp41_voc_index);

        // Measure from SHT35
        tca9548a.set_channel(3);
        if (sht3x_measure_single_shot(REPEATABILITY_MEDIUM, false, &temperature, &humidity) != NO_ERROR) {
            std::cerr << "Error measuring SHT35" << std::endl;
        }

        // Insert data into MySQL database
        insert_sensor_data(con, 30, tvoc_ppb, co2_eq_ppm, 1);  // Assuming sensor ID 30 for SGP30
        insert_sensor_data(con, 40, sgp40_sraw_voc, sgp40_voc_index, 1);  // Assuming sensor ID 40 for SGP40
        insert_sensor_data(con, 41, sraw_voc, sgp41_voc_index, 1);  // Assuming sensor ID 41 for SGP41
        insert_sensor_data(con, 35, temperature, humidity, 1);  // Assuming sensor ID 35 for SHT35

        // Write data to CSV file
        data_file << timestamp << ","
                  << tvoc_ppb << "," << co2_eq_ppm << ","
                  << sgp40_sraw_voc << "," << sraw_voc << "," << sraw_nox << ","
                  << sgp40_voc_index << "," << sgp41_voc_index << ","
                  << temperature << "," << humidity
                  << std::endl;          

        // Save baseline every 12 hours during the first power-up
        if (first_power_up) {
            auto current_time = std::chrono::system_clock::now();
            if (std::chrono::duration_cast<std::chrono::hours>(current_time - start_time).count() >= 12) {
                uint16_t sgp30_baseline;
                if (sgp30_get_iaq_baseline(&sgp30_baseline) == 0) {
                    save_baseline(sgp30_baseline, "sgp30_baseline.txt");
                }
                first_power_up = false;
                set_first_power_up_flag();  // Set flag to indicate first power-up is completed
            }
        }

        // Add a delay between measurements
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // Close the CSV file
    data_file.close();
    mysql_close(con);

    return 0;
}