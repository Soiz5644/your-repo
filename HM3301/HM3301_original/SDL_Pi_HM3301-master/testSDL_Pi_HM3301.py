import SDL_Pi_HM3301
import pigpio
import time  # Add this import

# Set up pigpio
mypi = pigpio.pi()

# Set the I2C pins
mySDA = 20
mySCL = 19

# Initialize the sensor with the correct parameters
hm3301 = SDL_Pi_HM3301.SDL_Pi_HM3301(mypi, mySDA, mySCL)

try:
    while True:
        data = hm3301.get_data()
        if data:
            hm3301.parse_data(data)
        time.sleep(1)
except KeyboardInterrupt:
    hm3301.close()
    print("closing hm3301")