import smbus
import time

# Constants for FS3000
FS3000_DEVICE_ADDRESS = 0x28
AIRFLOW_RANGE_7_MPS = 0x00
AIRFLOW_RANGE_15_MPS = 0x01

class FS3000:
    def __init__(self, bus):
        self.bus = bus
        self.range = AIRFLOW_RANGE_7_MPS
        self.mpsDataPoint = [0, 1.07, 2.01, 3.00, 3.97, 4.96, 5.98, 6.99, 7.23]
        self.rawDataPoint = [409, 915, 1522, 2066, 2523, 2908, 3256, 3572, 3686]

    def is_connected(self):
        try:
            self.bus.write_quick(FS3000_DEVICE_ADDRESS)
            return True
        except:
            return False

    def set_range(self, range):
        self.range = range
        if self.range == AIRFLOW_RANGE_7_MPS:
            self.mpsDataPoint = [0, 1.07, 2.01, 3.00, 3.97, 4.96, 5.98, 6.99, 7.23]
            self.rawDataPoint = [409, 915, 1522, 2066, 2523, 2908, 3256, 3572, 3686]
        elif self.range == AIRFLOW_RANGE_15_MPS:
            self.mpsDataPoint = [0, 2.00, 3.00, 4.00, 5.00, 6.00, 7.00, 8.00, 9.00, 10.00, 11.00, 13.00, 15.00]
            self.rawDataPoint = [409, 1203, 1597, 1908, 2187, 2400, 2629, 2801, 3006, 3178, 3309, 3563, 3686]

    def read_raw(self):
        try:
            data = self.bus.read_i2c_block_data(FS3000_DEVICE_ADDRESS, 0, 5)
            data_high_byte = data[1] & 0x0F
            data_low_byte = data[2]
            airflow_raw = (data_high_byte << 8) | data_low_byte
            return airflow_raw
        except Exception as e:
            print(f"Failed to read raw data: {e}")
            return 0

    def read_meters_per_second(self):
        airflow_raw = self.read_raw()
        if airflow_raw <= 409:
            return 0
        if airflow_raw >= 3686:
            return 7.23 if self.range == AIRFLOW_RANGE_7_MPS else 15.00
        data_points_num = 9 if self.range == AIRFLOW_RANGE_7_MPS else 13
        data_position = 0
        for i in range(data_points_num):
            if airflow_raw > self.rawDataPoint[i]:
                data_position = i
        window_size = self.rawDataPoint[data_position + 1] - self.rawDataPoint[data_position]
        diff = airflow_raw - self.rawDataPoint[data_position]
        percentage_of_window = float(diff) / float(window_size)
        window_size_mps = self.mpsDataPoint[data_position + 1] - self.mpsDataPoint[data_position]
        airflow_mps = self.mpsDataPoint[data_position] + (window_size_mps * percentage_of_window)
        return airflow_mps

    def read_miles_per_hour(self):
        return self.read_meters_per_second() * 2.2369362912

# Initialize I2C bus
bus = smbus.SMBus(1)
fs = FS3000(bus)

if not fs.is_connected():
    print("The sensor did not respond. Please check wiring.")
    exit()

fs.set_range(AIRFLOW_RANGE_7_MPS)
print("Sensor is connected properly.")

while True:
    raw = fs.read_raw()
    mps = fs.read_meters_per_second()
    mph = fs.read_miles_per_hour()
    print(f"FS3000 Readings \tRaw: {raw}\tm/s: {mps}\tmph: {mph}")
    time.sleep(1)