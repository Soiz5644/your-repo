#ifndef SGP30_I2C_H
#define SGP30_I2C_H

#include <stdint.h>

int sgp30_init();
int sgp30_read_measurements(uint16_t *co2_eq_ppm, uint16_t *tvoc_ppb);
int sgp30_get_baseline(uint16_t *co2, uint16_t *tvoc);
int sgp30_set_baseline(uint16_t co2, uint16_t tvoc);

#endif