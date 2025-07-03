import spidev
import RPi.GPIO as GPIO
import time
import csv
import os
from datetime import datetime

# Broches du Raspberry Pi connectées au ADS1220
CS_PIN = 8  # Chip Select (GPIO8/CE0)
DRDY_PIN = 17  # Data Ready (GPIO17)

# Configuration SPI - optimisée pour robustesse avec l'ADS1220
SPI_BUS = 0
SPI_DEVICE = 0
SPI_SPEED_HZ = 100000  # 100 kHz - vitesse basse pour robustesse

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

# Paramètres de conversion
VREF = 2.048  # Volts (référence interne du ADS1220)
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
    cpt = 0
    while GPIO.input(DRDY_PIN):
        time.sleep(0.001)
        cpt += 1
        if cpt % 1000 == 0:
            print(f"[DEBUG] Toujours en attente de DRDY ({cpt} ms)")

def spi_cs_low():
    """Active le CS (passe à LOW) pour sélectionner l'ADS1220"""
    GPIO.output(CS_PIN, GPIO.LOW)
    time.sleep(0.001)  # Petit délai pour stabilisation

def spi_cs_high():
    """Désactive le CS (passe à HIGH) pour désélectionner l'ADS1220"""
    time.sleep(0.001)  # Petit délai avant de relâcher CS
    GPIO.output(CS_PIN, GPIO.HIGH)

def spi_xfer_with_cs(spi, data):
    """Effectue un transfert SPI avec gestion manuelle du CS"""
    spi_cs_low()
    try:
        result = spi.xfer2(data)
        return result
    finally:
        spi_cs_high()

def ads1220_write_reg(spi, reg, value):
    cmd = [CMD_WREG | (reg << 2), 0x00, value]
    print(f"[SPI DEBUG] Write reg {reg}: {cmd}")
    spi_xfer_with_cs(spi, cmd)

def ads1220_read_reg(spi, reg):
    cmd = [CMD_RREG | (reg << 2), 0x00, 0x00]
    print(f"[SPI DEBUG] Read reg {reg}: {cmd}")
    resp = spi_xfer_with_cs(spi, cmd)
    return resp[2]

def ads1220_read_data(spi):
    resp = spi_xfer_with_cs(spi, [CMD_RDATA, 0x00, 0x00, 0x00])
    raw = (resp[1] << 16) | (resp[2] << 8) | resp[3]
    if raw & 0x800000:
        raw -= 1 << 24
    return raw

def ads1220_init(spi, gain):
    # Reset du convertisseur
    print("[DEBUG] Envoi de la commande RESET...")
    spi_xfer_with_cs(spi, [CMD_RESET])
    time.sleep(1)  # Délai robuste après reset
    
    # Lecture lisible de la séquence des registres (REG0 à REG3) après RESET
    print("\n[DEBUG] === LECTURE DES REGISTRES APRÈS RESET ===")
    for reg in range(4):
        cmd = [CMD_RREG | (reg << 2), 0x00, 0x00]
        resp = spi_xfer_with_cs(spi, cmd)
        reg_value = resp[2]
        print(f"[DEBUG] REG{reg} après RESET : 0x{reg_value:02X} (binaire: {reg_value:08b})")
    print("[DEBUG] ===============================================\n")
    
    # MUX[3:0]=0000 (A0-A1), PGA enabled, gain selon l'utilisateur
    mux_bits = 0b0000 << 4
    pga_enable = 0 << 3
    # GAIN bits (2:0), selon table
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

    print(f"[DEBUG] reg0 calculé : 0b{reg0:08b} (0x{reg0:02X})")

    reg1 = 0b00000000  # Data rate=20SPS, mode normal, conversion single-shot
    reg2 = 0b00010000  # Internal VREF, 50/60Hz rejection
    reg3 = 0x00        # Désactive IDACs

    print("[DEBUG] Écriture des registres de configuration...")
    ads1220_write_reg(spi, REG_CONF0, reg0)
    time.sleep(0.01)  # Délai entre écritures pour robustesse
    ads1220_write_reg(spi, REG_CONF1, reg1)
    time.sleep(0.01)
    ads1220_write_reg(spi, REG_CONF2, reg2)
    time.sleep(0.01)
    ads1220_write_reg(spi, REG_CONF3, reg3)
    time.sleep(0.01)

    # Lecture et vérification de tous les registres après configuration
    print("\n[DEBUG] === VÉRIFICATION DES REGISTRES APRÈS CONFIGURATION ===")
    val0 = ads1220_read_reg(spi, REG_CONF0)
    val1 = ads1220_read_reg(spi, REG_CONF1)
    val2 = ads1220_read_reg(spi, REG_CONF2)
    val3 = ads1220_read_reg(spi, REG_CONF3)
    print(f"[DEBUG] REG0: 0x{val0:02X} (binaire: {val0:08b}) - MUX + GAIN")
    print(f"[DEBUG] REG1: 0x{val1:02X} (binaire: {val1:08b}) - DATA RATE + MODE")
    print(f"[DEBUG] REG2: 0x{val2:02X} (binaire: {val2:08b}) - VREF + FILTRES")
    print(f"[DEBUG] REG3: 0x{val3:02X} (binaire: {val3:08b}) - IDAC CONFIG")
    print(f"[DEBUG] Configuration: Gain={gain}, MUX=A0-A1, VREF=Interne")
    print("[DEBUG] ========================================================\n")

def raw_to_voltage(raw):
    v = (raw / float(FSR)) * (VREF)
    return v

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
    
    # Configuration du CS en sortie pour gestion manuelle
    print(f"[DEBUG] Configuration CS pin {CS_PIN} en sortie")
    GPIO.setup(CS_PIN, GPIO.OUT)
    GPIO.output(CS_PIN, GPIO.HIGH)  # CS inactif au démarrage

    gain = ask_gain()

    print(f"[DEBUG] Initialisation SPI (bus={SPI_BUS}, device={SPI_DEVICE}, speed={SPI_SPEED_HZ} Hz)")
    spi = spidev.SpiDev()
    spi.open(SPI_BUS, SPI_DEVICE)
    spi.max_speed_hz = SPI_SPEED_HZ
    spi.mode = 1  # Mode SPI 1 (CPOL=0, CPHA=1) pour ADS1220
    
    # Désactiver la gestion automatique du CS par spidev
    print("[DEBUG] Désactivation de la gestion automatique du CS (spi.no_cs = True)")
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
        # S'assurer que CS est inactif avant de fermer
        GPIO.output(CS_PIN, GPIO.HIGH)
        spi.close()
        GPIO.cleanup()

if __name__ == '__main__':
    main()