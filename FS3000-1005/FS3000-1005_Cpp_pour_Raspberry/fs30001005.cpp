#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <iostream>
#include <unistd.h>
#include <cstdint>
#include <cstring>  // Include cstring for strerror

#define FS3000_ADDRESS 0x28
#define FS3000_1005 0x00

class RTrobot_FS3000 {
public:
    RTrobot_FS3000(int i2c_addr = FS3000_ADDRESS, int device = FS3000_1005);
    float FS3000_ReadData();

private:
    int file_i2c;
    int i2c_addr;
    int device;

    int FS3000_i2c_read();
    const float fs3000_1005_air_velocity_table[9] = {0, 1.07, 2.01, 3.0, 3.97, 4.96, 5.98, 6.99, 7.23};
    const int fs3000_1005_adc_table[9] = {409, 915, 1522, 2066, 2523, 2908, 3256, 3572, 3686};
};

RTrobot_FS3000::RTrobot_FS3000(int i2c_addr, int device)
    : i2c_addr(i2c_addr), device(device) {
    wiringPiSetup();
    file_i2c = wiringPiI2CSetup(i2c_addr);
    if (file_i2c == -1) {
        std::cerr << "Failed to initialize I2C communication." << std::endl;
        exit(1);
    }
    std::cout << "FS3000 sensor initialized correctly." << std::endl;
}

float RTrobot_FS3000::FS3000_ReadData() {
    int fm_raw = FS3000_i2c_read();
    if (fm_raw == -1) {
        return 0;
    }

    int fm_level = 0;
    float fm_percentage = 0.0;

    if (device == FS3000_1005) {
        if (fm_raw < fs3000_1005_adc_table[0] || fm_raw > fs3000_1005_adc_table[8]) {
            return 0;
        }
        for (int i = 0; i < 8; i++) {
            if (fm_raw > fs3000_1005_adc_table[i]) {
                fm_level = i;
            }
        }
        fm_percentage = (fm_raw - fs3000_1005_adc_table[fm_level]) / 
                        float(fs3000_1005_adc_table[fm_level + 1] - fs3000_1005_adc_table[fm_level]);
        return (fs3000_1005_air_velocity_table[fm_level + 1] - fs3000_1005_air_velocity_table[fm_level]) * 
               fm_percentage + fs3000_1005_air_velocity_table[fm_level];
    }

    return 0;
}

int RTrobot_FS3000::FS3000_i2c_read() {
    uint8_t buffer[5];
    usleep(15000);  // Delay before reading
    if (read(file_i2c, buffer, 5) != 5) {
        std::cerr << "Failed to read from the i2c bus. Error: " << strerror(errno) << std::endl;
        return -1;
    } else {
        int sum = 0;
        for (int i = 0; i < 5; i++) {
            sum += buffer[i];
        }
        if (sum & 0xff != 0x00) {
            return -1;
        } else {
            return (buffer[1] * 256 + buffer[2]) & 0xfff;
        }
    }
}

int main() {
    RTrobot_FS3000 fs3000;
    while (true) {
        float velocity = fs3000.FS3000_ReadData();
        std::cout << "Air Velocity: " << velocity << " m/s" << std::endl;
        sleep(1);
    }
    return 0;
}