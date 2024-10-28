#include "sgp30_i2c.h"
#include "sensirion_i2c_hal.h"
#include <unistd.h> // for usleep
#include <math.h> // for exp()


#define SGP30_I2C_ADDRESS 0x58

int sgp30_init() {
    // Initialization commands for SGP30
    // (Example: send command 0x2003)
    uint8_t init_cmd[2] = {0x20, 0x03};
    return sensirion_i2c_hal_write(SGP30_I2C_ADDRESS, init_cmd, sizeof(init_cmd));
}

int sgp30_read_measurements(uint16_t *co2_eq_ppm, uint16_t *tvoc_ppb) {
    uint8_t read_cmd[2] = {0x20, 0x08}; // Example command
    uint8_t data[6];
    
    if (sensirion_i2c_hal_write(SGP30_I2C_ADDRESS, read_cmd, sizeof(read_cmd)) != 0) {
        return -1;
    }
    usleep(12000); // Wait for 12ms

    if (sensirion_i2c_hal_read(SGP30_I2C_ADDRESS, data, sizeof(data)) != 0) {
        return -1;
    }

    *co2_eq_ppm = (data[0] << 8) | data[1];
    *tvoc_ppb = (data[3] << 8) | data[4];

    return 0;
}

int sgp30_get_baseline(uint16_t *co2, uint16_t *tvoc) {
    uint8_t baseline_cmd[2] = {0x20, 0x15};
    uint8_t data[6];

    if (sensirion_i2c_hal_write(SGP30_I2C_ADDRESS, baseline_cmd, sizeof(baseline_cmd)) != 0) {
        return -1;
    }
    usleep(10000); // Wait for 10ms

    if (sensirion_i2c_hal_read(SGP30_I2C_ADDRESS, data, sizeof(data)) != 0) {
        return -1;
    }

    *co2 = (data[0] << 8) | data[1];
    *tvoc = (data[3] << 8) | data[4];

    return 0;
}

int sgp30_set_baseline(uint16_t co2, uint16_t tvoc) {
    uint8_t baseline_cmd[8] = {0x20, 0x1E, 
                               (uint8_t)(tvoc >> 8), (uint8_t)(tvoc & 0xFF), 
                               sensirion_common_generate_crc((uint8_t*)&tvoc, 2),
                               (uint8_t)(co2 >> 8), (uint8_t)(co2 & 0xFF), 
                               sensirion_common_generate_crc((uint8_t*)&co2, 2)};

    return sensirion_i2c_hal_write(SGP30_I2C_ADDRESS, baseline_cmd, sizeof(baseline_cmd));
}

int sgp30_set_relative_humidity(float T, float RH) {
    // Calculate absolute humidity based on temperature and relative humidity
    float absoluteHumidity = (2.167 * 6.112) * RH * exp((17.62 * T) / (243.12 + T)) / (273.15 + T);
    return sgp30_set_absolute_humidity(absoluteHumidity);
}

int sgp30_set_absolute_humidity(float absoluteHumidity) {
    uint16_t AH = (uint16_t)(absoluteHumidity * 256.0);
    uint8_t ah_cmd[5] = {0x20, 0x61, (uint8_t)(AH >> 8), (uint8_t)(AH & 0xFF), sensirion_common_generate_crc((uint8_t*)&AH, 2)};
    return sensirion_i2c_hal_write(SGP30_I2C_ADDRESS, ah_cmd, sizeof(ah_cmd));
}