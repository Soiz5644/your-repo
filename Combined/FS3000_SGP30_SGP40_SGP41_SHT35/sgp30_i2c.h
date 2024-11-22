#ifndef SGP30_I2C_H
#define SGP30_I2C_H

#include <stdint.h>

class SGP30 {
public:
    SGP30();
    int init();
    int measure();
    int readRaw();
    uint16_t getTVOC();
    uint16_t getCO2();
    uint16_t getH2_raw();
    uint16_t getEthanol_raw();
    float getH2();
    float getEthanol();

private:
    uint16_t co2_eq_ppm;
    uint16_t tvoc_ppb;
    uint16_t h2_raw;
    uint16_t ethanol_raw;
    uint16_t srefH2;
    uint16_t srefEthanol;
};

#endif // SGP30_I2C_H