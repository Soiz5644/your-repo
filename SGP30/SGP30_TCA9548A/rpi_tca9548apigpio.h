#ifndef RPI_TCA9548APIGPIO_H
#define RPI_TCA9548APIGPIO_H

#include <stdint.h>

class rpi_tca9548apigpio {
public:
    rpi_tca9548apigpio();
    ~rpi_tca9548apigpio();
    int init(int id);
    int set_channel(uint8_t channel);
    void no_channel();
  
private:
    int i2c_handle; // This replaces the previous 'fd'
};

#endif // RPI_TCA9548APIGPIO_H