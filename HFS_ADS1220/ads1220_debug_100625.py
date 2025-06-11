import spidev
import RPi.GPIO as GPIO
import time

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

# Paramètres de conversion
VREF = 2.048  # Volts (supposée référence interne du ADS1220)
GAIN = 1      # Gain (selon configuration du registre)
FSR = (2**23) # Full scale range (24 bits dont 1 bit de signe)

def wait_drdy():
    print("[DEBUG] Attente de DRDY (Data Ready)...")
    cpt = 0
    while GPIO.input(DRDY_PIN):
        time.sleep(0.001)
        cpt += 1
        if cpt % 1000 == 0:
            print(f"[DEBUG] Toujours en attente de DRDY ({cpt} ms)")
    print("[DEBUG] DRDY détecté (conversion terminée)")

def ads1220_write_reg(spi, reg, value):
    print(f"[DEBUG] Ecriture registre {reg} : {bin(value)} / décimal {value}")
    spi.xfer2([CMD_WREG | (reg << 2), 0x00, value])

def ads1220_read_reg(spi, reg):
    resp = spi.xfer2([CMD_RREG | (reg << 2), 0x00, 0x00])
    print(f"[DEBUG] Lecture registre {reg} : {resp}")
    return resp[2]

def ads1220_read_data(spi):
    resp = spi.xfer2([CMD_RDATA, 0x00, 0x00, 0x00])
    print(f"[DEBUG] Trame brute lue (CMD_RDATA + 3 octets) : {resp}")
    # Les 3 derniers octets correspondent à la conversion
    raw = (resp[1] << 16) | (resp[2] << 8) | resp[3]
    print(f"[DEBUG] Valeur brute reconstituée avant signe : {raw}")
    # Convertit en entier signé 24 bits
    if raw & 0x800000:
        raw -= 1 << 24
    print(f"[DEBUG] Valeur brute signée : {raw}")
    return raw

def ads1220_init(spi):
    # Reset du convertisseur
    print("[DEBUG] Reset du convertisseur")
    spi.xfer2([CMD_RESET])
    time.sleep(0.1)

    # Configuration du registre 0 : MUX[3:0]=0000 (A0-A1), gain=2, PGA enabled
    reg0 = 0b00000010
    # Configuration du registre 1 : Data rate=20SPS (000), mode normal, conversion single-shot
    reg1 = 0b00000000
    # Configuration du registre 2 : Par défaut (0x10) : Internal VREF, 50/60Hz rejection
    reg2 = 0b00010000
    # Configuration du registre 3 : Désactive IDACs
    reg3 = 0x00

    ads1220_write_reg(spi, REG_CONF0, reg0)
    ads1220_write_reg(spi, REG_CONF1, reg1)
    ads1220_write_reg(spi, REG_CONF2, reg2)
    ads1220_write_reg(spi, REG_CONF3, reg3)

    # Lecture des registres pour debug
    print("[DEBUG] Lecture des registres après initialisation :")
    val0 = ads1220_read_reg(spi, REG_CONF0)
    val1 = ads1220_read_reg(spi, REG_CONF1)
    val2 = ads1220_read_reg(spi, REG_CONF2)
    val3 = ads1220_read_reg(spi, REG_CONF3)
    print(f"[DEBUG] REG0: {bin(val0)} REG1: {bin(val1)} REG2: {bin(val2)} REG3: {bin(val3)}")
    print(f"[DEBUG] MUX (entrée sélectionnée) : {bin((val0 >> 4) & 0x0F)} (attendu : 0b1011 pour A2-A3)")
    print(f"[DEBUG] GAIN: {1 << ((val0 >> 1) & 0x07)}")
    print(f"[DEBUG] VREF config : {(val2 >> 4) & 0x03} (attendu : 0b01 pour réf interne)")

def raw_to_voltage(raw):
    v = (raw / float(FSR)) * (VREF / GAIN)
    print(f"[DEBUG] Conversion tension : raw={raw}, VREF={VREF}, GAIN={GAIN}, V={v} V")
    return v

def main():
    print("[DEBUG] Initialisation GPIO")
    GPIO.setmode(GPIO.BCM)
    GPIO.setup(DRDY_PIN, GPIO.IN)

    print(f"[DEBUG] Initialisation SPI (bus={SPI_BUS}, device={SPI_DEVICE}, speed={SPI_SPEED_HZ} Hz)")
    spi = spidev.SpiDev()
    spi.open(SPI_BUS, SPI_DEVICE)
    spi.max_speed_hz = SPI_SPEED_HZ
    spi.mode = 1

    ads1220_init(spi)

    try:
        while True:
            print("[DEBUG] Démarrage d'une conversion single-shot")
            spi.xfer2([CMD_START])
            wait_drdy()
            value_raw = ads1220_read_data(spi)
            value_volts = raw_to_voltage(value_raw)
            print(f'[RESULTAT] Valeur brute lue A2-A3 : {value_raw}')
            print(f'[RESULTAT] ==> Tension différentielle mesurée : {value_volts:.6f} V')
            print("--------------------------------------------------")
            time.sleep(1)
    finally:
        print("[DEBUG] Fermeture SPI et nettoyage GPIO")
        spi.close()
        GPIO.cleanup()

if __name__ == '__main__':
    main()