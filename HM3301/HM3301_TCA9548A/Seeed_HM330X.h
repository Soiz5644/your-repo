#ifndef _SEEED_HM330X_H
#define _SEEED_HM330X_H

#include <stdint.h>
#include "HM330XErrorCode.h"
#include "I2COperations.h"

#define DEFAULT_IIC_ADDR  0x40
#define SELECT_COMM_CMD   0X88

class HM330X : public I2COperations {
  public:
    HM330X(uint8_t IIC_ADDR = DEFAULT_IIC_ADDR);

    HM330XErrorCode init();

    HM330XErrorCode read_sensor_value(uint8_t* data, uint32_t data_len);

  private:
    HM330XErrorCode select_comm();
};

#endif