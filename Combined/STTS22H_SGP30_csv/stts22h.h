#ifndef STTS22H_H
#define STTS22H_H

#include "stts22h_reg.h"

// Define the I2C address for the STTS22H sensor
#define STTS22H_I2C_ADDRESS 0x71

// Define the context for the STTS22H sensor
stmdev_ctx_t stts22h_ctx;

// Function to initialize the STTS22H sensor
int8_t stts22h_init();

// Function to measure temperature from the STTS22H sensor
int8_t stts22h_measure_temperature(float* temperature);

#endif // STTS22H_H