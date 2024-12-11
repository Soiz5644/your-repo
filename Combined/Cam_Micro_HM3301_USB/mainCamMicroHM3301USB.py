import time
import smbus2
import picamera
from datetime import datetime
import os
import csv
import subprocess

# Constants for HM3301 sensor
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

def log_to_csv(data, filename):
    with open(filename, "a", newline="") as csvfile:
        writer = csv.writer(csvfile)
        writer.writerow(data)

def record_audio(audio_file):
    command = [
        "arecord",
        "--device=hw:1,0",
        "--format", "S16_LE",
        "--rate", "44100",
        "-c1",
        audio_file
    ]
    return subprocess.Popen(command)

def combine_audio_video(video_file, audio_file, output_file):
    command = [
        "ffmpeg",
        "-i", video_file,
        "-i", audio_file,
        "-c:v", "copy",
        "-c:a", "aac",
        "-strict", "experimental",
        output_file
    ]
    subprocess.run(command)

def main():
    # Output directory setup
    output_directory = "/mnt/usb"
    if not os.path.exists(output_directory):
        os.makedirs(output_directory, exist_ok=True)
    
    # File names
    creation_time = datetime.now().strftime("%Y-%m-%d_%H-%M-%S")
    video_file = f"{output_directory}/video_{creation_time}.h264"
    audio_file = f"{output_directory}/audio_{creation_time}.wav"
    csv_file = f"{output_directory}/sensor_data_{creation_time}.csv"
    output_file = f"{output_directory}/output_{creation_time}.mp4"

    # Initialize HM3301 sensor
    bus = smbus2.SMBus(1)
    sensor = HM3301(i2c=bus)

    # Initialize camera
    camera = picamera.PiCamera()
    camera.resolution = (1920, 1080)
    camera.framerate = 30
    max_bitrate = 17000000

    try:
        # Start recording
        camera.start_recording(video_file, bitrate=max_bitrate)
        log_to_csv(["Timestamp", "PM1.0", "PM2.5", "PM10"], csv_file)

        # Start audio recording
        audio_process = record_audio(audio_file)

        print("Recording... Press Ctrl+C to stop.")
        while True:
            camera.wait_recording(1)
            data = sensor.get_data()
            if data:
                log_to_csv([datetime.now().strftime("%Y-%m-%d %H:%M:%S"), *data], csv_file)
            else:
                print("Invalid data from sensor.")
            time.sleep(1)

    except KeyboardInterrupt:
        print("\nRecording stopped by user.")

    finally:
        # Stop camera recording
        camera.stop_recording()
        camera.close()

        # Stop audio recording
        if audio_process:
            audio_process.terminate()

        # Combine video and audio
        print("Combining audio and video...")
        combine_audio_video(video_file, audio_file, output_file)

        # Cleanup
        bus.close()
        print(f"Output saved to {output_file}")

if __name__ == "__main__":
    main()
