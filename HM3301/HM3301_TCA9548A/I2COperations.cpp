#include "I2COperations.h"
#include <iostream>

HM330XErrorCode I2COperations::IIC_write_byte(uint8_t reg, uint8_t byte) {
    int ret = wiringPiI2CWriteReg8(_IIC_ADDR, reg, byte);
    return ret == 0 ? NO_ERROR : ERROR_COMM;
}

HM330XErrorCode I2COperations::IIC_read_byte(uint8_t reg, uint8_t* byte) {
    int value = wiringPiI2CReadReg8(_IIC_ADDR, reg);
    if (value == -1) return ERROR_COMM;
    *byte = value;
    return NO_ERROR;
}

HM330XErrorCode I2COperations::IIC_write_16bit(uint8_t reg, uint16_t value) {
    int ret = wiringPiI2CWriteReg16(_IIC_ADDR, reg, value);
    return ret == 0 ? NO_ERROR : ERROR_COMM;
}

HM330XErrorCode I2COperations::IIC_read_16bit(uint8_t reg, uint16_t* value) {
    int val = wiringPiI2CReadReg16(_IIC_ADDR, reg);
    if (val == -1) return ERROR_COMM;
    *value = val;
    return NO_ERROR;
}

HM330XErrorCode I2COperations::IIC_read_bytes(uint8_t reg, uint8_t* data, uint32_t data_len) {
    for (uint32_t i = 0; i < data_len; i++) {
        int val = wiringPiI2CReadReg8(_IIC_ADDR, reg + i);
        if (val == -1) return ERROR_COMM;
        data[i] = val;
    }
    return NO_ERROR;
}

void I2COperations::set_iic_addr(uint8_t IIC_ADDR) {
    _IIC_ADDR = IIC_ADDR;
}

HM330XErrorCode I2COperations::IIC_SEND_CMD(uint8_t CMD) {
    int ret = wiringPiI2CWrite(_IIC_ADDR, CMD);
    if (ret != 0) {
        std::cerr << "IIC_SEND_CMD failed with error: " << ret << std::endl;
    }
    return ret == 0 ? NO_ERROR : ERROR_COMM;
}