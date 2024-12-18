#ifndef SGP40_I2C_H
#define SGP40_I2C_H

#ifdef __cplusplus
extern "C" {
#endif

#include "sensirion_config.h"

int16_t sgp40_measure_raw_signal(uint16_t relative_humidity, uint16_t temperature, uint16_t* sraw_voc);
int16_t sgp40_execute_self_test(uint16_t* test_result);
int16_t sgp40_turn_heater_off(void);
int16_t sgp40_get_serial_number(uint16_t* serial_number, uint8_t serial_number_size);

#ifdef __cplusplus
}
#endif

#endif /* SGP40_I2C_H */
