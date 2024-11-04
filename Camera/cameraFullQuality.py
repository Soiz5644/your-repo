import picamera

# Set the highest video resolution and format supported by the Raspberry Pi camera
resolution = (3280, 2464)  # Maximum resolution for Raspberry Pi Camera Module V2
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
    output_filename = "/media/pi/PHILIPS UFD/video.h264"
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