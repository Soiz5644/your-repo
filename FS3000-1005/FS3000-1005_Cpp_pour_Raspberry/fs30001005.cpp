#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <pigpio.h>
#include <unistd.h>

#define FS3000_ADDRESS 0x28

class RTrobot_FS3000 {
public:
    enum DeviceType { FS3000_1005, FS3000_1015 };

    RTrobot_FS3000(DeviceType device = FS3000_1005) : this_device(device) {
        if (gpioInitialise() < 0) {
            throw std::runtime_error("Failed to initialize PiGPIO");
        }

        i2c_handle = i2cOpen(1, FS3000_ADDRESS, 0);
        if (i2c_handle < 0) {
            gpioTerminate();
            throw std::runtime_error("Failed to open I2C connection");
        }

        std::cout << "Thanks for using RTrobot module" << std::endl;
    }

    ~RTrobot_FS3000() {
        i2cClose(i2c_handle);
        gpioTerminate();
    }

    float readData() {
        uint8_t buffer[5];
        if (i2cReadDevice(i2c_handle, reinterpret_cast<char*>(buffer), 5) != 5) {
            throw std::runtime_error("Failed to read from I2C device");
        }

        uint8_t checksum = 0;
        for (int i = 0; i < 5; i++) {
            checksum += buffer[i];
        }

        if (checksum != 0) return 0;

        int fm_raw = ((buffer[1] << 8) | buffer[2]) & 0xFFF;

        const float *air_velocity_table = nullptr;
        const int *adc_table = nullptr;
        int table_size = 0;

        if (this_device == FS3000_1005) {
            air_velocity_table = fs3000_1005_air_velocity_table;
            adc_table = fs3000_1005_adc_table;
            table_size = 8;
        } else {
            air_velocity_table = fs3000_1015_air_velocity_table;
            adc_table = fs3000_1015_adc_table;
            table_size = 12;
        }

        if (fm_raw < adc_table[0] || fm_raw > adc_table[table_size]) return 0;

        int fm_level = 0;
        for (int i = 0; i < table_size; i++) {
            if (fm_raw > adc_table[i]) fm_level = i;
        }

        float fm_percentage = static_cast<float>(fm_raw - adc_table[fm_level]) / 
                              (adc_table[fm_level + 1] - adc_table[fm_level]);
        return (air_velocity_table[fm_level + 1] - air_velocity_table[fm_level]) * 
                fm_percentage + air_velocity_table[fm_level];
    }

private:
    int i2c_handle;
    DeviceType this_device;

    static constexpr float fs3000_1005_air_velocity_table[9] = {0, 1.07, 2.01, 3.0, 3.97, 4.96, 5.98, 6.99, 7.23};
    static constexpr int fs3000_1005_adc_table[9] = {409, 915, 1522, 2066, 2523, 2908, 3256, 3572, 3686};

    static constexpr float fs3000_1015_air_velocity_table[13] = {0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0, 13.0, 15.0};
    static constexpr int fs3000_1015_adc_table[13] = {409, 1203, 1597, 1908, 2187, 2400, 2629, 2801, 3006, 3178, 3309, 3563, 3686};
};

int main() {
    // Set custom configuration for pigpio
    gpioCfgSetInternals(PI_CFG_NOSIGHANDLER); // Avoid conflicts with signal handlers

    // Initialize pigpio library
    if (gpioInitialise() < 0) {
        std::cerr << "Failed to initialize PiGPIO. Ensure pigpiod is running on port 9999.\n";
        return 1;
    }

    // Your program logic
    RTrobot_FS3000 sensor(RTrobot_FS3000::FS3000_1005);  // Correct constructor call

    try {
        while (true) {
            float airSpeed = sensor.readData();
            if (airSpeed > 0) {
                std::cout << airSpeed << " m/s" << std::endl;
            }
            time_sleep(0.1); // Sleep for 100ms
        }
    } catch (...) {
        gpioTerminate(); // Cleanup pigpio library on exception
        throw;
    }

    gpioTerminate(); // Cleanup pigpio library before exiting
    return 0;
}
