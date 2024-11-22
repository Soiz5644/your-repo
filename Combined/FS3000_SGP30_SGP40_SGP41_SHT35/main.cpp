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
#include <unistd.h>
#include <cstring>

// Include FS3000 header
#define FS3000_ADDRESS 0x28
#define FS3000_1005 0x00

class RTrobot_FS3000 {
public:
    RTrobot_FS3000(int i2c_addr = FS3000_ADDRESS, int device = FS3000_1005);
    float FS3000_ReadData();

private:
    int file_i2c;
    int i2c_addr;
    int device;

    int FS3000_i2c_read();
    const float fs3000_1005_air_velocity_table[9] = {0, 1.07, 2.01, 3.0, 3.97, 4.96, 5.98, 6.99, 7.23};
    const int fs3000_1005_adc_table[9] = {409, 915, 1522, 2066, 2523, 2908, 3256, 3572, 3686};
};

RTrobot_FS3000::RTrobot_FS3000(int i2c_addr, int device)
    : i2c_addr(i2c_addr), device(device) {
    wiringPiSetup();
    file_i2c = wiringPiI2CSetup(i2c_addr);
    if (file_i2c == -1) {
        std::cerr << "Failed to initialize I2C communication." << std::endl;
        exit(1);
    }
    std::cout << "FS3000 sensor initialized correctly." << std::endl;
}

float RTrobot_FS3000::FS3000_ReadData() {
    int fm_raw = FS3000_i2c_read();
    if (fm_raw == -1) {
        return 0;
    }

    int fm_level = 0;
    float fm_percentage = 0.0;

    if (device == FS3000_1005) {
        if (fm_raw < fs3000_1005_adc_table[0] || fm_raw > fs3000_1005_adc_table[8]) {
            return 0;
        }
        for (int i = 0; i < 8; i++) {
            if (fm_raw > fs3000_1005_adc_table[i]) {
                fm_level = i;
            }
        }
        fm_percentage = (fm_raw - fs3000_1005_adc_table[fm_level]) / 
                        float(fs3000_1005_adc_table[fm_level + 1] - fs3000_1005_adc_table[fm_level]);
        return (fs3000_1005_air_velocity_table[fm_level + 1] - fs3000_1005_air_velocity_table[fm_level]) * 
               fm_percentage + fs3000_1005_air_velocity_table[fm_level];
    }

    return 0;
}

int RTrobot_FS3000::FS3000_i2c_read() {
    uint8_t buffer[5];
    usleep(15000);  // Delay before reading
    if (read(file_i2c, buffer, 5) != 5) {
        std::cerr << "Failed to read from the i2c bus. Error: " << strerror(errno) << std::endl;
        return -1;
    } else {
        int sum = 0;
        for (int i = 0; i < 5; i++) {
            sum += buffer[i];
        }
        if (sum & 0xff != 0x00) {
            return -1;
        } else {
            return (buffer[1] * 256 + buffer[2]) & 0xfff;
        }
    }
}

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

    // Initialize FS3000 on channel 7
    std::cout << "Initializing FS3000 on channel 7..." << std::endl;
    tca9548a.set_channel(7);
    RTrobot_FS3000 fs3000;

    // Open CSV file for appending data
    std::ofstream data_file;
    bool file_exists_flag = file_exists("sensor_data.csv");
    data_file.open("sensor_data.csv", std::ios::out | std::ios::app);
    
    // Write header if file is created new
    if (!file_exists_flag) {
        data_file << "Timestamp,SGP30_tVOC_ppb,SGP30_CO2eq_ppm,SGP30_SRAW,SGP40_SRAW_VOC,SGP41_SRAW_VOC,SGP41_SRAW_NOX,sgp40_voc_index,sgp41_voc_index,Temperature_C,Humidity_%,FS3000_Velocity_m/s" << std::endl;
    }

    // Measure temperature and humidity from SHT35, and raw signals from SGP30, SGP40, SGP41, and FS3000 in an infinite loop
    float temperature = 0.0;
    float humidity = 0.0;
    uint16_t co2_eq_ppm, tvoc_ppb, sgp30_sraw;
    uint16_t sgp40_sraw_voc;

    while (true) {
        std::string timestamp = get_current_time();

        // Measure from SGP30
        tca9548a.set_channel(2);
        if (sgp30_read_measurements(&co2_eq_ppm, &tvoc_ppb) != 0) {
            std::cerr << "Error reading SGP30 measurements" << std::endl;
        }
        // Assuming SGP30 has a function to read SRAW data, if not you need to implement it
        if (sgp30_read_sraw(&sgp30_sraw) != 0) {
            std::cerr << "Error reading SGP30 SRAW data" << std::endl;
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

        // Measure from FS3000
        tca9548a.set_channel(7);
        float fs3000_velocity = fs3000.FS3000_ReadData();
        std::cout << "FS3000 Air Velocity: " << fs3000_velocity << " m/s" << std::endl;

        // Write data to CSV file
        data_file << timestamp << ","
                  << tvoc_ppb << "," << co2_eq_ppm << "," << sgp30_sraw << ","
                  << sgp40_sraw_voc << "," << sraw_voc << "," << sraw_nox << ","
                  << sgp40_voc_index << "," << sgp41_voc_index << ","
                  << temperature << "," << humidity << ","
                  << fs3000_velocity
                  << std::endl;

        // Add a delay between measurements
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // Close the CSV file
    data_file.close();

    return 0;
}