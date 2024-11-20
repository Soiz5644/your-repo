#!/usr/bin/env python3

import fcntl
print("fcntl is working")
import array
a = array.array('i', [1, 2, 3])
print("array is working:", a)
import RPi.GPIO as GPIO
GPIO.setmode(GPIO.BCM)
print("RPi.GPIO is working")
GPIO.cleanup()
import numpy as np
a = np.array([1, 2, 3])
print("numpy is working:", a)

I2C_SLAVE = 0x0703

class RTrobot_FS3000:
    FS3000_ADDRESS = (0x28)

    def __init__(self, i2c_no=1, i2c_addr=FS3000_ADDRESS):
        global FS3000_rb, FS3000_wb
        global calibration
        FS3000_rb = open("/dev/i2c-"+str(i2c_no), "rb", buffering=0)
        FS3000_wb = open("/dev/i2c-"+str(i2c_no), "wb", buffering=0)
        print("Thanks for using RTrobot module")
        fcntl.ioctl(FS3000_rb, I2C_SLAVE, i2c_addr)
        fcntl.ioctl(FS3000_wb, I2C_SLAVE, i2c_addr)
        print("I2C interface initialized successfully")

    def FS3000_ReadData(self):
        air_velocity_table = (0, 1.07, 2.01, 3.00, 3.97, 4.96, 5.98, 6.99, 7.23)
        adc_table = (409, 915, 1522, 2066, 2523, 2908, 3256, 3572, 3686)
        fm_raw = self.FS3000_i2c_read()
        print(f"Raw data read: {fm_raw}")
        fm_level = 0
        fm_percentage = 0
        if fm_raw < adc_table[0] or fm_raw > adc_table[8]:
            return 0
        for i in range(8):
            if fm_raw > adc_table[i]:
                fm_level = i
        fm_percentage = (fm_raw - adc_table[fm_level]) / (adc_table[fm_level + 1] - adc_table[fm_level])
        return (air_velocity_table[fm_level + 1] - air_velocity_table[fm_level]) * fm_percentage + air_velocity_table[fm_level]

    def FS3000_i2c_read(self, delays=0.015):
        tmp = FS3000_rb.read(5)
        print(f"Raw I2C data: {list(tmp)}")  # Print raw data bytes
        data = array.array('B', tmp)

        # Calculate 256-modulo sum of the last 4 bytes
        sum = 0
        for i in range(1, 5):
            sum += data[i]
        sum = sum & 0xFF  # Ensure it's 8-bit

        # Calculate 2's complement (negative) of the sum
        calculated_checksum = ((256 - sum) & 0xFF)
        print(f"Calculated checksum: {calculated_checksum}, Received checksum: {data[0]}")

        if calculated_checksum != data[0]:
            print("Checksum error in data read")
            return 0x00
        else:
            return (data[1] * 256 + data[2]) & 0xFFF

# Ensure GPIO cleanup is at the end of the program
GPIO.cleanup()
print("GPIO cleanup completed")
print("Script terminated by user.")