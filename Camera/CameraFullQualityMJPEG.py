import picamera
from datetime import datetime

# Set the highest video resolution supported by the OV5647 camera
resolution = (2592, 1944)  # Maximum resolution for OV5647
framerate = 30  # Frames per second

# Create a camera object
camera = picamera.PiCamera()

try:
    # Set the camera resolution and framerate
    camera.resolution = resolution
    camera.framerate = framerate

    # Set the maximum bitrate for high quality
    max_bitrate = 25000000  # 25 Mbps

    # Get the current date and time
    creation_time = datetime.now().strftime("%Y-%m-%d_%H-%M-%S")

    # Set the recording parameters
    output_filename = f"/media/pi/PHILIPS UFD/video_{creation_time}.h264"
    camera.start_recording(output_filename, bitrate=int(max_bitrate))

    # Keep recording until stopped by the user
    while True:
        camera.wait_recording(1)

except KeyboardInterrupt:
    print("Recording stopped by user")

finally:
    # Stop recording and release the camera
    camera.stop_recording()
    camera.close()

    # Save the creation date and time to a text file
    with open(f"/media/pi/PHILIPS UFD/video_metadata_{creation_time}.txt", "w") as metadata_file:
        metadata_file.write(f"Video creation date and time: {creation_time}")