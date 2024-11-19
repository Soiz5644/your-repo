#include <wiringPiI2C.h>
#include <iostream>
#include <unistd.h>
#include <vector> // Add this line

// Constants for FS3000
#define FS3000_DEVICE_ADDRESS 0x28
#define AIRFLOW_RANGE_7_MPS 0x00
#define AIRFLOW_RANGE_15_MPS 0x01

class FS3000 {
public:
    FS3000(int i2c_fd) : fd(i2c_fd), range(AIRFLOW_RANGE_7_MPS) {
        mpsDataPoint = {0, 1.07, 2.01, 3.00, 3.97, 4.96, 5.98, 6.99, 7.23};
        rawDataPoint = {409, 915, 1522, 2066, 2523, 2908, 3256, 3572, 3686};
    }

    bool isConnected() {
        return wiringPiI2CWrite(fd, 0) >= 0;
    }

    void setRange(uint8_t range) {
        this->range = range;
        if (range == AIRFLOW_RANGE_7_MPS) {
            mpsDataPoint = {0, 1.07, 2.01, 3.00, 3.97, 4.96, 5.98, 6.99, 7.23};
            rawDataPoint = {409, 915, 1522, 2066, 2523, 2908, 3256, 3572, 3686};
        } else if (range == AIRFLOW_RANGE_15_MPS) {
            mpsDataPoint = {0, 2.00, 3.00, 4.00, 5.00, 6.00, 7.00, 8.00, 9.00, 10.00, 11.00, 13.00, 15.00};
            rawDataPoint = {409, 1203, 1597, 1908, 2187, 2400, 2629, 2801, 3006, 3178, 3309, 3563, 3686};
        }
    }

    uint16_t readRaw() {
        int data[5];
        for (int i = 0; i < 5; ++i) {
            data[i] = wiringPiI2CReadReg8(fd, i);
        }
        uint16_t airflowRaw = ((data[1] & 0x0F) << 8) | data[2];
        return airflowRaw;
    }

    float readMetersPerSecond() {
        uint16_t airflowRaw = readRaw();
        if (airflowRaw <= 409) return 0;
        if (airflowRaw >= 3686) return range == AIRFLOW_RANGE_7_MPS ? 7.23 : 15.00;
        int dataPointsNum = range == AIRFLOW_RANGE_7_MPS ? 9 : 13;
        int dataPosition = 0;
        for (int i = 0; i < dataPointsNum; ++i) {
            if (airflowRaw > rawDataPoint[i]) dataPosition = i;
        }
        float percentageOfWindow = float(airflowRaw - rawDataPoint[dataPosition]) / float(rawDataPoint[dataPosition + 1] - rawDataPoint[dataPosition]);
        return mpsDataPoint[dataPosition] + (mpsDataPoint[dataPosition + 1] - mpsDataPoint[dataPosition]) * percentageOfWindow;
    }

    float readMilesPerHour() {
        return readMetersPerSecond() * 2.2369362912;
    }

private:
    int fd;
    uint8_t range;
    std::vector<float> mpsDataPoint;
    std::vector<int> rawDataPoint;
};

int main() {
    int fd = wiringPiI2CSetup(FS3000_DEVICE_ADDRESS);
    if (fd < 0) {
        std::cerr << "Failed to initialize I2C communication.\n";
        return -1;
    }

    FS3000 fs3000(fd);
    if (!fs3000.isConnected()) {
        std::cerr << "The sensor did not respond. Please check wiring.\n";
        return -1;
    }

    fs3000.setRange(AIRFLOW_RANGE_7_MPS);
    std::cout << "Sensor is connected properly.\n";

    while (true) {
        uint16_t raw = fs3000.readRaw();
        float mps = fs3000.readMetersPerSecond();
        float mph = fs3000.readMilesPerHour();
        std::cout << "FS3000 Readings \tRaw: " << raw << "\tm/s: " << mps << "\tmph: " << mph << "\n";
        sleep(1);
    }

    return 0;
}