import picamera

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

    # Set the recording parameters
    output_filename = "/media/pi/PHILIPS UFD/video.mjpeg"
    camera.start_recording(output_filename, bitrate=int(max_bitrate), format='mjpeg')

    # Keep recording until stopped by the user
    while True:
        camera.wait_recording(1)

except KeyboardInterrupt:
    print("Recording stopped by user")

finally:
    # Stop recording and release the camera
    camera.stop_recording()
    camera.close()