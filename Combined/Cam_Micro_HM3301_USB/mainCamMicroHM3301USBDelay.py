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
    
def start_camera(camera, video_file, bitrate):
    camera.start_recording(video_file, bitrate=bitrate)
    
def encapsulate_h264_to_mp4(input_h264, output_mp4):
    command = [
        "ffmpeg",
        "-fflags", "+genpts",  # Générer les timestamps manquants pour la vidéo
        "-probesize", "50M",   # Augmenter la taille du buffer pour analyser plus de frames
        "-analyzeduration", "50M",  # Augmenter la durée d'analyse
        "-i", input_h264,      # Vidéo H.264 en entrée
        "-c:v", "copy",        # Copier le flux vidéo sans ré-encoder
        "-f", "mp4",           # Forcer le format MP4
        output_mp4             # Vidéo encapsulée en MP4
    ]
    subprocess.run(command, check=True)
    
def combine_audio_video_with_fixed_video(video_file, audio_file, output_file):
    command = [
        "ffmpeg",
        "-i", video_file,  # Fichier vidéo encapsulé
        "-i", audio_file,  # Fichier audio
        "-c:v", "copy",    # Copier le codec vidéo sans ré-encoder
        "-c:a", "aac",     # Encoder l'audio en AAC
        "-shortest",       # Ajuster pour la durée la plus courte entre vidéo et audio
        output_file        # Fichier de sortie
    ]
    subprocess.run(command)

def main():
    # Output directory setup
    output_directory = "/mnt/usb"
    if not os.path.exists(output_directory):
        os.makedirs(output_directory, exist_ok=True)

    # File names
    creation_time = datetime.now().strftime("%Y-%m-%d_%H-%M-%S")
    raw_video_file = f"{output_directory}/video_{creation_time}.h264"
    audio_file = f"{output_directory}/audio_{creation_time}.wav"
    csv_file = f"{output_directory}/sensor_data_{creation_time}.csv"
    encapsulated_video_file = f"{output_directory}/video_encapsulated_{creation_time}.mp4"
    output_file = f"{output_directory}/output_{creation_time}.mp4"
    
    # Initialize pigpio and HM3301 sensor
    pi = pigpio.pi()
    if not pi.connected:
        raise RuntimeError("Failed to connect to pigpio daemon")

    sda = 2  # GPIO pin for SDA
    scl = 3  # GPIO pin for SCL
    sensor = HM3301(pi, sda, scl)

    # Initialize camera
    camera = picamera.PiCamera()
    camera.resolution = (1920, 1080)
    camera.framerate = 30
    max_bitrate = 17000000

    try:
        # Start camera recording in a separate thread
        video_thread = Thread(target=start_camera, args=(camera, raw_video_file, max_bitrate))
        video_thread.start()
        
        # Encapsulation de la vidéo
        print("Encapsulation de la vidéo H.264...")
        encapsulate_h264_to_mp4(raw_video_file, encapsulated_video_file)
        if not os.path.exists(encapsulated_video_file):
            print("Erreur : Le fichier vidéo encapsulé n'a pas été créé.")
            return 
            
        # Start audio recording
        audio_process = record_audio(audio_file)

        # Ensure the video thread has started
        video_thread.join()

        log_to_csv(["Timestamp", "PM1.0", "PM2.5", "PM10"], csv_file)

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
            audio_process.wait()

        # Combine video and audio
        print("Fusion de l'audio et de la vidéo encapsulée...")
        combine_audio_video_with_fixed_video(encapsulated_video_file, audio_file, output_file)
        if not os.path.exists(output_file):
            print("Erreur : Le fichier final n'a pas été créé.")
            return

        # Cleanup
        sensor.close()
        pi.stop()
        print(f"Output saved to {output_file}")

if __name__ == "__main__":
    main()
