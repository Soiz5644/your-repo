import picamera
from datetime import datetime
import os

def get_free_space_mb(folder):
    """ Returns the free space in megabytes """
    statvfs = os.statvfs(folder)
    return statvfs.f_frsize * statvfs.f_bavail / 1024 / 1024

# Set a lower resolution to comply with H.264 macroblock limits
resolution = (1920, 1080)  # Full HD resolution
framerate = 30  # Frames per second

# Create a camera object
camera = picamera.PiCamera()

try:
    # Set the camera resolution and framerate
    camera.resolution = resolution
    camera.framerate = framerate

    # Set the maximum bitrate for high quality
    max_bitrate = 17000000  # 17 Mbps

    # Get the current date and time
    creation_time = datetime.now().strftime("%Y-%m-%d_%H-%M-%S")

    # Set the recording parameters to save in the /mnt/usb directory
    output_directory = "/mnt/usb/videos"
    if not os.path.exists(output_directory):
        os.makedirs(output_directory)
    output_filename = f"{output_directory}/video_{creation_time}.h264"
    camera.start_recording(output_filename, bitrate=int(max_bitrate))

    # Keep recording until stopped by the user or storage is full
    while True:
        camera.wait_recording(1)
        free_space = get_free_space_mb(output_directory)
        if free_space < 100:  # stop recording if less than 100 MB is available
            print("Storage is almost full, stopping recording.")
            break

except KeyboardInterrupt:
    print("Recording stopped by user")

finally:
    # Stop recording and release the camera
    camera.stop_recording()
    camera.close()

    # Save the creation date and time to a text file
    with open(f"{output_directory}/video_metadata_{creation_time}.txt", "w") as metadata_file:
        metadata_file.write(f"Video creation date and time: {creation_time}")