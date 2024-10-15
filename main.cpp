#include <iostream>
#include <fstream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <ctime>
#include <cstdint>
#include <sstream>
#include <iomanip>

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
        std::cout << "Initializing TCA9548A..." << std::endl;
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
        std::cout << "Selecting channel: " << std::hex << (int)channel << std::dec << std::endl;
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

// SGP40 Sensor Class
class SGP40 {
public:
    SGP40(int fd) : fd(fd) {}

    void read_voc(uint16_t &voc) {
        uint8_t cmd[2] = {0x26, 0x0F};
        write_command(cmd, 2);
        usleep(50000);  // Wait for 50ms

        uint8_t response[3];
        if (read_response(response, 3)) {
            voc = (response[0] << 8) | response[1];
            std::cout << "VOC: " << voc << std::endl;
        } else {
            voc = 0;
        }
    }

private:
    int fd;

    void write_command(uint8_t *cmd, int length) {
        if (ioctl(fd, I2C_SLAVE, 0x59) < 0) {  // SGP40 I2C address
            std::cerr << "Failed to set I2C address for SGP40.\n";
            return;
        }
        if (write(fd, cmd, static_cast<size_t>(length)) != length) {
            std::cerr << "Failed to write command to SGP40.\n";
        } else {
            std::cout << "Command sent to SGP40: " << std::hex << (int)cmd[0] << " " << (int)cmd[1] << std::dec << std::endl;
        }
    }

    bool read_response(uint8_t *response, int length) {
        if (read(fd, response, static_cast<size_t>(length)) != length) {
            std::cerr << "Failed to read data from SGP40.\n";
            return false;
        }
        std::cout << "Response received from SGP40: ";
        for (int i = 0; i < length; ++i) {
            std::cout << std::hex << (int)response[i] << " ";
        }
        std::cout << std::dec << std::endl;
        return true;
    }
};

// SGP41 Sensor Class
class SGP41 {
public:
    SGP41(int fd) : fd(fd) {}

    void read_voc_nox(uint16_t &voc, uint16_t &nox) {
        uint8_t cmd[2] = {0x26, 0x0F};
        write_command(cmd, 2);
        usleep(50000);  // Wait for 50ms

        uint8_t response[6];
        if (read_response(response, 6)) {
            voc = (response[0] << 8) | response[1];
            nox = (response[3] << 8) | response[4];
            std::cout << "VOC: " << voc << ", NOx: " << nox << std::endl;
        } else {
            voc = 0;
            nox = 0;
        }
    }

private:
    int fd;

    void write_command(uint8_t *cmd, int length) {
        if (ioctl(fd, I2C_SLAVE, 0x59) < 0) {  // SGP41 I2C address
            std::cerr << "Failed to set I2C address for SGP41.\n";
            return;
        }
        if (write(fd, cmd, static_cast<size_t>(length)) != length) {
            std::cerr << "Failed to write command to SGP41.\n";
        } else {
            std::cout << "Command sent to SGP41: " << std::hex << (int)cmd[0] << " " << (int)cmd[1] << std::dec << std::endl;
        }
    }

    bool read_response(uint8_t *response, int length) {
        if (read(fd, response, static_cast<size_t>(length)) != length) {
            std::cerr << "Failed to read data from SGP41.\n";
            return false;
        }
        std::cout << "Response received from SGP41: ";
        for (int i = 0; i < length; ++i) {
            std::cout << std::hex << (int)response[i] << " ";
        }
        std::cout << std::dec << std::endl;
        return true;
    }
};

// SGP30 Sensor Class
class SGP30 {
public:
    SGP30(int fd) : fd(fd) {}

    void init_air_quality() {
        std::cout << "Initializing air quality..." << std::endl;
        uint8_t cmd[2] = {0x20, 0x03};
        write_command(cmd, 2);
        usleep(10000);  // Wait for 10ms
    }

    void read_measurements(uint16_t &co2, uint16_t &tvoc) {
        std::cout << "Reading measurements..." << std::endl;
        uint8_t cmd[2] = {0x20, 0x08};
        write_command(cmd, 2);
        usleep(25000);  // Wait for 25ms

        uint8_t response[6];
        if (read_response(response, 6)) {
            co2 = (response[0] << 8) | response[1];
            tvoc = (response[3] << 8) | response[4];
            std::cout << "CO2: " << co2 << ", TVOC: " << tvoc << std::endl;
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
        if (write(fd, cmd, static_cast<size_t>(length)) != length) {
            std::cerr << "Failed to write command to SGP30.\n";
        } else {
            std::cout << "Command sent to SGP30: " << std::hex << (int)cmd[0] << " " << (int)cmd[1] << std::dec << std::endl;
        }
    }

    bool read_response(uint8_t *response, int length) {
        if (read(fd, response, static_cast<size_t>(length)) != length) {
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
        std::cout << "Reading SHT35 measurements..." << std::endl;
        uint8_t cmd[2] = {0x24, 0x00};
        write_command(cmd, 2);
        usleep(16000);  // Wait for 16ms

        uint8_t response[6];
        if (read_response(response, 6)) {
            temperature = -45 + (175 * (response[0] << 8 | response[1]) / 65535.0);
            humidity = 100 * ((response[3] << 8 | response[4]) / 65535.0);
            std::cout << "Temperature: " << temperature << ", Humidity: " << humidity << std::endl;
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
        if (write(fd, cmd, static_cast<size_t>(length)) != length) {
            std::cerr << "Failed to write command to SHT35.\n";
        } else {
            std::cout << "Command sent to SHT35: " << std::hex << (int)cmd[0] << " " << (int)cmd[1] << std::dec << std::endl;
        }
    }

    bool read_response(uint8_t *response, int length) {
        if (read(fd, response, static_cast<size_t>(length)) != length) {
            std::cerr << "Failed to read data from SHT35.\n";
            return false;
        }
        std::cout << "Response received from SHT35: ";
        for (int i = 0; i < length; ++i) {
            std::cout << std::hex << (int)response[i] << " ";
        }
        std::cout << std::dec << std::endl;
        return true;
    }
};

void log_data(const std::string& filepath, const std::string& data) {
    std::ofstream file;
    file.open(filepath, std::ios::out | std::ios::app);
    if (file.is_open()) {
        file << data << std::endl;
        file.close();
    } else {
        std::cerr << "Failed to open file: " << filepath << std::endl;
    }
}

std::string get_current_time() {
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    char buffer[80];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &tm);
    return std::string(buffer);
}

int main() {
    std::cout << "Starting main..." << std::endl;
    int fd = open(I2C_BUS, O_RDWR);
    if (fd < 0) {
        std::cerr << "Failed to open the I2C bus.\n";
        return 1;
    }

    TCA9548A tca9548a;
    SGP30 sgp30(fd);
    SHT35 sht35(fd);
    SGP40 sgp40(fd);
    SGP41 sgp41(fd);

    std::string filepath = "sensor_data.csv";
    std::ofstream file(filepath, std::ios::out | std::ios::app);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filepath << std::endl;
        return 1;
    }
    file << "DateTime,Temperature,Humidity,VOC_raw,eCO2,VOC_SGP40,VOC_SGP41,NOx_SGP41\n";
    file.close();

    while (true) {
        std::string datetime = get_current_time();

        // Read SGP40 on channel 0
        tca9548a.selectChannel(TCA_CHANNEL_0);
        uint16_t voc_sgp40;
        sgp40.read_voc(voc_sgp40);

        // Read SGP41 on channel 1
        tca9548a.selectChannel(TCA_CHANNEL_1);
        uint16_t voc_sgp41, nox_sgp41;
        sgp41.read_voc_nox(voc_sgp41, nox_sgp41);

        // Read SGP30 on channel 2
        tca9548a.selectChannel(TCA_CHANNEL_2);
        sgp30.init_air_quality();
        uint16_t co2, tvoc;
        sgp30.read_measurements(co2, tvoc);

        // Read SHT35 on channel 3
        tca9548a.selectChannel(TCA_CHANNEL_3);
        float temperature, humidity;
        sht35.read_measurements(temperature, humidity);

        // Log data to CSV file
        std::stringstream ss;
        ss << datetime << ","
           << temperature << "," << humidity << ","
           << tvoc << "," << co2 << ","
           << voc_sgp40 << "," << voc_sgp41 << "," << nox_sgp41;
        log_data(filepath, ss.str());

        // Wait for a minute before next reading
        sleep(60);
    }

    close(fd);
    std::cout << "Finished main." << std::endl;
    return 0;
}
