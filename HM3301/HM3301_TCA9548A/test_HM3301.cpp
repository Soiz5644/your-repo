#include <iostream>
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include "Seeed_HM330X.h"
#include "rpi_tca9548a.h"

#define HM3301_ADDR 0x40
#define TCA9548A_ADDR 0x70
#define TCA9548A_CHANNEL 5  // Channel 5

int main() {
    // Initialize the TCA9548A
    rpi_tca9548a tca9548a(TCA9548A_ADDR);
    if (!tca9548a.selectChannel(TCA9548A_CHANNEL)) {
        std::cerr << "Failed to select channel on TCA9548A" << std::endl;
        return 1;
    }

    // Initialize the HM3301 sensor
    HM330X sensor(HM3301_ADDR);
    uint8_t data[29];

    if (sensor.init() == NO_ERROR) {
        if (sensor.read_sensor_value(data, 29) == NO_ERROR) {
            std::cout << "Sensor Data: ";
            for (int i = 0; i < 29; ++i) {
                std::cout << (int)data[i] << " ";
            }
            std::cout << std::endl;
        } else {
            std::cerr << "Failed to read data from sensor" << std::endl;
        }
    } else {
        std::cerr << "Failed to initialize sensor" << std::endl;
    }

    return 0;
}