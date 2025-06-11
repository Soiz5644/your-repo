import spidev
import RPi.GPIO as GPIO
import time
import csv
import os
from datetime import datetime

# Broches du Raspberry Pi connectées au ADS1220
CS_PIN = 8  # Chip Select (GPIO8/CE0)
DRDY_PIN = 17  # Data Ready (GPIO17)

# Configuration SPI
SPI_BUS = 0
SPI_DEVICE = 0
SPI_SPEED_HZ = 1000000  # 1 MHz

# Commandes du ADS1220
CMD_RESET = 0x06
CMD_START = 0x08
CMD_POWERDOWN = 0x02
CMD_RDATA = 0x10
CMD_WREG = 0x40
CMD_RREG = 0x20

# Registres du ADS1220
REG_CONF0 = 0x00
REG_CONF1 = 0x01
REG_CONF2 = 0x02
REG_CONF3 = 0x03

VREF = 2.048  # Volts (supposée référence interne du ADS1220)
FSR = (2**23) # Full scale range (24 bits dont 1 bit de signe)
CSV_FILENAME = "ads1220_data.csv"

def ask_gain():
    print("Choisissez le gain pour l'ADS1220 :")
    print(" 1 : Gain = 1")
    print(" 2 : Gain = 2")
    print(" 4 : Gain = 4")
    print(" 8 : Gain = 8")
    print("16 : Gain = 16")
    print("32 : Gain = 32")
    print("64 : Gain = 64")
    print("128: Gain = 128")
    valid_gains = [1,2,4,8,16,32,64,128]
    while True:
        try:
            g = int(input("Entrer la valeur du gain souhaité : "))
            if g in valid_gains:
                return g
            else:
                print("Valeur non valide. Essayez encore.")
        except Exception:
            print("Entrée non valide. Essayez encore.")

def wait_drdy():
    while GPIO.input(DRDY_PIN):
        time.sleep(0.001)

def ads1220_write_reg(spi, reg, value):
    spi.xfer2([CMD_WREG | (reg << 2), 0x00, value])

def ads1220_read_reg(spi, reg):
    resp = spi.xfer2([CMD_RREG | (reg << 2), 0x00, 0x00])
    return resp[2]

def ads1220_read_data(spi):
    resp = spi.xfer2([CMD_RDATA, 0x00, 0x00, 0x00])
    raw = (resp[1] << 16) | (resp[2] << 8) | resp[3]
    if raw & 0x800000:
        raw -= 1 << 24
    return raw

def ads1220_init(spi, gain, mux_bits):
    # Reset du convertisseur
    spi.xfer2([CMD_RESET])
    time.sleep(0.1)

    # PGA enabled, mux selection, gain
    pga_enable = 0 << 3
    gain_map = {
        1: 0b000,
        2: 0b001,
        4: 0b010,
        8: 0b011,
        16: 0b100,
        32: 0b101,
        64: 0b110,
        128: 0b111
    }
    gain_bits = gain_map[gain]
    reg0 = mux_bits | pga_enable | gain_bits

    reg1 = 0b00000000  # Data rate=20SPS, mode normal, conversion single-shot
    reg2 = 0b00010000  # Internal VREF, 50/60Hz rejection
    reg3 = 0x00        # Désactive IDACs

    ads1220_write_reg(spi, REG_CONF0, reg0)
    ads1220_write_reg(spi, REG_CONF1, reg1)
    ads1220_write_reg(spi, REG_CONF2, reg2)
    ads1220_write_reg(spi, REG_CONF3, reg3)
    # Petite pause pour être sûr que le registre est appliqué
    time.sleep(0.01)

def raw_to_voltage(raw):
    v = (raw / float(FSR)) * (VREF)
    return v

def append_to_csv(filename, row):
    file_exists = os.path.isfile(filename)
    with open(filename, 'a', newline='') as csvfile:
        writer = csv.writer(csvfile, delimiter=';')
        if not file_exists:
            writer.writerow(['Horodatage', 'Valeur_brute', 'Tension_diff_V', 'Gain', 'Entree_diff'])
        writer.writerow(row)

def get_timestamp():
    return datetime.now().strftime("%d/%m/%y - %H:%M:%S")

def main():
    print("[DEBUG] Initialisation GPIO")
    GPIO.setmode(GPIO.BCM)
    GPIO.setup(DRDY_PIN, GPIO.IN)

    gain = ask_gain()

    print(f"[DEBUG] Initialisation SPI (bus={SPI_BUS}, device={SPI_DEVICE}, speed={SPI_SPEED_HZ} Hz)")
    spi = spidev.SpiDev()
    spi.open(SPI_BUS, SPI_DEVICE)
    spi.max_speed_hz = SPI_SPEED_HZ
    spi.mode = 1

    # MUX bits pour A0A1 et A2A3
    mux_a0a1 = 0b0000 << 4  # A0-A1
    mux_a2a3 = 0b1011 << 4  # A2-A3

    try:
        while True:
            # --- Mesure A0A1 ---
            ads1220_init(spi, gain, mux_a0a1)
            spi.xfer2([CMD_START])
            wait_drdy()
            value_raw = ads1220_read_data(spi)
            value_volts = raw_to_voltage(value_raw)
            timestamp = get_timestamp()
            print(f"{timestamp} ; {value_raw} ; {value_volts:.6f} ; {gain} ; A0A1")
            append_to_csv(CSV_FILENAME, [timestamp, value_raw, f"{value_volts:.6f}", gain, "A0A1"])
            time.sleep(0.5)  # Petite pause si besoin

            # --- Mesure A2A3 ---
            ads1220_init(spi, gain, mux_a2a3)
            spi.xfer2([CMD_START])
            wait_drdy()
            value_raw = ads1220_read_data(spi)
            value_volts = raw_to_voltage(value_raw)
            timestamp = get_timestamp()
            print(f"{timestamp} ; {value_raw} ; {value_volts:.6f} ; {gain} ; A2A3")
            append_to_csv(CSV_FILENAME, [timestamp, value_raw, f"{value_volts:.6f}", gain, "A2A3"])
            time.sleep(0.5)  # Pour faire une acquisition par seconde en tout (2x0.1 + 0.8 ≈ 1s)
    finally:
        print("[DEBUG] Fermeture SPI et nettoyage GPIO")
        spi.close()
        GPIO.cleanup()

if __name__ == '__main__':
    main()