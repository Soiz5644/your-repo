import time
import csv
import os
from datetime import datetime
from ads1220_codeberg import ADS1220

# === Paramétrage matériel/fichier ===
SPI_BUS = 0
SPI_DEVICE = 0
DRDY = 25       # GPIO25 (pin 22 du Pi)
CS = None       # CS matériel SPI
VREF = 3.3      # Référence externe 3.3V

# === Choix du gain utilisateur ===
gains = {
    1: "GAIN_1",
    2: "GAIN_2",
    4: "GAIN_4",
    8: "GAIN_8",
    16: "GAIN_16",
    32: "GAIN_32",
    64: "GAIN_64",
    128: "GAIN_128"
}
print("Sélectionne le gain parmi :", list(gains.keys()))
while True:
    try:
        GAIN = int(input("Gain = "))
        if GAIN in gains:
            break
        else:
            print("Valeur incorrecte.")
    except Exception:
        print("Entrée non valide.")

# === Initialisation ADC ===
adc = ADS1220()
# On modifie les attributs de config directement dans l'objet
adc.spi_bus = SPI_BUS
adc.spi_dev = SPI_DEVICE
adc.drdy = DRDY
adc.cs = CS
adc.VREF = VREF

# --- Configuration personnalisée du canal, du gain, etc. ---
def config_ADC_custom(adc, gain):
    # --- Sélectionne AIN2-AIN3, PGA on, gain sélectionné ---
    mux = adc.CFG0_MUX_P2N3
    gain_map = {
        1: adc.CFG0_GAIN_1,
        2: adc.CFG0_GAIN_2,
        4: adc.CFG0_GAIN_4,
        8: adc.CFG0_GAIN_8,
        16: adc.CFG0_GAIN_16,
        32: adc.CFG0_GAIN_32,
        64: adc.CFG0_GAIN_64,
        128: adc.CFG0_GAIN_128
    }
    cfg0 = adc.CFG0_PGA_BYPASS_OFF | gain_map[gain] | mux
    adc.current_gain = gain

    # Mode : continu, Turbo, Data rate max
    cfg1 = adc.CFG1_BCS_OFF | adc.CFG1_TS_OFF | adc.CFG1_CM_CONTINUOUS | adc.CFG1_MODE_TURBO | adc.CFG1_DR_STAGE7
    # VREF externe (3.3 V), pas d'IDAC, pas de FIR
    cfg2 = adc.CFG2_IDAC_OFF | adc.CFG2_PSW_ALWAYS_OPEN | adc.CFG2_FIR_NONE | adc.CFG2_VREF_EXTERNAL
    cfg3 = adc.CFG3_DRDY_DEDICATED | adc.CFG3_I2MUX_DISABLE | adc.CFG3_I1MUX_DISABLE

    adc.write_register(0, cfg0)
    adc.write_register(1, cfg1)
    adc.write_register(2, cfg2)
    adc.write_register(3, cfg3)

config_ADC_custom(adc, GAIN)
print(f"Configuration ADS1220 : AIN2–AIN3, VREF={VREF}V, gain={GAIN}, DRDY=GPIO{DRDY}")

# --- Préparation du fichier CSV ---
date_str = datetime.now().strftime("%Y%m%d_%H%M%S")
csv_filename = f"ADS1220_log_{date_str}.csv"
csv_fields = ["datetime", "raw", "voltage"]

with open(csv_filename, mode='w', newline='') as csvfile:
    csvwriter = csv.writer(csvfile)
    csvwriter.writerow(csv_fields)

    print(f"Enregistrement dans {csv_filename}")
    print("datetime\t\t\tRaw\tVoltage (V)")
    print("-" * 50)
    try:
        while True:
            value = adc.read_data()
            # Conversion brute → tension
            # Formule : (raw / 2^23) * (VREF / gain)
            voltage = (value / 8388607.0) * (VREF / GAIN)
            now = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
            print(f"{now}\t{value}\t{voltage:.6f}")
            csvwriter.writerow([now, value, f"{voltage:.6f}"])
            csvfile.flush()
            time.sleep(1)
    except KeyboardInterrupt:
        print("\nArrêt de la mesure.")