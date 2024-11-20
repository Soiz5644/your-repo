import pigpio
import time

FS3000_ADDRESS = 0x28
ORIGINAL_BAUDRATE = 100000  # Original baudrate (100kHz)
TEST_BAUDRATE = 400000      # Test baudrate (400kHz)

class FS3000:
    def __init__(self, pi, bus=1):
        self.pi = pi
        self.bus = bus
        self.handle = self.pi.i2c_open(self.bus, FS3000_ADDRESS)

    def __del__(self):
        if self.handle is not None:
            self.pi.i2c_close(self.handle)

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
            count, data = self.pi.i2c_read_device(self.handle, 5)
            self.set_baudrate(ORIGINAL_BAUDRATE)
            if count == 5:
                print(f"Raw data read: {data}")
                checksum = data[0]
                high = data[1]
                low = data[2]
                received_checksum = data[3]
                extra = data[4]

                # Calculate checksum
                calculated_checksum = (256 - ((high + low + received_checksum + extra) & 0xFF)) & 0xFF
                if checksum != calculated_checksum:
                    print(f"Checksum mismatch: expected {checksum}, got {calculated_checksum}")
                    return None

                return (high << 8) | low
            else:
                print(f"Unexpected data length: {count}")
                return None
        except Exception as e:
            print(f"Read raw data failed: {e}")
            return None

    def read_meters_per_second(self):
        raw = self.read_raw()
        if raw is not None:
            return raw * 7.23 / 4095  # Assuming 0-7.23 m/s range
        else:
            return None

    def read_miles_per_hour(self):
        mps = self.read_meters_per_second()
        if mps is not None:
            return mps * 2.2369362912
        else:
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