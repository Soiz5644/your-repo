import picamera
import datetime

# Set the duration for 4 hours (in seconds)
duration = 4 * 60 * 60

# Set the video resolution and format
resolution = (2592, 1944)  # Full HD resolution
framerate = 30  # Frames per second

# Create a camera object
camera = picamera.PiCamera()

try:
    # Set the camera resolution and framerate
    camera.resolution = resolution
    camera.framerate = framerate

    # Calculate the maximum bitrate to stay within 60GB for 4 hours
    max_bitrate = (60 * 1024 * 1024 * 8) / duration

    # Set the recording parameters
    output_filename = "/media/pi/PHILIPS UFD/video.h264"
    camera.start_recording(output_filename, bitrate=int(max_bitrate), format='mjpeg')

    # Record for the specified duration
    camera.wait_recording(duration)

except KeyboardInterrupt:
    print("Recording stopped by user")

finally:
    # Stop recording and release the camera
    camera.stop_recording()
    camera.close()