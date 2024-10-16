#include "sgp41_i2c.h"
#include "sensirion_common.h"
#include "sensirion_i2c.h"
#include "sensirion_i2c_hal.h"

#define SGP41_I2C_ADDRESS 0x59

int16_t sgp41_measure_raw_signal(uint16_t relative_humidity, uint16_t temperature, uint16_t* sraw_voc, uint16_t* sraw_nox) {
    int16_t error;
    uint8_t buffer[8];
    uint16_t offset = 0;
    offset = sensirion_i2c_add_command_to_buffer(&buffer[0], offset, 0x261C);

    offset = sensirion_i2c_add_uint16_t_to_buffer(&buffer[0], offset, relative_humidity);
    offset = sensirion_i2c_add_uint16_t_to_buffer(&buffer[0], offset, temperature);

    error = sensirion_i2c_write_data(SGP41_I2C_ADDRESS, &buffer[0], offset);
    if (error) {
        return error;
    }

    sensirion_i2c_hal_sleep_usec(30000);

    error = sensirion_i2c_read_data_inplace(SGP41_I2C_ADDRESS, &buffer[0], 6);
    if (error) {
        return error;
    }
    *sraw_voc = sensirion_common_bytes_to_uint16_t(&buffer[0]);
    *sraw_nox = sensirion_common_bytes_to_uint16_t(&buffer[3]);
    return NO_ERROR;
}

int16_t sgp41_execute_self_test(uint16_t* test_result) {
    int16_t error;
    uint8_t buffer[3];
    uint16_t offset = 0;
    offset = sensirion_i2c_add_command_to_buffer(&buffer[0], offset, 0x280E);

    error = sensirion_i2c_write_data(SGP41_I2C_ADDRESS, &buffer[0], offset);
    if (error) {
        return error;
    }

    sensirion_i2c_hal_sleep_usec(320000);

    error = sensirion_i2c_read_data_inplace(SGP41_I2C_ADDRESS, &buffer[0], 2);
    if (error) {
        return error;
    }
    *test_result = sensirion_common_bytes_to_uint16_t(&buffer[0]);
    return NO_ERROR;
}

int16_t sgp41_turn_heater_off(void) {
    int16_t error;
    uint8_t buffer[2];
    uint16_t offset = 0;
    offset = sensirion_i2c_add_command_to_buffer(&buffer[0], offset, 0x3615);

    error = sensirion_i2c_write_data(SGP41_I2C_ADDRESS, &buffer[0], offset);
    if (error) {
        return error;
    }
    sensirion_i2c_hal_sleep_usec(1000);
    return NO_ERROR;
}

int16_t sgp41_get_serial_number(uint16_t* serial_number, uint8_t serial_number_size) {
    int16_t error;
    uint8_t buffer[9];
    uint16_t offset = 0;
    offset = sensirion_i2c_add_command_to_buffer(&buffer[0], offset, 0x3682);

    error = sensirion_i2c_write_data(SGP41_I2C_ADDRESS, &buffer[0], offset);
    if (error) {
        return error;
    }

    sensirion_i2c_hal_sleep_usec(1000);

    error = sensirion_i2c_read_data_inplace(SGP41_I2C_ADDRESS, &buffer[0], 6);
    if (error) {
        return error;
    }

    serial_number[0] = sensirion_common_bytes_to_uint16_t(&buffer[0]);
    serial_number[1] = sensirion_common_bytes_to_uint16_t(&buffer[2]);
    serial_number[2] = sensirion_common_bytes_to_uint16_t(&buffer[4]);

    return NO_ERROR;
}

int16_t sgp41_execute_conditioning(uint16_t default_rh, uint16_t default_t, uint16_t *sraw_voc) {
    // Add the implementation for the sgp41_execute_conditioning function here
    // This function should execute the conditioning process for the SGP41 sensor
    return 0; // Return 0 if successful, or an error code if not
}

int16_t sgp41_measure_raw_signals(uint16_t default_rh, uint16_t default_t, uint16_t *sraw_voc, uint16_t *sraw_nox) {
    int16_t error;
    uint8_t buffer[8];
    uint16_t offset = 0;

    offset = sensirion_i2c_add_command_to_buffer(&buffer[0], offset, 0x261C);
    offset = sensirion_i2c_add_uint16_t_to_buffer(&buffer[0], offset, default_rh);
    offset = sensirion_i2c_add_uint16_t_to_buffer(&buffer[0], offset, default_t);

    error = sensirion_i2c_write_data(SGP41_I2C_ADDRESS, &buffer[0], offset);
    if (error) {
        return error;
    }

    sensirion_i2c_hal_sleep_usec(30000);

    error = sensirion_i2c_read_data_inplace(SGP41_I2C_ADDRESS, &buffer[0], 6);
    if (error) {
        return error;
    }

    *sraw_voc = sensirion_common_bytes_to_uint16_t(&buffer[0]);
    *sraw_nox = sensirion_common_bytes_to_uint16_t(&buffer[3]);
    return NO_ERROR;
}
