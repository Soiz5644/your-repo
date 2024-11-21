#!/usr/bin/env python3

# RTrobot FS3000 Sensor Test
# http://rtrobot.org

from RTrobot_FS3000 import RTrobot_FS3000
import sys
import time
import RPi.GPIO as GPIO

fs = RTrobot_FS3000(device=RTrobot_FS3000.FS3000_1005)

try:
    while True:
        speed = fs.FS3000_ReadData()
        if speed != 0:
            print(str(speed)+" m/s")
        time.sleep(0.1)

except KeyboardInterrupt:
    pass
GPIO.cleanup()
