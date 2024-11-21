#include <iostream>
#include <fstream>
#include <array>
#include <chrono>
#include <ctime>
#include <unistd.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <cstring>  // Include this header for strerror

#define I2C_SLAVE 0x0703

class RTrobot_FS3000 {
public:
    static const int FS3000_ADDRESS = 0x28;
    static const int FS3000_1005 = 0x00;
    static const int FS3000_1015 = 0x01;

    std::array<float, 13> fs3000_1015_air_velocity_table = {0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0, 13.0, 15.0};
    std::array<int, 13> fs3000_1015_adc_table = {409, 1203, 1597, 1908, 2187, 2400, 2629, 2801, 3006, 3178, 3309, 3563, 3686};
    std::array<float, 9> fs3000_1005_air_velocity_table = {0, 1.07, 2.01, 3.0, 3.97, 4.96, 5.98, 6.99, 7.23};
    std::array<int, 10> fs3000_1005_adc_table = {409, 915, 1522, 2066, 2523, 2908, 3256, 3572, 3686, 3178};

    RTrobot_FS3000(int i2c_addr = FS3000_ADDRESS, int device = FS3000_1005) {
        this->i2c_addr = i2c_addr;
        this->device = device;

        bool initialized = false;
        std::array<int, 2> i2c_buses = {1, 2};
        for (int bus : i2c_buses) {
            char filename[20];
            snprintf(filename, 19, "/dev/i2c-%d", bus);
            if ((file_i2c = open(filename, O_RDWR)) < 0) {
                std::cerr << "Failed to open the i2c bus " << filename << std::endl;
                continue;
            }
            if (ioctl(file_i2c, I2C_SLAVE, i2c_addr) < 0) {
                std::cerr << "Failed to acquire bus access and/or talk to slave on " << filename << std::endl;
                close(file_i2c);
                continue;
            }
            initialized = true;
            std::cout << "FS3000 sensor initialized correctly on " << filename << std::endl;
            break;
        }

        if (!initialized) {
            std::cerr << "Failed to initialize FS3000 sensor on any I2C bus." << std::endl;
            exit(1);
        }
    }

    float FS3000_ReadData() {
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
            fm_percentage = (fm_raw - fs3000_1005_adc_table[fm_level]) / float(fs3000_1005_adc_table[fm_level + 1] - fs3000_1005_adc_table[fm_level]);
            return (fs3000_1005_air_velocity_table[fm_level + 1] - fs3000_1005_air_velocity_table[fm_level]) * fm_percentage + fs3000_1005_air_velocity_table[fm_level];
        } else {
            if (fm_raw < fs3000_1015_adc_table[0] || fm_raw > fs3000_1015_adc_table[12]) {
                return 0;
            }
            for (int i = 0; i < 12; i++) {
                if (fm_raw > fs3000_1015_adc_table[i]) {
                    fm_level = i;
                }
            }
            fm_percentage = (fm_raw - fs3000_1015_adc_table[fm_level]) / float(fs3000_1015_adc_table[fm_level + 1] - fs3000_1015_adc_table[fm_level]);
            return (fs3000_1015_air_velocity_table[fm_level + 1] - fs3000_1015_air_velocity_table[fm_level]) * fm_percentage + fs3000_1015_air_velocity_table[fm_level];
        }
    }

private:
    int file_i2c;
    int i2c_addr;
    int device;

    int FS3000_i2c_read() {
        unsigned char buffer[5];
        int length = 5;
        usleep(15000);  // Add delay before reading
        if (read(file_i2c, buffer, length) != length) {
            std::cerr << "Failed to read from the i2c bus. Error: " << strerror(errno) << std::endl;
            return -1;
        } else {
            std::cout << "Raw I2C data: ";
            for (int i = 0; i < length; i++) {
                std::cout << "0x" << std::hex << (int)buffer[i] << " ";
            }
            std::cout << std::endl;
            int sum = 0;
            for (int i = 0; i < 5; i++) {
                sum += buffer[i];
            }
            if ((sum & 0xff) != 0x00) {
                std::cerr << "Checksum error in the data read from the i2c bus" << std::endl;
                return -1;
            } else {
                return (buffer[1] * 256 + buffer[2]) & 0xfff;
            }
        }
    }
};

int main() {
    RTrobot_FS3000 fs(RTrobot_FS3000::FS3000_1005);
    while (true) {
        float speed = fs.FS3000_ReadData();
        if (speed != 0) {
            auto now = std::chrono::system_clock::now();
            std::time_t now_time = std::chrono::system_clock::to_time_t(now);
            std::cout << std::ctime(&now_time) << " - " << speed << " m/s" << std::endl;
        }
        usleep(100000);  // Sleep for 0.1 seconds
    }
    return 0;
}