#include "sgp30_i2c.h"
#include "sensirion_i2c_hal.h"
#include <unistd.h> // for usleep

#define SGP30_I2C_ADDRESS 0x58

int sgp30_init() {
    // Initialization commands for SGP30
    // (Example: send command 0x2003)
    uint8_t init_cmd[2] = {0x20, 0x03};
    return sensirion_i2c_write(SGP30_I2C_ADDRESS, init_cmd, sizeof(init_cmd));
}

int sgp30_read_measurements(uint16_t *co2_eq_ppm, uint16_t *tvoc_ppb) {
    uint8_t read_cmd[2] = {0x20, 0x08}; // Example command
    uint8_t data[6];
    
    if (sensirion_i2c_write(SGP30_I2C_ADDRESS, read_cmd, sizeof(read_cmd)) != 0) {
        return -1;
    }
    usleep(12000); // Wait for 12ms

    if (sensirion_i2c_read(SGP30_I2C_ADDRESS, data, sizeof(data)) != 0) {
        return -1;
    }

    *co2_eq_ppm = (data[0] << 8) | data[1];
    *tvoc_ppb = (data[3] << 8) | data[4];

    return 0;
}