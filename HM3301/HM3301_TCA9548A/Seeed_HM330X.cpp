#include "Seeed_HM330X.h"
#include <wiringPi.h>
#include <wiringPiI2C.h>

HM330X::HM330X(uint8_t IIC_ADDR) {
    set_iic_addr(IIC_ADDR);
}

HM330XErrorCode HM330X::select_comm() {
    return IIC_SEND_CMD(SELECT_COMM_CMD);
}

HM330XErrorCode HM330X::init() {
    return select_comm();
}

HM330XErrorCode HM330X::read_sensor_value(uint8_t* data, uint32_t data_len) {
    uint32_t time_out_count = 0;
    HM330XErrorCode ret = NO_ERROR;
    // Request data from the sensor
    wiringPiI2CReadReg8(this->_IIC_ADDR, 29);
    while (data_len != wiringPiI2CReadReg8(this->_IIC_ADDR, 29)) {
        time_out_count++;
        if (time_out_count > 10) {
            return ERROR_COMM;
        }
        delay(1);
    }
    for (int i = 0; i < data_len; i++) {
        data[i] = wiringPiI2CReadReg8(this->_IIC_ADDR, i);
    }
    return ret;
}