#include <iostream>
#include <fstream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <ctime>
#include <cstdint>
#include <cstring>

#define TCA9548A_ADDR 0x70
#define I2C_BUS "/dev/i2c-1"

// Channel hex values
#define TCA_CHANNEL_0 0x1
#define TCA_CHANNEL_1 0x2
#define TCA_CHANNEL_2 0x4
#define TCA_CHANNEL_3 0x8

class TCA9548A {
public:
    TCA9548A(uint8_t address = TCA9548A_ADDR) : _address(address) {
        fd = open(I2C_BUS, O_RDWR);
        if (fd < 0) {
            std::cerr << "Failed to open the I2C bus.\n";
            exit(1);
        }

        if (ioctl(fd, I2C_SLAVE, _address) < 0) {
            std::cerr << "Failed to acquire bus access and/or talk to slave.\n";
            close(fd);
            exit(1);
        }
    }

    void selectChannel(uint8_t channel) {
        writeRegister(channel);
    }

private:
    int fd;
    uint8_t _address;

    void writeRegister(uint8_t value) {
        if (write(fd, &value, 1) != 1) {
            std::cerr << "Failed to write to the multiplexer.\n";
        } else {
            std::cout << "Switched to channel: " << std::hex << (int)value << std::dec << std::endl;
        }
    }
};

// SGP30 Sensor Class
class SGP30 {
public:
    SGP30(int fd) : fd(fd) {}

    void init_air_quality() {
        uint8_t cmd[2] = {0x20, 0x03};
        write_command(cmd, 2);
        usleep(10000);  // Wait for 10ms
    }

    void read_measurements(uint16_t &co2, uint16_t &tvoc) {
        uint8_t cmd[2] = {0x20, 0x08};
        write_command(cmd, 2);
        usleep(25000);  // Wait for 25ms

        uint8_t response[6];
        if (read_response(response, 6)) {
            co2 = (response[0] << 8) | response[1];
            tvoc = (response[3] << 8) | response[4];
        } else {
            co2 = 0;
            tvoc = 0;
        }
    }

private:
    int fd;

    void write_command(uint8_t *cmd, int length) {
        if (ioctl(fd, I2C_SLAVE, 0x58) < 0) {  // SGP30 I2C address
            std::cerr << "Failed to set I2C address for SGP30.\n";
            return;
        }
        if (write(fd, cmd, length) != length) {
            std::cerr << "Failed to write command to SGP30.\n";
        } else {
            std::cout << "Command sent to SGP30: " << std::hex << (int)cmd[0] << " " << (int)cmd[1] << std::dec << std::endl;
        }
    }

    bool read_response(uint8_t *response, int length) {
        if (read(fd, response, length) != length) {
            std::cerr << "Failed to read data from SGP30.\n";
            return false;
        }
        std::cout << "Response received from SGP30: ";
        for (int i = 0; i < length; ++i) {
            std::cout << std::hex << (int)response[i] << " ";
        }
        std::cout << std::dec << std::endl;
        return true;
    }
};

// SHT35 Sensor Class
class SHT35 {
public:
    SHT35(int fd) : fd(fd) {}

    void read_measurements(float &temperature, float &humidity) {
        uint8_t cmd[2] = {0x24, 0x00};
        write_command(cmd, 2);
        usleep(16000);  // Wait for 16ms

        uint8_t response[6];
        if (read_response(response, 6)) {
            temperature = -45 + (175 * (response[0] << 8 | response[1]) / 65535.0);
            humidity = 100 * ((response[3] << 8 | response[4]) / 65535.0);
        } else {
            temperature = 0;
            humidity = 0;
        }
    }

private:
    int fd;

    void write_command(uint8_t *cmd, int length) {
        if (ioctl(fd, I2C_SLAVE, 0x45) < 0) {  // SHT35 I2C address
            std::cerr << "Failed to set I2C address for SHT35.\n";
            return;
        }
        if (write(fd, cmd, length) != length) {
            std::cerr << "Failed to write command to SHT35.\n";
        } else {
            std::cout << "Command sent to SHT35: " << std::hex << (int)cmd[0] << " " << (int)cmd[1] << std::dec << std::endl;
        }
    }

    bool read_response(uint8_t *response, int length)