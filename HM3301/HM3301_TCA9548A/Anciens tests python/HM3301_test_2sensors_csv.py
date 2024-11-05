import time
import smbus2
import csv

HM330_I2C_ADDR = 0x40  # Adresse I2C du capteur HM3301
HM330_INIT = 0x80
HM330_MEM_ADDR = 0x88
TCA9548A_ADDR = 0x70  # Adresse par défaut du TCA9548A

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
        time.sleep(0.005)
        if self.check_crc(datas):
            data_parsed = self.parse_data(datas)
            return data_parsed[select]
        return None

# Fonction pour sélectionner un canal sur le TCA9548A
def select_tca9548a_channel(bus, channel):
    if channel < 0 or channel > 7:
        raise ValueError("Invalid channel: must be between 0 and 7")
    bus.write_byte(TCA9548A_ADDR, 1 << channel)  # Sélection du canal (1 << channel active le bit correspondant)

# Fonction pour enregistrer les données dans un fichier CSV
def log_to_csv(data):
    with open("sensor_data.csv", "a", newline="") as csvfile:
        writer = csv.writer(csvfile)
        writer.writerow(data)

# Création d'un objet bus I2C
bus = smbus2.SMBus(1)

# Instanciation des capteurs sur le TCA9548A, canaux 0 et 1
sensor1 = HM3301(i2c=bus, addr=HM330_I2C_ADDR)  # Capteur sur le canal 5
sensor2 = HM3301(i2c=bus, addr=HM330_I2C_ADDR)  # Capteur sur le canal 7

# Temporisation pour l'initialisation des capteurs
time.sleep(30)

try:
    # En-tête du CSV
    log_to_csv(["Timestamp", "Sensor", "PM1.0", "PM2.5", "PM10"])

    while True:
        for sensor, channel, sensor_name in [(sensor1, 5, "Sensor 1"), (sensor2, 7, "Sensor 2")]:
            # Sélectionner le canal sur le multiplexeur
            select_tca9548a_channel(bus, channel)

            # Récupération des données
            std_PM1 = sensor.get_data(0)
            std_PM2_5 = sensor.get_data(1)
            std_PM10 = sensor.get_data(2)

            if std_PM1 is not None and std_PM2_5 is not None and std_PM10 is not None:
                # Affichage des données
                print(f"Concentration pour {sensor_name}: ")
                print(f" - PM1.0 : {std_PM1} µg/m^3")
                print(f" - PM2.5 : {std_PM2_5} µg/m^3")
                print(f" - PM10  : {std_PM10} µg/m^3\n")

                # Enregistrement des données dans un fichier CSV
                log_to_csv([time.strftime("%Y-%m-%d %H:%M:%S"), sensor_name, std_PM1, std_PM2_5, std_PM10])
            else:
                print(f"Données non valides reçues de {sensor_name}")

        # Temporisation de 30 secondes avant la prochaine lecture
        time.sleep(30)

except KeyboardInterrupt:
    print("Mesure interrompue par l'utilisateur")

finally:
    bus.close()
