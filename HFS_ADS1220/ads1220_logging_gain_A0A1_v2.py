import spidev
import time
import csv
import os
from datetime import datetime

CSV_FILENAME = "ads1220_data.csv"

# --- Configuration des registres sous forme lisible ---
MUX_A0A1 = 0b0000 << 4      # MUX[3:0]=0000 (A0-A1)
PGA_ENABLE = 0 << 3         # PGA enabled (bit 3 à 0)
# Le gain sera ajouté dynamiquement (bits 2:0)

REG1 = 0b00000000           # Data rate=20SPS, mode normal, single-shot
REG2 = 0b00010000           # Référence interne, 50/60Hz rejection
REG3 = 0x00                 # IDACs désactivés

def get_timestamp():
    return datetime.now().strftime("%d/%m/%y - %H:%M:%S")

def append_to_csv(filename, row):
    file_exists = os.path.isfile(filename)
    with open(filename, 'a', newline='') as csvfile:
        writer = csv.writer(csvfile, delimiter=';')
        if not file_exists:
            writer.writerow(['Horodatage', 'Valeur_brute', 'Tension_diff_V', 'Gain'])
        writer.writerow(row)

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

def gain_to_regval(gain):
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
    return gain_map[gain]

def configure_ads1220_registers(spi, gain):
    reg0 = MUX_A0A1 | PGA_ENABLE | gain_to_regval(gain)
    spi.xfer2([0x40, 0x03, reg0, REG1, REG2, REG3])
    time.sleep(0.01)
    print(f"Registre 0 actuel : 0b{reg0:08b} (0x{reg0:02X})")

def main():
    spi = spidev.SpiDev()
    spi.open(0, 0)  # bus 0, device 0 (CE0)
    spi.max_speed_hz = 1000000
    spi.mode = 1

    VREF = 2.048

    gain = ask_gain()
    configure_ads1220_registers(spi, gain)
    print(f"Gain sélectionné : {gain}")
    print("Horodatage ; Valeur_brute ; Tension_diff_V ; Gain")

    try:
        while True:
            spi.xfer2([0x08])  # Commande RDATA
            result = spi.xfer2([0x00, 0x00, 0x00])
            raw = (result[0] << 16) | (result[1] << 8) | result[2]
            if raw & 0x800000:
                raw -= 1 << 24
            voltage = raw * VREF / (gain * (1 << 23))
            timestamp = get_timestamp()

            print(f"{timestamp} ; {raw} ; {voltage:.6f} ; {gain}")
            append_to_csv(CSV_FILENAME, [timestamp, raw, f"{voltage:.6f}", gain])
            time.sleep(1)
    except KeyboardInterrupt:
        print("\nArrêt de l'acquisition.")
    finally:
        spi.close()

if __name__ == "__main__":
    main()