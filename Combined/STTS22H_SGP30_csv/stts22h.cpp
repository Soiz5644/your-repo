#include "stts22h.h"
#include "sensirion_i2c_hal.h" // Assuming the same I2C HAL used for SHT40
#include <cstring> // Include for memcpy

// Define the context for the STTS22H sensor
stmdev_ctx_t stts22h_ctx;

// Function to write to STTS22H registers
static int32_t platform_write(void *handle, uint8_t reg, const uint8_t *bufp, uint16_t len) {
    uint8_t address = *(uint8_t*)handle;
    uint8_t data[len + 1];
    data[0] = reg;
    memcpy(&data[1], bufp, len);
    return sensirion_i2c_hal_write(address, data, len + 1);
}

// Function to read from STTS22H registers
static int32_t platform_read(void *handle, uint8_t reg, uint8_t *bufp, uint16_t len) {
    uint8_t address = *(uint8_t*)handle;
    if (sensirion_i2c_hal_write(address, &reg, 1) != 0) {
        return -1;
    }
    return sensirion_i2c_hal_read(address, bufp, len);
}

// Function to initialize the STTS22H sensor
int8_t stts22h_init() {
    uint8_t whoamI;
    stts22h_ctx.write_reg = platform_write;
    stts22h_ctx.read_reg = platform_read;
    static uint8_t i2c_address = STTS22H_I2C_ADDRESS;
    stts22h_ctx.handle = &i2c_address;

    // Check device ID
    if (stts22h_dev_id_get(&stts22h_ctx, &whoamI) != 0 || whoamI != STTS22H_ID) {
        return -1; // Sensor not found
    }

    // Set the output data rate
    if (stts22h_temp_data_rate_set(&stts22h_ctx, STTS22H_1Hz) != 0) {
        return -1; // Failed to set data rate
    }

    return 0; // Initialization successful
}

// Function to measure temperature from the STTS22H sensor
int8_t stts22h_measure_temperature(float* temperature) {
    int16_t raw_temp;
    if (stts22h_temperature_raw_get(&stts22h_ctx, &raw_temp) != 0) {
        return -1; // Failed to read temperature
    }
    *temperature = stts22h_from_lsb_to_celsius(raw_temp);
    return 0; // Measurement successful
}