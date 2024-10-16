#define _DEFAULT_SOURCE
#include "sensirion_i2c_hal.h"
#include "sensirion_common.h"
#include "sensirion_config.h"

#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define I2C_DEVICE_PATH "/dev/i2c-1"
#define I2C_SLAVE 0x0703
#define I2C_WRITE_FAILED -1
#define I2C_READ_FAILED -1
#define TCA9548A_ADDRESS 0x70 // Default address for TCA9548A

static int i2c_device = -1;
static uint8_t i2c_address = 0;

int8_t select_i2c_channel(uint8_t channel) {
    uint8_t data = 1 << channel; // Select the channel by setting the corresponding bit
    if (i2c_address != TCA9548A_ADDRESS) {
        ioctl(i2c_device, I2C_SLAVE, TCA9548A_ADDRESS);
        i2c_address = TCA9548A_ADDRESS;
    }
    if (write(i2c_device, &data, 1) != 1) {
        return I2C_WRITE_FAILED;
    }
    return 0;
}

void sensirion_i2c_hal_init(void) {
    i2c_device = open(I2C_DEVICE_PATH, O_RDWR);
    if (i2c_device == -1)
        return;
    // Select the initial channel (e.g., channel 0)
    select_i2c_channel(0);
}

void sensirion_i2c_hal_free(void) {
    if (i2c_device >= 0)
        close(i2c_device);
}

int8_t sensirion_i2c_hal_read(uint8_t address, uint8_t* data, uint16_t count) {
    if (i2c_address != address) {
        ioctl(i2c_device, I2C_SLAVE, address);
        i2c_address = address;
    }

    if (read(i2c_device, data, count) != count) {
        return I2C_READ_FAILED;
    }
    return 0;
}

int8_t sensirion_i2c_hal_write(uint8_t address, const uint8_t* data, uint16_t count) {
    if (i2c_address != address) {
        ioctl(i2c_device, I2C_SLAVE, address);
        i2c_address = address;
    }

    if (write(i2c_device, data, count) != count) {
        return I2C_WRITE_FAILED;
    }
    return 0;
}

void sensirion_i2c_hal_sleep_usec(uint32_t useconds) {
    usleep(useconds);
}
