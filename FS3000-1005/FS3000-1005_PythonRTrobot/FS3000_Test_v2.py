#!/usr/bin/env python3

# RTrobot FS3000 Sensor Test
# http://rtrobot.org

import RTrobot_FS3000
import sys
import time
import RPi.GPIO as GPIO

print("Initializing FS3000 sensor...")
tmf = RTrobot_FS3000.RTrobot_FS3000()
print("FS3000 sensor initialized.")

try:
    while True:
        print("Reading data...")
        speed = tmf.FS3000_ReadData()
        print(f"Data read: {speed}")
        if speed != 0:
            print(f"{speed} m/s")
        time.sleep(1)  # 1 second delay

except KeyboardInterrupt:
    pass
GPIO.cleanup()
print("Script terminated by user.")