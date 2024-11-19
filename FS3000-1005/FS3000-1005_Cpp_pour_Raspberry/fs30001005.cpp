#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <iostream>

class FS3000 {
public:
    FS3000();
    bool begin();
    uint16_t readRaw();
    float readMetersPerSecond();
    float readMilesPerHour();
    void setRange(uint8_t range);

private:
    void readData(uint8_t* buffer_in);
    bool checksum(uint8_t* data_in);
    int fd;
    uint8_t _buff[5];
    uint8_t _range;
    float _mpsDataPoint[13];
    int _rawDataPoint[13];
};

FS3000::FS3000() {}

bool FS3000::begin() {
    fd = wiringPiI2CSetup(0x28); // Replace with your sensor's I2C address
    return (fd != -1);
}

void FS3000::setRange(uint8_t range) {
    _range = range;
    const float mpsDataPoint_7_mps[9] = {0, 1.07, 2.01, 3.00, 3.97, 4.96, 5.98, 6.99, 7.23};
    const int rawDataPoint_7_mps[9] = {409, 915, 1522, 2066, 2523, 2908, 3256, 3572, 3686};
    const float mpsDataPoint_15_mps[13] = {0, 2.00, 3.00, 4.00, 5.00, 6.00, 7.00, 8.00, 9.00, 10.00, 11.00, 13.00, 15.00};
    const int rawDataPoint_15_mps[13] = {409, 1203, 1597, 1908, 2187, 2400, 2629, 2801, 3006, 3178, 3309, 3563, 3686};

    if (range == 7) {
        for (int i = 0; i < 9; i++) {
            _mpsDataPoint[i] = mpsDataPoint_7_mps[i];
            _rawDataPoint[i] = rawDataPoint_7_mps[i];
        }
    } else if (range == 15) {
        for (int i = 0; i < 13; i++) {
            _mpsDataPoint[i] = mpsDataPoint_15_mps[i];
            _rawDataPoint[i] = rawDataPoint_15_mps[i];
        }
    }
}

uint16_t FS3000::readRaw() {
    readData(_buff);
    if (!checksum(_buff)) {
        return 0;
    }

    uint16_t airflowRaw = 0;
    uint8_t data_high_byte = _buff[1];
    uint8_t data_low_byte = _buff[2];
    data_high_byte &= 0x0F;
    airflowRaw = (data_high_byte << 8) | data_low_byte;
    
    return airflowRaw;
}

float FS3000::readMetersPerSecond() {
    float airflowMps = 0.0;
    int airflowRaw = readRaw();
    int dataPointsNum = (_range == 7) ? 9 : 13;
    int data_position = 0;

    for (int i = 0; i < dataPointsNum; i++) {
        if (airflowRaw > _rawDataPoint[i]) {
            data_position = i;
        }
    }

    if (airflowRaw <= 409) return 0;
    if (airflowRaw >= 3686) return (_range == 7) ? 7.23 : 15.00;

    int window_size = (_rawDataPoint[data_position + 1] - _rawDataPoint[data_position]);
    int diff = (airflowRaw - _rawDataPoint[data_position]);
    float percentage_of_window = ((float)diff / (float)window_size);
    float window_size_mps = (_mpsDataPoint[data_position + 1] - _mpsDataPoint[data_position]);
    
    airflowMps = _mpsDataPoint[data_position] + (window_size_mps * percentage_of_window);
    return airflowMps;
}

float FS3000::readMilesPerHour() {
    return (readMetersPerSecond() * 2.23694);
}

void FS3000::readData(uint8_t* buffer_in) {
    wiringPiI2CWrite(fd, 0x00); // Command to read data, replace with the correct command if necessary
    for (int i = 0; i < 5; i++) {
        buffer_in[i] = wiringPiI2CRead(fd);
    }
}

bool FS3000::checksum(uint8_t* data_in) {
    uint8_t sum = 0;
    for (int i = 1; i <= 4; i++) {
        sum += data_in[i];
    }
    sum %= 256;
    uint8_t calculated_cksum = (~sum + 1);
    return (calculated_cksum == data_in[0]);
}

int main() {
    FS3000 fs;
    if (!fs.begin()) {
        std::cerr << "The sensor did not respond. Please check wiring." << std::endl;
        return -1;
    }

    fs.setRange(7); // Set the range to 7 m/s

    while (true) {
        std::cout << "FS3000 Readings \tRaw: " << fs.readRaw()
                  << "\tm/s: " << fs.readMetersPerSecond()
                  << "\tmph: " << fs.readMilesPerHour() << std::endl;
        delay(1000);
    }

    return 0;
}