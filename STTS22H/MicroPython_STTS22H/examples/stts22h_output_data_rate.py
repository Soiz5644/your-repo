# SPDX-FileCopyrightText: Copyright (c) 2023 Jose D. Montoya
#
# SPDX-License-Identifier: MIT

import time
from machine import Pin, I2C
from micropython_stts22h import stts22h

i2c = I2C(1, sda=Pin(2), scl=Pin(3))  # Correct I2C pins for RP2040
stts = stts22h.STTS22H(i2c)

stts.output_data_rate = stts22h.ODR_200_HZ

while True:
    for output_data_rate in stts22h.output_data_rate_values:
        print("Current Output data rate setting: ", stts.output_data_rate)
        for _ in range(10):
            print(f"Temperature: {stts.temperature:.1f}°C")
            time.sleep(0.5)
        stts.output_data_rate = output_data_rate
