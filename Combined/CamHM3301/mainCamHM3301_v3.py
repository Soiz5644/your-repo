import time
import smbus2
import picamera
from datetime import datetime
import os
import csv

HM330_I2C_ADDR = 0x40
HM330_INIT = 0x80
HM330_MEM_ADDR = 0x88

class HM3301:
    def __init__(self, i2c, addr=HM330_I2C_ADDR):
        self._i2c = i2c
        self._addr = addr
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
        time.sleep(0.005)  # Sleep for 5 milliseconds
        if self.check_crc(datas):
            data_parsed = self.parse_data(datas)
            measure = data_parsed[select]
            return measure

def get_free_space_mb(folder):
    """ Returns the free space in megabytes """
    statvfs = os.statvfs(folder)
    return statvfs.f_frsize * statvfs.f_bavail / 1024 / 1024

# Function to log data to CSV
def log_to_csv(data, filename):
    with open(filename, "a", newline="") as csvfile:
        writer = csv.writer(csvfile)
        writer.writerow(data)

# Set a lower resolution to comply with H.264 macroblock limits
resolution = (1920, 1080)  # Full HD resolution
framerate = 30  # Frames per second

# Create a camera object
camera = picamera.PiCamera()

# Create an I2C bus object
bus = smbus2.SMBus(1)  # Use 0 for older Raspberry Pi boards

# Instantiate the HM3301 sensor
sensor = HM3301(i2c=bus)

# Temporisation de 30 secondes
time.sleep(30)

try:
    # Set the camera resolution and framerate
    camera.resolution = resolution
    camera.framerate = framerate

    # Set the maximum bitrate for high quality
    max_bitrate = 17000000  # 17 Mbps

    # Get the current date and time
    creation_time = datetime.now().strftime("%Y-%m-%d_%H-%M-%S")

    # Set the recording parameters to save in the /mnt/usb directory
    output_directory = "/mnt/usb"
    if not os.path.exists(output_directory):
        os.makedirs(output_directory)
        os.chmod(output_directory, 0o777)
    output_filename = f"{output_directory}/video_{creation_time}.h264"
    camera.start_recording(output_filename, bitrate=int(max_bitrate))

    # CSV file for sensor data
    csv_filename = f"{output_directory}/sensor_data_{creation_time}.csv"
    log_to_csv(["Timestamp", "PM1.0", "PM2.5", "PM10"], csv_filename)

    while True:
        # Keep recording until stopped by the user or storage is full
        camera.wait_recording(1)
        free_space = get_free_space_mb(output_directory)
        if free_space < 100:  # stop recording if less than 100 MB is available
            print("Storage is almost full, stopping recording.")
            break
        
        # Read sensor data
        std_PM1 = sensor.get_data(0)
        std_PM2_5 = sensor.get_data(1)
        std_PM10 = sensor.get_data(2)

        if std_PM1 is not None and std_PM2_5 is not None and std_PM10 is not None:
            # Display sensor data
            print("Concentration des particules : ")
            print(" - De taille 1 µm : %d µg/m^3" % std_PM1)
            print(" - De taille 2,5 µm : %d µg/m^3" % std_PM2_5)
            print(" - De taille 10 µm : %d µg/m^3\n" % std_PM10)

            # Log sensor data to CSV
            log_to_csv([datetime.now().strftime("%Y-%m-%d %H:%M:%S"), std_PM1, std_PM2_5, std_PM10], csv_filename)
        else:
            print("Données non valides reçues.")

        # Temporisation de 30 secondes
        time.sleep(30)

except KeyboardInterrupt:
    print("Recording stopped by user")

finally:
    # Stop recording and release the camera
    camera.stop_recording()
    camera.close()

    # Save the creation date and time to a text file
    with open(f"{output_directory}/video_metadata_{creation_time}.txt", "w") as metadata_file:
        metadata_file.write(f"Video creation date and time: {creation_time}")

    # Close the I2C bus
    bus.close()