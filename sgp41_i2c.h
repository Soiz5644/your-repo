#ifndef SGP41_I2C_H
#define SGP41_I2C_H

#ifdef __cplusplus
extern "C" {
#endif

#include "sensirion_config.h"

int16_t sgp41_measure_raw_signal(uint16_t relative_humidity, uint16_t temperature, uint16_t* sraw_voc, uint16_t* sraw_nox);
int16_t sgp41_execute_self_test(uint16_t* test_result);
int16_t sgp41_turn_heater_off(void);
int16_t sgp41_get_serial_number(uint16_t* serial_number, uint8_t serial_number_size);

#ifdef __cplusplus
}
#endif

#endif /* SGP41_I2C_H */