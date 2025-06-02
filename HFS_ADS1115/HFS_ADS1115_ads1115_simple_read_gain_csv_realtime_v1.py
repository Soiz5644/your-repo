import time
import csv
import os
from datetime import datetime

import board
import busio
from adafruit_ads1x15.ads1115 import ADS1115
from adafruit_ads1x15.analog_in import AnalogIn

# === Configuration I2C et ADS1115 ===
I2C_ADDRESS = 0x48  # Adresse par défaut de l'ADS1115
VREF = 4.096        # Référence interne maximale possible de l'ADS1115 (peut être 6.144, 4.096, 2.048, 1.024, 0.512, 0.256 V)

GAIN_SETTINGS = {
    2/3: 6.144,  # ±6.144V (le + courant, mais limité à la tension d’alim du module)
    1:   4.096,
    2:   2.048,
    4:   1.024,
    8:   0.512,
    16:  0.256
}

def ask_gain():
    valid_gains = list(GAIN_SETTINGS.keys())
    print("Veuillez choisir un gain parmi :", valid_gains)
    while True:
        try:
            gain = float(input("Gain désiré (ex: 2/3, 1, 2, 4, 8, 16) : "))
            if gain in valid_gains:
                return gain
            else:
                print("Valeur invalide. Choisissez parmi :", valid_gains)
        except Exception:
            print("Entrée invalide. Veuillez saisir un nombre.")

def main():
    print("=== ADS1115 : sélection du gain PGA et acquisition CSV ===")
    gain = ask_gain()

    # Initialisation I2C et ADC
    i2c = busio.I2C(board.SCL, board.SDA)
    ads = ADS1115(i2c, address=I2C_ADDRESS)
    ads.gain = gain

    channel = AnalogIn(ads, ADS1115.P0, ADS1115.P1)  # Lecture différentielle A0-A1

    filename = f"ads1115_gain_{gain}.csv"
    file_exists = os.path.exists(filename)

    with open(filename, "a", newline='') as csvfile:
        writer = csv.writer(csvfile)
        if not file_exists:
            writer.writerow(["datetime", "voltage_V", "raw", "gain"])
        print(f"Écriture dans {filename} (Ctrl+C pour arrêter)")
        try:
            while True:
                now = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
                raw = channel.value
                voltage = channel.voltage
                writer.writerow([now, f"{voltage:.6f}", raw, gain])
                print(f"{now}\tRaw: {raw}\tVoltage: {voltage:.6f} V")
                csvfile.flush()
                time.sleep(1)
        except KeyboardInterrupt:
            print("\nArrêt de l'acquisition.")

if __name__ == '__main__':
    main()