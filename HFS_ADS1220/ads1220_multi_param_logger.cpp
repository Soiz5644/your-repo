#include <iostream>
#include <fstream>
#include <pigpio.h>
#include <unistd.h> // for usleep
#include <vector>
#include <string>
#include <ctime>

// SPI settings
#define SPI_CHANNEL 0  // SPI0 CE0
#define SPI_SPEED 500000  // 500 kHz, safe for ADS1220

// ADS1220 Commands
#define CMD_RESET   0x06
#define CMD_START   0x08
#define CMD_READ_DATA 0x10
#define CMD_WREG    0x40

int handle; // SPI handle

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

// Configure ADS1220 with a given PGA setting
void configureADS1220(uint8_t pga) {
    writeRegister(0x00, 0x01 | (pga << 1)); // Register 0: AINP = AIN0, AINN = AIN1, set PGA
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
    file << "Timestamp,PGA,AIN0_raw,AIN1_raw,AIN0_scale,AIN1_scale,AIN0-AIN1_raw,AIN0-AIN1_scale\n";

    // PGA gain values (as per ADS1220 datasheet: 1, 2, 4, 8, 16, 32, 64, 128)
    std::vector<uint8_t> pgaValues = {0, 1, 2, 3, 4, 5, 6, 7};

    // Voltage reference
    const double vRef = 2.048; // Internal reference voltage

    while (true) {
        for (uint8_t pga : pgaValues) {
            // Configure ADS1220 with the current PGA setting
            configureADS1220(pga);
            usleep(50000); // Wait for the configuration to take effect

            // Start conversion
            spiWriteByte(CMD_START);
            usleep(50000); // Wait for conversion to complete

            // Read raw data for AIN0, AIN1, and AIN0-AIN1
            spiWriteByte(CMD_READ_DATA);
            int32_t ain0_ain1_raw = readADCData();

            // Calculate scaled values
            double gain = 1 << pga; // Gain = 2^pga
            double ain0_ain1_scale = (ain0_ain1_raw / 8388607.0) * (vRef / gain);

            // Log data
            std::string timestamp = getCurrentTimestamp();
            file << timestamp << "," << (1 << pga) << ","    // Timestamp and PGA
                 << "N/A" << "," << "N/A" << ","             // Placeholder for AIN0_raw and AIN1_raw
                 << "N/A" << "," << "N/A" << ","             // Placeholder for AIN0_scale and AIN1_scale
                 << ain0_ain1_raw << ","                    // AIN0-AIN1_raw
                 << ain0_ain1_scale << "\n";                // AIN0-AIN1_scale
            file.flush(); // Ensure data is written to the file
        }

        // Sleep for 2 minutes
        std::cout << "Sleeping for 2 minutes..." << std::endl;
        sleep(120);
    }

    file.close();
    spiClose(handle);
    gpioTerminate();
    return 0;
}