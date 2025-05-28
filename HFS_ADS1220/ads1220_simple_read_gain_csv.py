import spidev
import time
import csv

# SPI configuration
SPI_BUS = 0
SPI_DEVICE = 0
spi = spidev.SpiDev()
spi.open(SPI_BUS, SPI_DEVICE)
spi.max_speed_hz = 1000000
spi.mode = 1  # CPOL=0, CPHA=1

CMD_RESET = 0x06
CMD_START = 0x08
CMD_RDATA = 0x10

VREF = 3.3  # Référence ADS1220

GAIN_SETTINGS = {
    1:   0x08,
    2:   0x0A,
    4:   0x0C,
    8:   0x0E,
    16:  0x10,
    32:  0x12,
    64:  0x14,
    128: 0x16
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
    tx = [0x43, 0x00, config0, 0x04, 0x10, 0x00]
    spi.xfer2(tx)

def ads1220_read_raw():
    spi.xfer2([CMD_START])
    time.sleep(0.06)
    result = spi.xfer2([CMD_RDATA, 0, 0, 0])
    value = (result[1] << 16) | (result[2] << 8) | result[3]
    if value & 0x800000:
        value -= 1 << 24
    return value

def main():
    print("=== ADS1220 : sélection du gain PGA et acquisition CSV ===")
    gain = ask_gain()
    config0 = GAIN_SETTINGS[gain]
    print(f"Configuration du gain PGA à {gain} (registre CONFIG0=0x{config0:02X})")
    spi.xfer2([CMD_RESET])
    time.sleep(0.1)
    ads1220_write_registers(config0)
    print("Lecture en différentiel (AIN0–AIN1), Gain =", gain)

    filename = "ads1220_log.csv"
    start_time = time.time()
    with open(filename, "w", newline='') as csvfile:
        writer = csv.writer(csvfile)
        writer.writerow(["time_s", "voltage_V", "raw", "gain"])
        print(f"Enregistrement des données dans {filename} (Ctrl+C pour arrêter)")
        try:
            while True:
                now = time.time() - start_time
                raw = ads1220_read_raw()
                voltage = (raw / (2**23)) * (VREF / gain)
                writer.writerow([f"{now:.3f}", f"{voltage:.6f}", raw, gain])
                print(f"t={now:.3f}s\tRaw: {raw}\tVoltage: {voltage:.6f} V")
                csvfile.flush()
                time.sleep(0.5)
        except KeyboardInterrupt:
            print("\nArrêt de l'acquisition.")

if __name__ == '__main__':
    main()