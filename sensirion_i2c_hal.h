#ifndef SENSIRION_I2C_HAL_H
#define SENSIRION_I2C_HAL_H

#include "sensirion_config.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

int16_t sensirion_i2c_hal_select_bus(uint8_t bus_idx);
void sensirion_i2c_hal_init(void);
void sensirion_i2c_hal_free(void);
int8_t sensirion_i2c_hal_read(uint8_t address, uint8_t* data, uint16_t count);
int8_t sensirion_i2c_hal_write(uint8_t address, const uint8_t* data, uint16_t count);
void sensirion_i2c_hal_sleep_usec(uint32_t useconds);
int8_t select_i2c_channel(uint8_t channel); // Add this line

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SENSIRION_I2C_HAL_H */