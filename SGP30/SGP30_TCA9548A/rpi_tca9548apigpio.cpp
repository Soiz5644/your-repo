#include <pigpio.h>
#include "rpi_tca9548apigpiopigpio.h"
#include <iostream>

// Constructor
rpi_tca9548apigpio::rpi_tca9548apigpio() : i2c_handle(-1) {}

// Initialize the TCA9548A
int rpi_tca9548apigpio::init(int address) {
    if (gpioInitialise() < 0) {
        std::cerr << "Pigpio initialization failed" << std::endl;
        return -1;
    }
    
    i2c_handle = i2cOpen(1, address, 0); // Open I2C on bus 1
    if (i2c_handle < 0) {
        std::cerr << "Failed to open I2C device" << std::endl;
        return -1;
    }
    return 0;
}

// Set the active channel on the TCA9548A
int rpi_tca9548apigpio::set_channel(int channel) {
    if (i2c_write_byte(i2c_handle, 1 << channel) < 0) {
        std::cerr << "Failed to set channel" << std::endl;
        return -1;
    }
    return 0;
}

// Destructor
rpi_tca9548apigpio::~rpi_tca9548apigpio() {
    if (i2c_handle >= 0) {
        i2cClose(i2c_handle);
    }
    gpioTerminate();
}