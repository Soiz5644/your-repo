import time
import sys

sys.path.append('/home/pi/pi-hats-master')

from ads1115 import ADS1115

adc = ADS1115()
adc.set_gain(1)

while True:
    value = adc.read(0)
    voltage = value * 4.096 / 32767

    print(f"Raw: {value} | Voltage: {voltage:.3f} V")

    time.sleep(1)