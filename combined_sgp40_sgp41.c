#include <inttypes.h>
#include <stdio.h>  // printf
#include "sensirion_common.h"
#include "sensirion_i2c_hal.h"
#include "sgp41_i2c.h"
#include "sgp40_i2c.h"

int main(void) {
    int16_t error = 0;
    uint8_t channel = 1;  // Change this to the desired channel (0-7)

    // Parameters for deactivated humidity compensation:
    uint16_t default_rh = 0x8000;
    uint16_t default_t = 0x6666;

    sensirion_i2c_hal_init();

    // Select the I2C channel on the multiplexer
    error = select_i2c_channel(channel);
    if (error) {
        printf("Error selecting I2C channel %u: %i\n", channel, error);
        return error;
    }

    uint16_t sgp41_serial_number[3];
    error = sgp41_get_serial_number(sgp41_serial_number, sizeof(sgp41_serial_number) / sizeof(sgp41_serial_number[0]));
    if (error) {
        printf("Error executing sgp41_get_serial_number(): %i\n", error);
    } else {
        printf("SGP41 Serial number: %" PRIu64 "\n",
               (((uint64_t)sgp41_serial_number[0]) << 32) |
               (((uint64_t)sgp41_serial_number[1]) << 16) |
               ((uint64_t)sgp41_serial_number[2]));
    }

    uint16_t sgp40_serial_number[3];
    uint8_t sgp40_serial_number_size = 3;
    error = sgp40_get_serial_number(sgp40_serial_number, sgp40_serial_number_size);
    if (error) {
        printf("Error executing sgp40_get_serial_number(): %i\n", error);
    } else {
        printf("SGP40 Serial number: %" PRIu64 "\n",
               (((uint64_t)sgp40_serial_number[0]) << 32) |
               (((uint64_t)sgp40_serial_number[1]) << 16) |
               ((uint64_t)sgp40_serial_number[2]));
    }

    // SGP41 conditioning during 15 seconds before measuring
    for (int i = 0; i < 15; i++) {
        uint16_t sraw_voc;

        sensirion_i2c_hal_sleep_usec(1000000);  // 1 second delay

        error = sgp41_execute_conditioning(default_rh, default_t, &sraw_voc);
        if (error) {
            printf("Error executing sgp41_execute_conditioning(): %i\n", error);
        } else {
            printf("SGP41 SRAW VOC: %u\n", sraw_voc);
            printf("SGP41 SRAW NOx: conditioning\n");
        }
    }

    // Start Measurement for both sensors
    for (int i = 0; i < 60; i++) {
        uint16_t sraw_voc_sgp41, sraw_nox_sgp41;
        uint16_t sraw_voc_sgp40;

        sensirion_i2c_hal_sleep_usec(1000000);  // 1 second delay

        // SGP41 measurement
        error = sgp41_measure_raw_signals(default_rh, default_t, &sraw_voc_sgp41, &sraw_nox_sgp41);
        if (error) {
            printf("Error executing sgp41_measure_raw_signals(): %i\n", error);
        } else {
            printf("SGP41 SRAW VOC: %u\n", sraw_voc_sgp41);
            printf("SGP41 SRAW NOx: %u\n", sraw_nox_sgp41);
        }

        // SGP40 measurement
        error = sgp40_measure_raw_signal(default_rh, default_t, &sraw_voc_sgp40);
        if (error) {
            printf("Error executing sgp40_measure_raw_signal(): %i\n", error);
        } else {
            printf("SGP40 SRAW VOC: %u\n", sraw_voc_sgp40);
        }
    }

    return 0;
}