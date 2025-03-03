#include "stts22h_reg.h"
#include <wiringPiI2C.h>
#include <unistd.h>
#include <stdio.h>

int fd; // File descriptor for I2C device

int32_t platform_write(void *handle, uint8_t reg, const uint8_t *bufp, uint16_t len) {
    for (int i = 0; i < len; i++) {
        if (wiringPiI2CWriteReg8(fd, reg + i, bufp[i]) < 0) {
            return -1;
        }
    }
    return 0;
}

int32_t platform_read(void *handle, uint8_t reg, uint8_t *bufp, uint16_t len) {
    for (int i = 0; i < len; i++) {
        int val = wiringPiI2CReadReg8(fd, reg + i);
        if (val < 0) {
            return -1;
        }
        bufp[i] = val;
    }
    return 0;
}

void platform_delay(uint32_t millisec) {
    usleep(millisec * 1000);
}

int main() {
    stmdev_ctx_t dev_ctx;
    dev_ctx.write_reg = platform_write;
    dev_ctx.read_reg = platform_read;
    dev_ctx.mdelay = platform_delay;

    // Use the detected I2C address 0x3c
    fd = wiringPiI2CSetup(0x3c);
    if (fd < 0) {
        printf("Failed to initialize I2C communication.\n");
        return -1;
    }

    uint8_t who_am_i;
    if (stts22h_dev_id_get(&dev_ctx, &who_am_i) == 0) {
        printf("WHO_AM_I: 0x%02X\n", who_am_i);
        if (who_am_i != STTS22H_ID) {
            printf("Unexpected WHO_AM_I value.\n");
            return -1;
        }
    } else {
        printf("Error reading WHO_AM_I.\n");
        return -1;
    }

    // Set the sensor to one-shot mode to take a single measurement
    if (stts22h_temp_data_rate_set(&dev_ctx, STTS22H_ONE_SHOT) != 0) {
        printf("Error setting data rate.\n");
        return -1;
    }

    // Wait for the measurement to complete
    platform_delay(100);

    // Read the raw temperature value
    int16_t temperature_raw;
    if (stts22h_temperature_raw_get(&dev_ctx, &temperature_raw) == 0) {
        float temperature_c = stts22h_from_lsb_to_celsius(temperature_raw);
        printf("Temperature: %.2f °C\n", temperature_c);
    } else {
        printf("Error reading temperature.\n");
        return -1;
    }

    return 0;
}