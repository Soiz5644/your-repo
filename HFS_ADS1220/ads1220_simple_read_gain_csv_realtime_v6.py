import spidev
import RPi.GPIO as GPIO
import time
import csv
import os

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
    1:   0b000,
    2:   0b001,
    4:   0b010,
    8:   0b011,
    16:  0b100,
    32:  0b101,
    64:  0b110,
    128: 0b111
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
    config1 = 0x04  # DR=20SPS
    config2 = 0x10  # VREF = AVDD
    config3 = 0x00  # IDAC off
    tx = [0x40, 0x03, config0, config1, config2, config3]  # Correct WREG format
    spi.xfer2(tx)

def ads1220_read_registers():
    tx = [0x20, 0x03] + [0x00] * 4  # RREG start=0, len=3 => 4 regs
    result = spi.xfer2(tx)
    return result[2:]  # skip command

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
    GAIN = GAIN_SETTINGS[gain] << 1          # bits 3:1
    MUX = 0b0000 << 4                        # AIN0–AIN1
    PGA_BYPASS = 0x00                        # PGA activé
    config0 = MUX | GAIN | PGA_BYPASS
    
    print("Test SPI: ", spi.xfer2([0x00]))  # Doit renvoyer quelque chose != 0xFF
    print(f"CONFIG0 = 0x{config0:02X} | {config0:08b}")
    spi.xfer2([CMD_RESET])
    time.sleep(0.1)

    ads1220_write_registers(config0)
    regs = ads1220_read_registers()
    print("Registres lus (CONFIG0-3) :", [f"0x{r:02X}" for r in regs])

    print("Lecture différentielle (AIN0–AIN1), DRDY sur GPIO25, Gain =", gain)

    filename = f"ads1220_gain_{gain}.csv"
    file_exists = os.path.exists(filename)

    with open(filename, "a", newline='') as csvfile:
        writer = csv.writer(csvfile)
        if not file_exists:
            writer.writerow(["datetime", "voltage_V", "raw", "gain"])
        print(f"Écriture dans {filename} (Ctrl+C pour arrêter)")
        try:
            while True:
                now = time.strftime("%Y-%m-%d %H:%M:%S")
                raw = ads1220_read_raw()
                voltage = (raw / (2**23)) * VREF
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
