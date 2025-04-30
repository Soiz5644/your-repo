#include <iostream>
#include <pigpio.h>
#include <unistd.h> // for usleep

#define SPI_CHANNEL 0  // SPI0 CE0
#define SPI_SPEED 500000  // 500 kHz, safe for ADS1220

// ADS1220 Commands
#define CMD_RESET   0x06
#define CMD_START   0x08
#define CMD_READ_DATA 0x10

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

int main() {
    if (gpioInitialise() < 0) {
        errorExit("Failed to initialize pigpio.");
    }

    handle = spiOpen(SPI_CHANNEL, SPI_SPEED, 1); // mode=1 for SPI
    if (handle < 0) {
        errorExit("Failed to open SPI channel.");
    }
    std::cout << "SPI initialized successfully." << std::endl;

    // RESET the ADS1220
    spiWriteByte(CMD_RESET);
    usleep(5000); // Wait 5 ms for reset to complete

    // Send START command to begin conversion
    spiWriteByte(CMD_START);
    usleep(50000); // Wait 50 ms for conversion to complete

    // Read the conversion result
    spiWriteByte(CMD_READ_DATA);
    int32_t adcData = readADCData();

    std::cout << "ADC Conversion Result: " << adcData << std::endl;

    spiClose(handle);
    gpioTerminate();
    return 0;
}