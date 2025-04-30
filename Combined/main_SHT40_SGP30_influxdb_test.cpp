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
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <sstream>
#include <curl/curl.h>  // Include libcurl for HTTP requests

#define NO_ERROR 0  // Define NO_ERROR as 0

// InfluxDB connection parameters
const std::string influxdb_host = "00.00.00.0";
const int influxdb_port = 8086;
const std::string influxdb_database = "database";
const std::string influxdb_user = "user";
const std::string influxdb_password = "password";

// Sensor information
const std::string sensor_name = "SHT40_SGP30";
const std::string sensor_location = "intermédiaire";

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

void write_to_influxdb(const std::string& measurement, const std::string& sensor_id, float temperature, float humidity, uint16_t co2_eq_ppm, uint16_t tvoc_ppb, uint16_t h2_raw, uint16_t ethanol_raw) {
    CURL *curl;
    CURLcode res;
    std::string influxdb_url = "http://" + influxdb_host + ":" + std::to_string(influxdb_port) + "/write?db=" + influxdb_database;
    std::string data;

    // Prepare data in InfluxDB line protocol format
    std::ostringstream data_stream;
    data_stream << measurement
                << ",sensor_id=" << sensor_id
                << ",sensor_name=" << sensor_name
                << ",location=" << sensor_location
                << " temperature=" << temperature
                << ",humidity=" << humidity
                << ",co2_eq_ppm=" << co2_eq_ppm
                << ",tvoc_ppb=" << tvoc_ppb
                << ",h2_raw=" << h2_raw
                << ",ethanol_raw=" << ethanol_raw
                << " " << std::chrono::system_clock::now().time_since_epoch().count();
    data = data_stream.str();

    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    if(curl) {
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, ("Authorization: Basic " + std::string(curl_easy_escape(curl, (influxdb_user + ":" + influxdb_password).c_str(), 0))).c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_URL, influxdb_url.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);  // Set timeout to 10 seconds
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);   // Enable verbose output for debugging

        res = curl_easy_perform(curl);
        if(res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }

        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
    }
    curl_global_cleanup();
}

int main() {
    // Initialize I2C HAL
    sensirion_i2c_hal_init();

    // Initialize SGP30
    SGP30 sgp30;
    if (sgp30.init() != 0) {
        return -1;
    }

    // Initialize SHT40
    sht4x_init(SHT40_I2C_ADDR_44);

    // Open CSV file for appending data
    std::ofstream data_file;
    bool file_exists_flag = file_exists("data_SHT40_SGP30.csv");
    data_file.open("data_SHT40_SGP30.csv", std::ios::out | std::ios::app);
    
    // Write header if file is created new
    if (!file_exists_flag) {
        data_file << "Timestamp,SGP30_tVOC_ppb,SGP30_CO2eq_ppm,SGP30_H2_raw,SGP30_Ethanol_raw,SHT40_Temperature,SHT40_Humidity" << std::endl;
    }

    // Measure temperature and humidity from SHT40, and raw signals from SGP30 in an infinite loop
    float temperature = 0.0;
    float humidity = 0.0;
    uint16_t co2_eq_ppm, tvoc_ppb;

    while (true) {
        std::string timestamp = get_current_time();

        // Measure temperature and humidity from SHT40
        if (sht4x_measure_high_precision(&temperature, &humidity) != NO_ERROR) {
        }

        // Measure from SGP30 with compensation
        sgp30.set_relative_humidity(temperature, humidity);
        if (sgp30.measure()) {
        } else {
        }
        co2_eq_ppm = sgp30.getCO2();
        tvoc_ppb = sgp30.getTVOC();

        // Debugging raw H2 and Ethanol values
        if (sgp30.readRaw()) {
        } else {
        }

        // Write data to CSV file and standard output
        std::ostringstream data_stream;
        data_stream << timestamp << ","
                    << tvoc_ppb << "," << co2_eq_ppm << ","
                    << sgp30.getH2_raw() << "," << sgp30.getEthanol_raw() << ","
                    << temperature << "," << humidity;
        std::string data = data_stream.str();

        data_file << data << std::endl;
        std::cout << data << std::endl;
        
        // Send data to InfluxDB with a descriptive sensor_id
        write_to_influxdb("sensor_data", "sdbpm1", temperature, humidity, co2_eq_ppm, tvoc_ppb, sgp30.getH2_raw(), sgp30.getEthanol_raw());

        // Add a delay between measurements (10 seconds)
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }

    // Close the CSV file
    data_file.close();

    return 0;
}