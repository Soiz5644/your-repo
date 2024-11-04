import time
import smbus2

HM330_I2C_ADDR = 0x40
HM330_INIT = 0x80
HM330_MEM_ADDR = 0x88

class HM3301:

    def __init__(self, i2c, addr=HM330_I2C_ADDR):
        self._i2c = i2c
        self._addr = addr
        self._write([HM330_INIT])

    def read_data(self):
        return self._i2c.read_i2c_block_data(self._addr, HM330_MEM_ADDR, 29)

    def _write(self, buffer):
        self._i2c.write_i2c_block_data(self._addr, 0, buffer)

    def check_crc(self, data):
        total_sum = 0
        for i in range(29 - 1):
            total_sum += data[i]
        total_sum = total_sum & 0xFF
        return total_sum == data[28]

    def parse_data(self, data):
        std_PM1 = (data[4] << 8) | data[5]
        std_PM2_5 = (data[6] << 8) | data[7]
        std_PM10 = (data[8] << 8) | data[9]
        atm_PM1 = (data[10] << 8) | data[11]
        atm_PM2_5 = (data[12] << 8) | data[13]
        atm_PM10 = (data[14] << 8) | data[15]

        return [std_PM1, std_PM2_5, std_PM10, atm_PM1, atm_PM2_5, atm_PM10]

    def get_data(self, select):
        datas = self.read_data()
        time.sleep(0.005)  # Sleep for 5 milliseconds
        if self.check_crc(datas):
            data_parsed = self.parse_data(datas)
            measure = data_parsed[select]
            return measure

# Create an I2C bus object
bus = smbus2.SMBus(1)  # Use 0 for older Raspberry Pi boards

# Instantiate the HM3301 sensor
sensor = HM3301(i2c=bus)

# Temporisation de 30 secondes
time.sleep(30)

try:
    while True:
        # Concentration massique des particules de taille 1 µm
        std_PM1 = sensor.get_data(0)
        # Concentration massique des particules de taille 2.5 µm
        std_PM2_5 = sensor.get_data(1)
        # Concentration massique des particules de taille 10 µm
        std_PM10 = sensor.get_data(2)

        if std_PM1 is not None and std_PM2_5 is not None and std_PM10 is not None:
            # Affichage
            print("Concentration des particules : ")
            print(" - De taille 1 µm : %d µg/m^3" % std_PM1)
            print(" - De taille 2,5 µm : %d µg/m^3" % std_PM2_5)
            print(" - De taille 10 µm : %d µg/m^3\n" % std_PM10)
        else:
            print("Données non valides reçues.")

        # Temporisation de 30 secondes
        time.sleep(30)

except KeyboardInterrupt:
    print("Measurement stopped by user")

finally:
    bus.close()
