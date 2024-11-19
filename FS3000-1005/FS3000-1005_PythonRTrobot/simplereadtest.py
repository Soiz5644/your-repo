import smbus
import time

# Initialize I2C bus
bus = smbus.SMBus(1)
address = 0x28

def read_i2c_data():
    try:
        data = bus.read_i2c_block_data(address, 0, 5)
        print(f"Raw data: {data}")
        return data
    except Exception as e:
        print(f"Error reading data: {e}")
        return None

while True:
    read_i2c_data()
    time.sleep(1)