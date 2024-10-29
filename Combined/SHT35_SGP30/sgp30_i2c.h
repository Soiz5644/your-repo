#ifndef SGP30_I2C_H
#define SGP30_I2C_H

#include <stdint.h>

int sgp30_init();
int sgp30_iaq_init();
int sgp30_read_measurements(uint16_t *co2_eq_ppm, uint16_t *tvoc_ppb);
int sgp30_measure_iaq(uint16_t *co2_eq_ppm, uint16_t *tvoc_ppb);
int sgp30_get_baseline(uint16_t *co2, uint16_t *tvoc);
int sgp30_get_iaq_baseline(uint16_t *co2, uint16_t *tvoc);
int sgp30_set_baseline(uint16_t co2, uint16_t tvoc);
int sgp30_set_iaq_baseline(uint16_t co2, uint16_t tvoc);
int sgp30_get_tvoc_inceptive_baseline(uint16_t *tvoc);
int sgp30_set_tvoc_baseline(uint16_t tvoc);
int sgp30_set_relative_humidity(float T, float RH);
int sgp30_set_absolute_humidity(float absoluteHumidity);

#endif