import pigpio
import time
import fcntl
import array

FS3000_ADDRESS = 0x28
ORIGINAL_BAUDRATE = 100000  # Original baudrate (100kHz)
TEST_BAUDRATE = 400000      # Test baudrate (400kHz)

class FS3000:
    fs3000_1005_air_velocity_table = (0, 1.07, 2.01, 3.0, 3.97, 4.96, 5.98, 6.99, 7.23)
    fs3000_1005_adc_table = (409, 915, 1522, 2066, 2523, 2908, 3256, 3572, 3686)

    def __init__(self, pi, bus=1, device=0x00):
        self.pi = pi
        self.bus = bus
        self.device = device
        self.handle = self.pi.i2c_open(self.bus, FS3000_ADDRESS)
        self.rb = open(f"/dev/i2c-{bus}", "rb", buffering=0)
        self.wb = open(f"/dev/i2c-{bus}", "wb", buffering=0)
        fcntl.ioctl(self.rb, I2C_SLAVE, FS3000_ADDRESS)
        fcntl.ioctl(self.wb, I2C_SLAVE, FS3000_ADDRESS)

    def __del__(self):
        if self.handle is not None:
            self.pi.i2c_close(self.handle)
        self.rb.close()
        self.wb.close()

    def set_baudrate(self, baudrate):
        self.pi.set_baudrate(self.handle, baudrate)

    def is_connected(self):
        try:
            self.set_baudrate(TEST_BAUDRATE)
            self.pi.i2c_read_byte(self.handle)
            self.set_baudrate(ORIGINAL_BAUDRATE)
            return True
        except Exception as e:
            print(f"Connection check failed: {e}")
            return False

    def read_raw(self):
        try:
            self.set_baudrate(TEST_BAUDRATE)
            tmp = self.rb.read(5)
            self.set_baudrate(ORIGINAL_BAUDRATE)
            data = array.array('B', tmp)
            sum = 0x00
            for i in range(0, 5):
                sum += data[i]
            if sum & 0xff != 0x00:
                return 0x00
            else:
                return (data[1] * 256 + data[2]) & 0xfff
        except Exception as e:
            print(f"Read raw data failed: {e}")
            return None

    def read_meters_per_second(self):
        raw = self.read_raw()
        if raw is not None:
            if raw < self.fs3000_1005_adc_table[0] or raw > self.fs3000_1005_adc_table[8]:
                return 0
            for i in range(8):
                if raw > self.fs3000_1005_adc_table[i]:
                    level = i
            percentage = (raw - self.fs3000_1005_adc_table[level]) / (self.fs3000_1005_adc_table[level + 1] - self.fs3000_1005_adc_table[level])
            return (self.fs3000_1005_air_velocity_table[level + 1] - self.fs3000_1005_air_velocity_table[level]) * percentage + self.fs3000_1005_air_velocity_table[level]
        return None

if __name__ == "__main__":
    pi = pigpio.pi()
    if not pi.connected:
        print("Could not connect to pigpio daemon")
        exit()

    fs3000 = FS3000(pi)

    if fs3000.is_connected():
        print("FS3000 is connected")
    else:
        print("FS3000 is not connected")

    try:
        while True:
            speed_mps = fs3000.read_meters_per_second()
            if speed_mps is not None:
                print(f"Speed: {speed_mps:.2f} m/s")
            else:
                print("Failed to read data")
            time.sleep(1)
    except KeyboardInterrupt:
        pass

    pi.stop()