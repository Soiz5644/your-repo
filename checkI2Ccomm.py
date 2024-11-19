import smbus
bus = smbus.SMBus(1)
address = 0x28 # Update this if necessary
try:
    bus.write_quick(address)
    print("Sensor is connected properly.")
except OSError as e:
    print(f"Error: {e}")