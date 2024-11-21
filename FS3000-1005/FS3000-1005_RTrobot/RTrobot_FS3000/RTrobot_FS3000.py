#!/usr/bin/env python3

# RTrobot FS3000 Test
# http://rtrobot.org

import fcntl
import array
import RPi.GPIO as GPIO
import numpy as np

I2C_SLAVE = 0x0703


class RTrobot_FS3000:
    FS3000_ADDRESS = (0x28)
    FS3000_1005 = 0x00
    FS3000_1015 = 0x01

    fs3000_1015_air_velocity_table = (0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0, 13.0, 15.0)
    fs3000_1015_adc_table = (409, 1203, 1597, 1908, 2187, 2400, 2629, 2801, 3006, 3178, 3309, 3563, 3686)
    fs3000_1005_air_velocity_table = (0, 1.07, 2.01, 3.0, 3.97, 4.96, 5.98, 6.99, 7.23)
    fs3000_1005_adc_table = (409, 915, 1522, 2066, 2523, 2908, 3256, 3572, 3686, 3178)
    

    def __init__(self, i2c_no=1, i2c_addr=FS3000_ADDRESS, device=FS3000_1005):
        global FS3000_rb, FS3000_wb
        global calibration
        FS3000_rb = open("/dev/i2c-"+str(i2c_no), "rb", buffering=0)
        FS3000_wb = open("/dev/i2c-"+str(i2c_no), "wb", buffering=0)
        print("Thanks for using RTrobot module")
        fcntl.ioctl(FS3000_rb, I2C_SLAVE, i2c_addr)
        fcntl.ioctl(FS3000_wb, I2C_SLAVE, i2c_addr)
        self.this_device = device

    def FS3000_ReadData(self):
        fm_raw = self.FS3000_i2c_read()
        fm_level = 0
        fm_percentage = 0
        if self.this_device == self.FS3000_1005:
            if fm_raw < self.fs3000_1005_adc_table[0] or fm_raw > self.fs3000_1005_adc_table[8]:
                return 0
            for i in range(8):
                if fm_raw > self.fs3000_1005_adc_table[i]:
                    fm_level=i
            fm_percentage = (fm_raw - self.fs3000_1005_adc_table[fm_level]) / (self.fs3000_1005_adc_table[fm_level + 1] - self.fs3000_1005_adc_table[fm_level])
            return (self.fs3000_1005_air_velocity_table[fm_level + 1] - self.fs3000_1005_air_velocity_table[fm_level]) * fm_percentage + self.fs3000_1005_air_velocity_table[fm_level]
        else:
            if fm_raw < self.fs3000_1015_adc_table[0] or fm_raw > self.fs3000_1015_adc_table[12]:
                return 0
            for i in range(13):
                if fm_raw > self.fs3000_1015_adc_table[i]:
                    fm_level=i
            fm_percentage = (fm_raw - self.fs3000_1015_adc_table[fm_level]) / (self.fs3000_1015_adc_table[fm_level + 1] - self.fs3000_1015_adc_table[fm_level])
            return (self.fs3000_1015_air_velocity_table[fm_level + 1] - self.fs3000_1015_air_velocity_table[fm_level]) * fm_percentage + self.fs3000_1015_air_velocity_table[fm_level]

    ######################################i2C######################################

    def FS3000_i2c_read(self, delays=0.015):
        tmp = FS3000_rb.read(5)
        data = array.array('B', tmp)
        sum = 0x00
        for i in range(0, 5):
            sum += data[i]
        if sum & 0xff != 0x00:
            return 0x00
        else:
            return (data[1] * 256 + data[2]) & 0xfff
