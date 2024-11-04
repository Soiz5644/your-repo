import time
import grovepi
import atexit

atexit.register(grovepi.dust_sensor_dis)

print("Reading from the dust sensor")
grovepi.dust_sensor_en()
while True:
    try:
  [new_val,lowpulseoccupancy] = grovepi.dustSensorRead()
  if new_val:
   print(lowpulseoccupancy)
  time.sleep(5) 

    except IOError:
        print ("Error")
