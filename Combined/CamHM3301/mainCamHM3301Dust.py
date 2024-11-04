import picamera
from datetime import datetime
import os
import time
import smbus2
import csv
import grovepi
import atexit

# Camera related constants and functions
def get_free_space_mb(folder):
    """ Returns the free space in megabytes """
    statvfs = os.statvfs(folder)
    return statvfs.f_frsize * statvfs.f_bavail / 1024 / 1024

resolution = (1920, 1080)  # Full HD resolution
framerate = 30  # Frames per second
max_bitrate = 17000000  # 17 Mbps
output_folder = "/media/pi/PHILIPS UFD"

# HM3301 related constants and functions
HM330_I2C_ADDR = 0x40
HM330_INIT = 0x80
HM330_MEM_ADDR = 0x88
TCA9548A_ADDR = 0x70

class HM3301:
    def __init__(self, i2c, addr=HM330_I2C_ADDR):
        self._i2c = i2c
        self._addr = addr
        time.sleep(1)  # Add delay before writing to the sensor
        self._write([HM330_INIT])

    def read_data(self):
        return self._i2c.read_i2c_block_data(self._addr, HM330_MEM_ADDR, 29)

    def _write(self, buffer):
        self._i2c.write_i2c_block_data(self._addr, 0, buffer)

    def check_crc(self, data):
        total_sum = 0
        for i in range(29 - 1):
            total_sum += data[i]
        total_sum = total_sum & 0xFF
        return total_sum == data[28]

    def parse_data(self, data):
        std_PM1 = (data[4] << 8) | data[5]
        std_PM2_5 = (data[6] << 8) | data[7]
        std_PM10 = (data[8] << 8) | data[9]
        atm_PM1 = (data[10] << 8) | data[11]
        atm_PM2_5 = (data[12] << 8) | data[13]
        atm_PM10 = (data[14] << 8) | data[15]

        return [std_PM1, std_PM2_5, std_PM10, atm_PM1, atm_PM2_5, atm_PM10]

    def get_data(self, select):
        datas = self.read_data()
        time.sleep(0.005)
        if self.check_crc(datas):
            data_parsed = self.parse_data(datas)
            return data_parsed[select]
        return None

def select_tca9548a_channel(bus, channel):
    if channel < 0 or channel > 7:
        raise ValueError("Invalid channel: must be between 0 and 7")
    bus.write_byte(TCA9548A_ADDR, 1 << channel)
    print(f"Channel {channel} selected on TCA9548A")

def log_to_csv(data, csv_file_path):
    with open(csv_file_path, "a", newline="") as csvfile:
        writer = csv.writer(csvfile)
        writer.writerow(data)

# Create I2C bus object and sensor instances
bus = smbus2.SMBus(1)

# Initialize camera
camera = picamera.PiCamera()

# Initialize Dust sensor
atexit.register(grovepi.dust_sensor_dis)
grovepi.dust_sensor_en()

try:
    # Camera setup
    camera.resolution = resolution
    camera.framerate = framerate
    creation_time = datetime.now().strftime("%Y-%m-%d_%H-%M-%S")
    output_filename = f"{output_folder}/video_{creation_time}.h264"
    csv_file_path = f"{output_folder}/sensor_data_{creation_time}.csv"
    camera.start_recording(output_filename, bitrate=int(max_bitrate))

    # Initialize sensors and log CSV header
    time.sleep(30)  # Delay for sensor initialization
    log_to_csv(["Timestamp", "Sensor", "PM1.0", "PM2.5", "PM10"], csv_file_path)

    while True:
        # Camera recording loop
        camera.wait_recording(1)
        free_space = get_free_space_mb(output_folder)
        if free_space < 100:
            print("Storage is almost full, stopping recording.")
            break

        # Sensor data reading and logging loop
        for channel, sensor_name in [(5, "Sensor 1"), (7, "Sensor 2")]:
            select_tca9548a_channel(bus, channel)
            time.sleep(0.1)  # Give some time for the channel selection to take effect
            sensor = HM3301(i2c=bus, addr=HM330_I2C_ADDR)
            std_PM1 = sensor.get_data(0)
            std_PM2_5 = sensor.get_data(1)
            std_PM10 = sensor.get_data(2)

            if std_PM1 is not None and std_PM2_5 is not None and std_PM10 is not None:
                print(f"Concentration for {sensor_name}: ")
                print(f" - PM1.0 : {std_PM1} µg/m^3")
                print(f" - PM2.5 : {std_PM2_5} µg/m^3")
                print(f" - PM10  : {std_PM10} µg/m^3\n")
                log_to_csv([time.strftime("%Y-%m-%d %H:%M:%S"), sensor_name, std_PM1, std_PM2_5, std_PM10], csv_file_path)
            else:
                print(f"Invalid data received from {sensor_name}")

        # Read Dust sensor data
        try:
            new_val = grovepi.dust_sensor_read()
            if new_val:
                timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
                log_to_csv([timestamp, "Dust Sensor", *new_val], csv_file_path)
                print(f"Dust Sensor -> LPO time = {new_val[0]}, LPO% = {new_val[1]:.2f}, pcs/0.01cf = {new_val[2]:.1f}")
        except IOError:
            print("Error reading from Dust Sensor")

        time.sleep(30)  # Delay before next sensor reading

except KeyboardInterrupt:
    print("Recording stopped by user")

finally:
    camera.stop_recording()
    camera.close()
    bus.close()
    with open(f"{output_folder}/video_metadata_{creation_time}.txt", "w") as metadata_file:
        metadata_file.write(f"Video creation date and time: {creation_time}")