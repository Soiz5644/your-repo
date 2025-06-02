import spidev
import RPi.GPIO as GPIO
import time
import csv

# === Configuration SPI et GPIO ===
SPI_BUS = 0
SPI_DEVICE = 0
DRDY_PIN = 25  # GPIO25, pin physique 22
VREF = 3.3

spi = spidev.SpiDev()
spi.open(SPI_BUS, SPI_DEVICE)
spi.max_speed_hz = 1000000
spi.mode = 1

GPIO.setmode(GPIO.BCM)
GPIO.setup(DRDY_PIN, GPIO.IN)

CMD_RESET = 0x06
CMD_START = 0x08
CMD_RDATA = 0x10

GAIN_SETTINGS = {
    1:   0x00,
    2:   0x02,
    4:   0x04,
    8:   0x06,
    16:  0x08,
    32:  0x0A,
    64:  0x0C,
    128: 0x0E
}

def ask_gain():
    valid_gains = list(GAIN_SETTINGS.keys())
    print("Veuillez choisir un gain parmi :", valid_gains)
    while True:
        try:
            gain = int(input("Gain désiré (ex: 1, 2, 4, ... 128) : "))
            if gain in valid_gains:
                return gain
            else:
                print("Valeur invalide. Choisissez parmi :", valid_gains)
        except Exception:
            print("Entrée invalide. Veuillez saisir un nombre.")

def ads1220_write_registers(config0):
    """
    Écriture complète des registres CONFIG0 à CONFIG3
    CONFIG0 : gain sélectionné, PGA activé
    CONFIG1 : mode normal, DR=20SPS
    CONFIG2 : VREF=AVDD, IDAC off
    CONFIG3 : IDAC routing off
    """
    config1 = 0x04  # DR=20SPS, mode normal
    config2 = 0x10  # VREF = AVDD
    config3 = 0x00  # IDAC off
    tx = [0x43, 0x00, config0, config1, config2, config3]
    spi.xfer2(tx)

def ads1220_wait_drdy():
    while GPIO.input(DRDY_PIN):
        time.sleep(0.001)

def ads1220_read_raw():
    spi.xfer2([CMD_START])
    ads1220_wait_drdy()
    result = spi.xfer2([CMD_RDATA, 0, 0, 0])
    value = (result[1] << 16) | (result[2] << 8) | result[3]
    if value & 0x800000:
        value -= 1 << 24
    return value

def main():
    print("=== ADS1220 : sélection du gain PGA et acquisition CSV ===")
    gain = ask_gain()
    config0 = GAIN_SETTINGS[gain]
    print(f"Configuration du gain PGA à {gain} (CONFIG0=0x{config0:02X})")
    
    spi.xfer2([CMD_RESET])
    time.sleep(0.1)
    ads1220_write_registers(config0)

    print("Lecture différentielle (AIN0–AIN1), DRDY sur GPIO25, Gain =", gain)

    filename = "ads1220_log.csv"
    with open(filename, "w", newline='') as csvfile:
        writer = csv.writer(csvfile)
        writer.writerow(["datetime", "voltage_V", "raw", "gain"])
        print(f"Enregistrement dans {filename} (Ctrl+C pour arrêter)")
        try:
            while True:
                now = time.strftime("%Y-%m-%d %H:%M:%S")
                raw = ads1220_read_raw()
                voltage = (raw / (2**23)) * VREF  # pas de division par gain ici
                writer.writerow([now, f"{voltage:.6f}", raw, gain])
                print(f"{now}\tRaw: {raw}\tVoltage: {voltage:.6f} V")
                csvfile.flush()
                time.sleep(1)
        except KeyboardInterrupt:
            print("\nArrêt de l'acquisition.")
        finally:
            GPIO.cleanup()

if __name__ == '__main__':
    main()
