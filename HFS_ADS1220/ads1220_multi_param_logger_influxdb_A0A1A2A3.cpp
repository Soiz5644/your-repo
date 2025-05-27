#include <iostream>
#include <fstream>
#include <pigpio.h>
#include <unistd.h> // for usleep
#include <vector>
#include <string>
#include <ctime>
#include <curl/curl.h> // Include libcurl for HTTP requests
#include <sstream>  
#include <chrono>   

// SPI settings
#define SPI_CHANNEL 0  // SPI0 CE0
#define SPI_SPEED 500000  // 500 kHz, safe for ADS1220

// ADS1220 Commands
#define CMD_RESET   0x06
#define CMD_START   0x08
#define CMD_READ_DATA 0x10
#define CMD_WREG    0x40

int handle; // SPI handle

// InfluxDB connection parameters
const std::string influxdb_host = "00.00.00.00";
const int influxdb_port = 8086;
const std::string influxdb_database = "HFS";
const std::string influxdb_user = "user";
const std::string influxdb_password = "pwd";

// Function to handle errors
void errorExit(const std::string& message) {
    std::cerr << message << std::endl;
    gpioTerminate();
    exit(1);
}

// Send a single SPI command
void spiWriteByte(unsigned char data) {
    char txBuf[1];
    txBuf[0] = static_cast<char>(data);
    spiWrite(handle, txBuf, 1);
}

// Write to an ADS1220 register
void writeRegister(uint8_t reg, uint8_t value) {
    char txBuf[2];
    txBuf[0] = CMD_WREG | (reg << 2); // WREG command with register address
    txBuf[1] = value; // Value to write
    spiWrite(handle, txBuf, 2);
    usleep(1000); // Small delay to ensure the write completes
}

// Read 3 bytes of ADC data (24-bit result)
int32_t readADCData() {
    char txBuf[3] = {0x00, 0x00, 0x00};
    char rxBuf[3];
    spiXfer(handle, txBuf, rxBuf, 3);

    int32_t result = (static_cast<uint8_t>(rxBuf[0]) << 16) |
                     (static_cast<uint8_t>(rxBuf[1]) << 8) |
                     (static_cast<uint8_t>(rxBuf[2]));

    // Sign extension for 24-bit data
    if (result & 0x800000) {
        result |= 0xFF000000;
    }

    return result;
}

// Configure ADS1220 for a given MUX and PGA setting
void configureADS1220(uint8_t mux, uint8_t pga) {
    writeRegister(0x00, (mux << 4) | (pga << 1)); // Register 0: Set MUX and PGA
    writeRegister(0x01, 0x04); // Register 1: DR = 20 SPS, normal mode
    writeRegister(0x02, 0x40); // Register 2: Internal reference
    writeRegister(0x03, 0x00); // Register 3: No IDACs used
}

// Get current timestamp as a string
std::string getCurrentTimestamp() {
    std::time_t now = std::time(nullptr);
    char buf[80];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
    return std::string(buf);
}

// Write data to InfluxDB
void writeToInfluxDB(const std::string& measurement, int pga, int32_t ain0_raw, int32_t ain1_raw, double ain0_scale, double ain1_scale, int32_t ain0_ain1_raw, double ain0_ain1_scale, int32_t ain2_raw, int32_t ain3_raw, double ain2_scale, double ain3_scale, int32_t ain2_ain3_raw, double ain2_ain3_scale) {
    CURL *curl;
    CURLcode res;
    std::string influxdb_url = "http://" + influxdb_host + ":" + std::to_string(influxdb_port) + "/write?db=" + influxdb_database;

    // Prepare data in InfluxDB line protocol format
    std::ostringstream data_stream;
    data_stream << measurement
                << ",pga=" << pga
                << ",ain0_raw=" << ain0_raw
                << ",ain1_raw=" << ain1_raw
                << ",ain0_scale=" << ain0_scale
                << ",ain1_scale=" << ain1_scale
                << ",ain0_ain1_raw=" << ain0_ain1_raw
                << ",ain0_ain1_scale=" << ain0_ain1_scale
                << ",ain2_raw=" << ain2_raw
                << ",ain3_raw=" << ain3_raw
                << ",ain2_scale=" << ain2_scale
                << ",ain3_scale=" << ain3_scale
                << ",ain2_ain3_raw=" << ain2_ain3_raw
                << ",ain2_ain3_scale=" << ain2_ain3_scale
                << " " << std::chrono::system_clock::now().time_since_epoch().count();
    std::string data = data_stream.str();

    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    if(curl) {
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, ("Authorization: Basic " + std::string(curl_easy_escape(curl, (influxdb_user + ":" + influxdb_password).c_str(), 0))).c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_URL, influxdb_url.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);  // Set timeout to 10 seconds

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
    if (gpioInitialise() < 0) {
        errorExit("Failed to initialize pigpio.");
    }

    handle = spiOpen(SPI_CHANNEL, SPI_SPEED, 1); // mode=1 for SPI
    if (handle < 0) {
        errorExit("Failed to open SPI channel.");
    }
    std::cout << "SPI initialized successfully." << std::endl;

    // Open CSV file for logging
    std::ofstream file("ads1220_data.csv", std::ios::app);
    if (!file.is_open()) {
        errorExit("Failed to open CSV file.");
    }

    // Write header to CSV file
    file << "Timestamp,PGA,AIN0_raw,AIN1_raw,AIN0_scale,AIN1_scale,AIN0-AIN1_raw,AIN0-AIN1_scale,AIN2_raw,AIN3_raw,AIN2_scale,AIN3_scale,AIN2-AIN3_raw,AIN2-AIN3_scale\n";

    // PGA gain values (as per ADS1220 datasheet: 1, 2, 4, 8, 16, 32, 64, 128)
    std::vector<uint8_t> pgaValues = {0, 1, 2, 3, 4, 5, 6, 7};

    // Voltage reference
    const double vRef = 2.048; // Internal reference voltage

    // Declare variables outside the loop
    int32_t ain0_raw, ain1_raw, ain2_raw, ain3_raw, ain0_ain1_raw, ain2_ain3_raw;
    double ain0_scale, ain1_scale, ain2_scale, ain3_scale, ain0_ain1_scale, ain2_ain3_scale;

    while (true) {
        for (uint8_t pga : pgaValues) {
            // Configure for AIN0 (single-ended)
            configureADS1220(0x04, pga); // AINP = AIN0, AINN = GND
            usleep(100000); // Wait for configuration to take effect

            // Start conversion for AIN0
            spiWriteByte(CMD_START);
            usleep(100000); // Wait for conversion to complete
            ain0_raw = readADCData();
            ain0_scale = (ain0_raw / 8388607.0) * (vRef / (1 << pga));

            // Configure for AIN1 (single-ended)
            configureADS1220(0x05, pga); // AINP = AIN1, AINN = GND
            usleep(100000); // Wait for configuration to take effect

            // Start conversion for AIN1
            spiWriteByte(CMD_START);
            usleep(100000); // Wait for conversion to complete
            ain1_raw = readADCData();
            ain1_scale = (ain1_raw / 8388607.0) * (vRef / (1 << pga));

            // Configure for AIN0-AIN1 (differential)
            configureADS1220(0x00, pga); // AINP = AIN0, AINN = AIN1
            usleep(100000); // Wait for configuration to take effect

            // Start conversion for AIN0-AIN1
            spiWriteByte(CMD_START);
            usleep(100000); // Wait for conversion to complete
            ain0_ain1_raw = readADCData();
            ain0_ain1_scale = (ain0_ain1_raw / 8388607.0) * (vRef / (1 << pga));

            // Configure for AIN2 (single-ended)
            configureADS1220(0x06, pga); // AINP = AIN2, AINN = GND
            usleep(100000); // Wait for configuration to take effect

            // Start conversion for AIN2
            spiWriteByte(CMD_START);
            usleep(100000); // Wait for conversion to complete
            ain2_raw = readADCData();
            ain2_scale = (ain2_raw / 8388607.0) * (vRef / (1 << pga));

            // Configure for AIN3 (single-ended)
            configureADS1220(0x07, pga); // AINP = AIN3, AINN = GND
            usleep(100000); // Wait for configuration to take effect

            // Start conversion for AIN3
            spiWriteByte(CMD_START);
            usleep(100000); // Wait for conversion to complete
            ain3_raw = readADCData();
            ain3_scale = (ain3_raw / 8388607.0) * (vRef / (1 << pga));

            // Configure for AIN2-AIN3 (differential)
            configureADS1220(0x03, pga); // AINP = AIN2, AINN = AIN3
            usleep(100000); // Wait for configuration to take effect

            // Start conversion for AIN2-AIN3
            spiWriteByte(CMD_START);
            usleep(100000); // Wait for conversion to complete
            ain2_ain3_raw = readADCData();
            ain2_ain3_scale = (ain2_ain3_raw / 8388607.0) * (vRef / (1 << pga));


            // Log data
            std::string timestamp = getCurrentTimestamp();
            file << timestamp << "," << (1 << pga) << ","
                 << ain0_raw << "," << ain1_raw << "," << ain0_scale << "," << ain1_scale << ","
                 << ain0_ain1_raw << "," << ain0_ain1_scale << ","
                 << ain2_raw << "," << ain2_scale << "," << ain3_raw << "," << ain3_scale << ","
                 << ain2_ain3_raw << "," << ain2_ain3_scale << "\n";
            file.flush();

            // Write data to InfluxDB
            writeToInfluxDB("ads1220_data", (1 << pga), ain0_raw, ain1_raw, ain0_scale, ain1_scale, ain0_ain1_raw, ain0_ain1_scale, ain2_raw, ain2_scale, ain3_raw, ain3_scale, ain2_ain3_raw, ain2_ain3_scale);
        }

        // Sleep for 30 sec
        std::cout << "Sleeping for 30s..." << std::endl;
        sleep(30);
    }

    file.close();
    spiClose(handle);
    gpioTerminate();
    return 0;
}