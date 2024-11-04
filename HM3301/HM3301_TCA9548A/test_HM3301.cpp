#include <iostream>
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include "rpi_tca9548a.h"
#include "Seeed_HM330X.h"

#define TCA9548A_ADDR 0x70
#define HM3301_ADDR 0x40

void select_channel(rpi_tca9548a& tca, uint8_t channel) {
    tca.set_channel(channel);
    delay(500); // Delay to ensure channel selection
}

int main() {
    rpi_tca9548a tca;
    if (tca.init(TCA9548A_ADDR) != 0) {
        std::cerr << "Failed to initialize TCA9548A multiplexer" << std::endl;
        return -1;
    }

    HM330X sensor(HM3301_ADDR);
    uint8_t data[29];

    // Test Sensor 1 on Channel 5
    select_channel(tca, 5);
    if (sensor.init() == NO_ERROR) {
        if (sensor.read_sensor_value(data, 29) == NO_ERROR) {
            std::cout << "Sensor 1 Data: ";
            for (int i = 0; i < 29; ++i) {
                std::cout << (int)data[i] << " ";
            }
            std::cout << std::endl;
        } else {
            std::cerr << "Failed to read data from Sensor 1" << std::endl;
        }
    } else {
        std::cerr << "Failed to initialize Sensor 1" << std::endl;
    }

    // Test Sensor 2 on Channel 7
    select_channel(tca, 7);
    if (sensor.init() == NO_ERROR) {
        if (sensor.read_sensor_value(data, 29) == NO_ERROR) {
            std::cout << "Sensor 2 Data: ";
            for (int i = 0; i < 29; ++i) {
                std::cout << (int)data[i] << " ";
            }
            std::cout << std::endl;
        } else {
            std::cerr << "Failed to read data from Sensor 2" << std::endl;
        }
    } else {
        std::cerr << "Failed to initialize Sensor 2" << std::endl;
    }

    return 0;
}