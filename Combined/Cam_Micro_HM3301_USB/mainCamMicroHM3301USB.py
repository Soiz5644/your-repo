import time
import smbus2
import picamera
import subprocess
from datetime import datetime
import os
import csv

# Constants
HM330_I2C_ADDR = 0x40
HM330_INIT = 0x80
HM330_MEM_ADDR = 0x88
RESOLUTION = (1920, 1080)  # Full HD resolution
FRAMERATE = 30  # Frames per second
MAX_BITRATE = 17000000  # 17 Mbps
OUTPUT_DIRECTORY = "/mnt/usb"
AUDIO_FILENAME = "audio.wav"
VIDEO_FILENAME = "video.h264"
FINAL_VIDEO_FILENAME = "final_video.mp4"
AUDIO_RATE = 44100  # Sample rate for audio

# HM3301 sensor class
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
        time.sleep(0.005)
        if self.check_crc(datas):
            data_parsed = self.parse_data(datas)
            return data_parsed[select]

# Function to get free space in megabytes
def get_free_space_mb(folder):
    statvfs = os.statvfs(folder)
    return statvfs.f_frsize * statvfs.f_bavail / 1024 / 1024

# Function to log data to CSV
def log_to_csv(data, filename):
    with open(filename, "a", newline="") as csvfile:
        writer = csv.writer(csvfile)
        writer.writerow(data)

# Audio recording function
def record_audio(output_directory):
    # Define the command and its arguments
    command = [
        "arecord",
        "--device=hw:1,0",
        "--format", "S16_LE",
        "--rate", "44100",
        "-c1",
        f"{output_directory}/{AUDIO_FILENAME}"
    ]

    try:
        print("Recording... Press Ctrl+C to stop.")
        subprocess.run(command, check=True)
    except KeyboardInterrupt:
        print("\nRecording interrupted by user.")
    except subprocess.CalledProcessError as e:
        print(f"Error occurred while recording: {e}")
    except FileNotFoundError:
        print("The 'arecord' utility is not found. Ensure it is installed on your Raspberry Pi.")
    finally:
        print("Exiting script.")

# Function to combine video and audio using ffmpeg
def combine_video_audio(video_file, audio_file, output_file):
    command = [
        'ffmpeg',
        '-i', video_file,
        '-i', audio_file,
        '-c:v', 'copy',
        '-c:a', 'aac',
        '-strict', 'experimental',
        output_file
    ]
    subprocess.run(command)

# Main function
def main():
    # Create necessary directories
    if not os.path.exists(OUTPUT_DIRECTORY):
        os.makedirs(OUTPUT_DIRECTORY)
        os.chmod(OUTPUT_DIRECTORY, 0o777)

    # Create camera object
    camera = picamera.PiCamera()
    camera.resolution = RESOLUTION
    camera.framerate = FRAMERATE

    # Create I2C bus object
    bus = smbus2.SMBus(1)
    sensor = HM3301(i2c=bus)

    # Temporisation de 30 secondes
    time.sleep(30)

    try:
        # Get current time
        creation_time = datetime.now().strftime("%Y-%m-%d_%H-%M-%S")

        # Set output filenames
        video_filename = f"{OUTPUT_DIRECTORY}/{creation_time}_{VIDEO_FILENAME}"
        csv_filename = f"{OUTPUT_DIRECTORY}/sensor_data_{creation_time}.csv"
        
        # Start video recording
        camera.start_recording(video_filename, bitrate=int(MAX_BITRATE))

        # Log sensor data to CSV
        log_to_csv(["Timestamp", "PM1.0", "PM2.5", "PM10"], csv_filename)

        # Record audio in parallel
        audio_filename = f"{OUTPUT_DIRECTORY}/{creation_time}_{AUDIO_FILENAME}"
        record_audio(OUTPUT_DIRECTORY)
        if audio_filename is None:
            print("Audio recording failed.")
            return  # Exit if audio recording failed

        while True:
            camera.wait_recording(1)
            free_space = get_free_space_mb(OUTPUT_DIRECTORY)
            if free_space < 100:
                print("Storage is almost full, stopping recording.")
                break

            # Read sensor data
            std_PM1 = sensor.get_data(0)
            std_PM2_5 = sensor.get_data(1)
            std_PM10 = sensor.get_data(2)

            if std_PM1 is not None and std_PM2_5 is not None and std_PM10 is not None:
                print(f"Concentration des particules :\n - De taille 1 µm : {std_PM1} µg/m^3\n - De taille 2,5 µm : {std_PM2_5} µg/m^3\n - De taille 10 µm : {std_PM10} µg/m^3\n")
                log_to_csv([datetime.now().strftime("%Y-%m-%d %H:%M:%S"), std_PM1, std_PM2_5, std_PM10], csv_filename)
            else:
                print("Données non valides reçues.")

            time.sleep(30)
    except KeyboardInterrupt:
        print("Recording stopped by user")
    finally:
        camera.stop_recording()
        camera.close()
        bus.close()

        # Combine video and audio only if audio_filename is not None
        if audio_filename:
            final_video_filename = f"{OUTPUT_DIRECTORY}/{creation_time}_{FINAL_VIDEO_FILENAME}"
            combine_video_audio(video_filename, audio_filename, final_video_filename)
        else:
            print("Skipping video-audio combination due to missing audio file.")

if __name__ == "__main__":
    main()