import pigpio
import time

FS3000_ADDRESS = 0x28

class FS3000:
    def __init__(self, pi, bus=1):
        self.pi = pi
        self.handle = self.pi.i2c_open(bus, FS3000_ADDRESS)
        self.range = 0x00

    def __del__(self):
        self.pi.i2c_close(self.handle)

    def is_connected(self):
        try:
            self.pi.i2c_read_byte(self.handle)
            return True
        except:
            return False

    def read_raw(self):
        count, data = self.pi.i2c_read_i2c_block_data(self.handle, 0, 5)
        if count == 5:
            return (data[1] << 8) | data[2]
        else:
            return None

    def read_meters_per_second(self):
        raw = self.read_raw()
        if raw:
            return raw * 7.23 / 4095  # Assuming 0-7.23 m/s range
        else:
            return None

    def read_miles_per_hour(self):
        mps = self.read_meters_per_second()
        if mps:
            return mps * 2.2369362912
        else:
            return None

if __name__ == "__main__":
    pi = pigpio.pi()
    if not pi.connected:
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