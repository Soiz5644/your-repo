// Include necessary headers
#include "rpi_tca9548a.h"
#include "sgp40_i2c.h"
#include "sensirion_i2c_hal.h"
#include <iostream>

int main() {
    // Initialize I2C HAL
    sensirion_i2c_hal_init();

    // Initialize TCA9548A at the default I2C address (0x70)
    rpi_tca9548a tca9548a;
    if (tca9548a.init(0x70) != 0) {
        std::cerr << "Failed to initialize TCA9548A" << std::endl;
        return -1;
    }

    // Select channel 0
    tca9548a.set_channel(0);

    // Now you can communicate with the SGP40 sensor
    uint16_t serial_number[3];
    int16_t error = sgp40_get_serial_number(serial_number, 3);
    if (error) {
        std::cerr << "Error getting SGP40 serial number: " << error << std::endl;
    } else {
        std::cout << "SGP40 serial number: " 
                  << serial_number[0] << serial_number[1] << serial_number[2] 
                  << std::endl;
    }

    // Reading data from SGP40
    uint16_t sraw_voc;
    uint16_t default_rh = 0x8000; // Default relative humidity
    uint16_t default_t = 0x6666;  // Default temperature

    error = sgp40_measure_raw_signal(default_rh, default_t, &sraw_voc);
    if (error) {
        std::cerr << "Error measuring SGP40 raw signal: " << error << std::endl;
    } else {
        std::cout << "SGP40 SRAW VOC: " << sraw_voc << std::endl;
    }

    return 0;
}