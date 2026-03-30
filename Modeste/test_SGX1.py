import csv
import os
import time
from datetime import datetime, timezone
from smbus import SMBus

# ====== Configuration utilisateur ======
I2C_BUS = 1
ADS1115_ADDR = 0x48

# Gain (PGA) : ici ±4.096V (gain=1)
PGA_BITS = 0x0200
FSR_VOLTS = 4.096

# Data rate (SPS)
DR_128SPS = 0x0080  # 128 samples/sec

# Période d'échantillonnage (secondes)
SAMPLE_PERIOD_S = 10.0

# Fichier de sortie
CSV_PATH = "/home/pi/SGX/moisture_raw_data.csv"
FLUSH_EVERY_N_ROWS = 10  # flush périodique pour limiter les pertes si coupure courant

# Timestamp en UTC ? (sinon heure locale)
TIMESTAMP_UTC = False
# =======================================

REG_CONVERSION = 0x00
REG_CONFIG = 0x01

OS_SINGLE = 0x8000
MODE_SINGLE = 0x0100
COMP_DISABLE = 0x0003

# MUX single-ended (AINx vs GND)
MUX_SINGLE_ENDED = {
    "A0": 0x4000,  # 100
    "A1": 0x5000,  # 101
    "A2": 0x6000,  # 110
    "A3": 0x7000,  # 111
}

def read_chan(bus: SMBus, mux_bits: int) -> tuple[int, float]:
    """Retourne (raw signé 16-bit, volts) pour un canal single-ended."""
    config = OS_SINGLE | mux_bits | PGA_BITS | MODE_SINGLE | DR_128SPS | COMP_DISABLE

    # Ecriture config (big-endian)
    bus.write_i2c_block_data(ADS1115_ADDR, REG_CONFIG, [(config >> 8) & 0xFF, config & 0xFF])

    # Attendre la conversion: à 128 SPS ~7.8ms; marge
    time.sleep(0.01)

    data = bus.read_i2c_block_data(ADS1115_ADDR, REG_CONVERSION, 2)
    raw = (data[0] << 8) | data[1]
    if raw & 0x8000:
        raw -= 1 << 16

    volts = raw * (FSR_VOLTS / 32768.0)
    return raw, volts

def ensure_csv_header(path: str, header: list[str]) -> bool:
    """Crée le CSV avec header si absent. Retourne True si header écrit."""
    if os.path.exists(path) and os.path.getsize(path) > 0:
        return False
    parent = os.path.dirname(path)
    if parent:
        os.makedirs(parent, exist_ok=True)
    with open(path, "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(header)
    return True

def now_timestamp() -> str:
    if TIMESTAMP_UTC:
        return datetime.now(timezone.utc).isoformat(timespec="seconds")
    return datetime.now().isoformat(timespec="seconds")

def main():
    channels = list(MUX_SINGLE_ENDED.keys())

    header = ["timestamp"]
    for ch in channels:
        header += [f"{ch}_raw", f"{ch}_V"]

    ensure_csv_header(CSV_PATH, header)

    bus = SMBus(I2C_BUS)
    rows_since_flush = 0

    try:
        with open(CSV_PATH, "a", newline="") as f:
            w = csv.writer(f)

            while True:
                ts = now_timestamp()
                row = [ts]

                printable = [ts]
                for ch in channels:
                    raw, v = read_chan(bus, MUX_SINGLE_ENDED[ch])
                    row += [raw, f"{v:.6f}"]
                    printable.append(f"{ch}={v:.3f}V ({raw})")

                w.writerow(row)
                rows_since_flush += 1

                # Affichage console (commente si inutile)
                print(" | ".join(printable))

                if rows_since_flush >= FLUSH_EVERY_N_ROWS:
                    f.flush()
                    os.fsync(f.fileno())
                    rows_since_flush = 0

                time.sleep(SAMPLE_PERIOD_S)

    finally:
        bus.close()


if __name__ == "__main__":
    main()