import time
import pigpio
import picamera
from datetime import datetime
import os
import csv
import subprocess
from threading import Thread

# Constants for HM3301 sensor
HM330_I2C_ADDR = 0x40
HM330_INIT = 0x80
HM330_MEM_ADDR = 0x88

class HM3301:
    def __init__(self, pi, sda, scl, addr=HM330_I2C_ADDR):
        self.pi = pi
        self.sda = sda
        self.scl = scl
        self.addr = addr

        # Close any previous instance of bit-banging I2C
        try:
            self.pi.bb_i2c_close(self.sda)
        except pigpio.error as e:
            if str(e) != "'no bit bang I2C in progress on GPIO'":
                raise

        # Set up bit-banging I2C at 20 kHz
        self.pi.bb_i2c_open(self.sda, self.scl, 20000)


        # Initialize the sensor
        self._write([HM330_INIT])

    def _write(self, buffer):
        commands = [4, self.addr, 2, 7, 1] + buffer + [3, 0]
        self.pi.bb_i2c_zip(self.sda, commands)

    def read_data(self):
        commands = [4, self.addr, 2, 7, 1, HM330_MEM_ADDR, 3, 2, 6, 29, 3, 0]
        (count, data) = self.pi.bb_i2c_zip(self.sda, commands)
        if count < 0:
            raise IOError("I2C read error")
        return data

    def check_crc(self, data):
        total_sum = sum(data[:-1]) & 0xFF
        return total_sum == data[28]

    def parse_data(self, data):
        std_PM1 = (data[4] << 8) | data[5]
        std_PM2_5 = (data[6] << 8) | data[7]
        std_PM10 = (data[8] << 8) | data[9]
        return [std_PM1, std_PM2_5, std_PM10]

    def get_data(self):
        data = self.read_data()
        time.sleep(0.005)
        if self.check_crc(data):
            return self.parse_data(data)
        return None

    def close(self):
        try:
            self.pi.bb_i2c_close(self.sda)
        except pigpio.error as e:
            print(f"Error closing I2C: {e}")

def log_to_csv(data, filename):
    with open(filename, "a", newline="") as csvfile:
        writer = csv.writer(csvfile)
        writer.writerow(data)

def record_audio_video(output_file):
    command = [
        "ffmpeg",
        "-f", "alsa",
        "-ac", "1",
        "-i", "hw:1,0",
        "-f", "v4l2",
        "-framerate", "30",
        "-video_size", "1920x1080",
        "-i", "/dev/video0",
        "-c:v", "h264_omx",
        "-b:v", "1700k",
        "-c:a", "aac",
        "-strict", "experimental",
        output_file
    ]
    return subprocess.Popen(command)

def main():
    # Output directory setup
    output_directory = "/mnt/usb"
    if not os.path.exists(output_directory):
        os.makedirs(output_directory, exist_ok=True)
    
    # File names
    creation_time = datetime.now().strftime("%Y-%m-%d_%H-%M-%S")
    output_file = f"{output_directory}/output_{creation_time}.mp4"
    csv_file = f"{output_directory}/sensor_data_{creation_time}.csv"

    # Initialize pigpio and HM3301 sensor
    pi = pigpio.pi()
    if not pi.connected:
        raise RuntimeError("Failed to connect to pigpio daemon")

    sda = 2  # GPIO pin for SDA
    scl = 3  # GPIO pin for SCL
    sensor = HM3301(pi, sda, scl)

    try:
        # Start audio-video recording
        av_process = record_audio_video(output_file)

        log_to_csv(["Timestamp", "PM1.0", "PM2.5", "PM10"], csv_file)

        print("Recording... Press Ctrl+C to stop.")
        while True:
            time.sleep(1)
            data = sensor.get_data()
            if data:
                log_to_csv([datetime.now().strftime("%Y-%m-%d %H:%M:%S"), *data], csv_file)
            else:
                print("Invalid data from sensor.")

    except KeyboardInterrupt:
        print("\nRecording stopped by user.")

    finally:
        # Stop audio-video recording
        if av_process:
            av_process.terminate()

        # Cleanup
        sensor.close()
        pi.stop()
        print(f"Output saved to {output_file}")

if __name__ == "__main__":
    main()
