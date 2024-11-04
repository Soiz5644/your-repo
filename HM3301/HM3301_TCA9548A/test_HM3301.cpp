#include <iostream>
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include "Seeed_HM330X.h"

#define HM3301_ADDR 0x40

int main() {
    HM330X sensor(HM3301_ADDR);
    uint8_t data[29];

    // Initialize sensor
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