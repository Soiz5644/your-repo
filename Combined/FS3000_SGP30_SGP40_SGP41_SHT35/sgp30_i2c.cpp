#include "sgp30_i2c.h"
#include "sensirion_i2c_hal.h"
#include <unistd.h> // for usleep
#include <math.h> // for exp()

#define SGP30_I2C_ADDRESS 0x58

SGP30::SGP30() : srefH2(13119), srefEthanol(18472) {}

int SGP30::init() {
    uint8_t init_cmd[2] = {0x20, 0x03};
    return sensirion_i2c_hal_write(SGP30_I2C_ADDRESS, init_cmd, sizeof(init_cmd));
}

int SGP30::measure() {
    uint8_t read_cmd[2] = {0x20, 0x08}; // Example command
    uint8_t data[6];

    if (sensirion_i2c_hal_write(SGP30_I2C_ADDRESS, read_cmd, sizeof(read_cmd)) != 0) {
        return -1;
    }
    usleep(12000); // Wait for 12ms

    if (sensirion_i2c_hal_read(SGP30_I2C_ADDRESS, data, sizeof(data)) != 0) {
        return -1;
    }

    co2_eq_ppm = (data[0] << 8) | data[1];
    tvoc_ppb = (data[3] << 8) | data[4];

    return 0;
}

int SGP30::readRaw() {
    uint8_t read_cmd[2] = {0x20, 0x50}; // Raw data command
    uint8_t data[6];

    if (sensirion_i2c_hal_write(SGP30_I2C_ADDRESS, read_cmd, sizeof(read_cmd)) != 0) {
        return -1;
    }
    usleep(25000); // Wait for 25ms

    if (sensirion_i2c_hal_read(SGP30_I2C_ADDRESS, data, sizeof(data)) != 0) {
        return -1;
    }

    h2_raw = (data[0] << 8) | data[1];
    ethanol_raw = (data[3] << 8) | data[4];

    return 0;
}

uint16_t SGP30::getTVOC() {
    return tvoc_ppb;
}

uint16_t SGP30::getCO2() {
    return co2_eq_ppm;
}

uint16_t SGP30::getH2_raw() {
    return h2_raw;
}

uint16_t SGP30::getEthanol_raw() {
    return ethanol_raw;
}

float SGP30::getH2() {
    float cref = 0.5;  // ppm
    return cref * exp((srefH2 - h2_raw) * 1.953125e-3);
}

float SGP30::getEthanol() {
    float cref = 0.4;  // ppm
    return cref * exp((srefEthanol - ethanol_raw) * 1.953125e-3);
}

int SGP30::set_absolute_humidity(float absoluteHumidity) {
    uint16_t AH = (uint16_t)(absoluteHumidity * 256.0);
    uint8_t ah_cmd[5] = {0x20, 0x61, (uint8_t)(AH >> 8), (uint8_t)(AH & 0xFF), sensirion_common_generate_crc((uint8_t*)&AH, 2)};
    return sensirion_i2c_hal_write(SGP30_I2C_ADDRESS, ah_cmd, sizeof(ah_cmd));
}

int SGP30::set_relative_humidity(float T, float RH) {
    // Calculate absolute humidity based on temperature and relative humidity
    float absoluteHumidity = (2.167 * 6.112) * RH * exp((17.62 * T) / (243.12 + T)) / (273.15 + T);
    return set_absolute_humidity(absoluteHumidity);
}