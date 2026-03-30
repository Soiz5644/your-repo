# Patch propre pour communication ADS1220 avec registres correctement 
# configurés via SPI sur Raspberry Pi, conforme aux recommandations TI

import spidev
import RPi.GPIO as GPIO
import time
import csv
import os
from datetime import datetime

# --- Configuration des broches ---
CS_PIN = 8         # Chip Select (GPIO8/CE0)
DRDY_PIN = 17      # Data Ready (GPIO17)
SPI_BUS = 0
SPI_DEVICE = 0
SPI_SPEED_HZ = 500000  # Augmentation à 500 kHz pour fiabilité

# --- Commandes de l'ADS1220 ---
CMD_RESET = 0x06
CMD_START = 0x08
CMD_POWERDOWN = 0x02
CMD_RDATA = 0x10
CMD_WREG = 0x40
CMD_RREG = 0x20

REG_CONF0 = 0x00
REG_CONF1 = 0x01
REG_CONF2 = 0x02
REG_CONF3 = 0x03

VREF = 2.048  # Référence interne
FSR = (2**23)
CSV_FILENAME = "ads1220_data.csv"

def ask_gain():
    print("Choisissez le gain pour l'ADS1220 :")
    print(" 1 : Gain = 1\n 2 : Gain = 2\n 4 : Gain = 4\n 8 : Gain = 8")
    print("16 : Gain = 16\n32 : Gain = 32\n64 : Gain = 64\n128: Gain = 128")
    valid_gains = [1, 2, 4, 8, 16, 32, 64, 128]
    while True:
        try:
            g = int(input("Entrer la valeur du gain souhaité : "))
            if g in valid_gains:
                return g
        except Exception:
            pass
        print("Entrée non valide. Essayez encore.")

def wait_drdy():
    while GPIO.input(DRDY_PIN):
        time.sleep(0.001)

def spi_cs_low():
    GPIO.output(CS_PIN, GPIO.LOW)
    time.sleep(0.001)

def spi_cs_high():
    time.sleep(0.001)
    GPIO.output(CS_PIN, GPIO.HIGH)

def spi_xfer_with_cs(spi, data):
    spi_cs_low()
    try:
        return spi.xfer2(data)
    finally:
        spi_cs_high()

def ads1220_write_all_regs(spi, reg0, reg1, reg2, reg3):
    cmd = [CMD_WREG | (REG_CONF0 << 2), 0x03, reg0, reg1, reg2, reg3]
    print(f"[SPI DEBUG] Write all regs: {cmd}")
    spi_xfer_with_cs(spi, cmd)

def ads1220_read_all_regs(spi):
    spi_xfer_with_cs(spi, [CMD_RREG | (REG_CONF0 << 2), 0x03])
    return spi_xfer_with_cs(spi, [0x00]*4)

def ads1220_read_data(spi):
    resp = spi_xfer_with_cs(spi, [CMD_RDATA, 0x00, 0x00, 0x00])
    raw = (resp[1] << 16) | (resp[2] << 8) | resp[3]
    return raw - (1 << 24) if raw & 0x800000 else raw

def ads1220_init(spi, gain):
    print("[DEBUG] Envoi de la commande RESET...")
    spi_xfer_with_cs(spi, [CMD_RESET])
    time.sleep(0.5)
    spi_xfer_with_cs(spi, [CMD_RDATA])
    time.sleep(0.01)

    gain_map = {1: 0b000, 2: 0b001, 4: 0b010, 8: 0b011,
                16: 0b100, 32: 0b101, 64: 0b110, 128: 0b111}
    reg0 = (0b0000 << 4) | (0 << 3) | gain_map[gain]  # MUX = AIN0-AIN1, PGA on, GAIN
    reg1 = 0x00  # 20SPS, single-shot
    reg2 = 0x10  # VREF int, 50/60Hz rejection
    reg3 = 0x00  # IDACs off

    print(f"[DEBUG] reg0 calculé : 0x{reg0:02X}")
    print("[DEBUG] Écriture des registres de configuration...")
    ads1220_write_all_regs(spi, reg0, reg1, reg2, reg3)
    time.sleep(0.01)

    val = ads1220_read_all_regs(spi)
    for i, v in enumerate(val):
        print(f"[DEBUG] REG{i}: 0x{v:02X} (binaire: {v:08b})")

def raw_to_voltage(raw):
    return (raw / float(FSR)) * VREF

def append_to_csv(filename, row):
    file_exists = os.path.isfile(filename)
    with open(filename, 'a', newline='') as csvfile:
        writer = csv.writer(csvfile, delimiter=';')
        if not file_exists:
            writer.writerow(['Horodatage', 'Valeur_brute', 'Tension_diff_V', 'Gain'])
        writer.writerow(row)

def get_timestamp():
    return datetime.now().strftime("%d/%m/%y - %H:%M:%S")

def main():
    print("[DEBUG] Initialisation GPIO")
    GPIO.setmode(GPIO.BCM)
    GPIO.setup(DRDY_PIN, GPIO.IN)
    GPIO.setup(CS_PIN, GPIO.OUT)
    GPIO.output(CS_PIN, GPIO.HIGH)

    gain = ask_gain()

    print("[DEBUG] Initialisation SPI")
    spi = spidev.SpiDev()
    spi.open(SPI_BUS, SPI_DEVICE)
    spi.max_speed_hz = SPI_SPEED_HZ
    spi.mode = 1
    spi.no_cs = True

    print("[DEBUG] Initialisation de l'ADS1220...")
    ads1220_init(spi, gain)
    print("[DEBUG] Initialisation terminée, début des mesures...\n")

    try:
        while True:
            print("[DEBUG] Démarrage d'une conversion...")
            spi_xfer_with_cs(spi, [CMD_START])
            wait_drdy()
            value_raw = ads1220_read_data(spi)
            value_volts = raw_to_voltage(value_raw)
            timestamp = get_timestamp()
            print(f"{timestamp} ; {value_raw} ; {value_volts:.6f} ; {gain}")
            append_to_csv(CSV_FILENAME, [timestamp, value_raw, f"{value_volts:.6f}", gain])
            time.sleep(1)
    finally:
        print("[DEBUG] Fermeture SPI et nettoyage GPIO")
        GPIO.output(CS_PIN, GPIO.HIGH)
        spi.close()
        GPIO.cleanup()

if __name__ == '__main__':
    main()
