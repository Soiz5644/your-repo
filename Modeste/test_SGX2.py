import csv
import os
import time
from datetime import datetime, timezone
from smbus import SMBus

# ====== Configuration ======
I2C_BUS = 1
ADS1115_ADDR = 0x48

# Gain (PGA): ±4.096 V (gain=1)
PGA_BITS = 0x0200
FSR_VOLTS = 4.096

# Data rate: 128 SPS
DR_128SPS = 0x0080

SAMPLE_PERIOD_S = 1.0

CSV_PATH = "/home/pi/SGX/moisture_raw.csv"

# Pour debug/fiabilité maximale: flush à chaque ligne
FLUSH_EVERY_N_ROWS = 1

# Timestamp UTC (sinon heure locale)
TIMESTAMP_UTC = False
# ===========================

REG_CONVERSION = 0x00
REG_CONFIG = 0x01

OS_SINGLE = 0x8000
MODE_SINGLE = 0x0100
COMP_DISABLE = 0x0003

MUX_SINGLE_ENDED = {
    "A0": 0x4000,
    "A1": 0x5000,
    "A2": 0x6000,
    "A3": 0x7000,
}

def now_timestamp() -> str:
    if TIMESTAMP_UTC:
        return datetime.now(timezone.utc).isoformat(timespec="seconds")
    return datetime.now().isoformat(timespec="seconds")

def ensure_csv_header(path: str, header: list[str]) -> None:
    parent = os.path.dirname(path)
    if parent:
        os.makedirs(parent, exist_ok=True)

    # a+ : append + lecture possible. Header seulement si fichier vide.
    with open(path, "a+", newline="") as f:
        f.seek(0, os.SEEK_END)
        if f.tell() == 0:
            csv.writer(f).writerow(header)
            f.flush()
            os.fsync(f.fileno())

def read_chan(bus: SMBus, mux_bits: int) -> tuple[int, float]:
    config = OS_SINGLE | mux_bits | PGA_BITS | MODE_SINGLE | DR_128SPS | COMP_DISABLE
    bus.write_i2c_block_data(ADS1115_ADDR, REG_CONFIG, [(config >> 8) & 0xFF, config & 0xFF])

    time.sleep(0.01)

    data = bus.read_i2c_block_data(ADS1115_ADDR, REG_CONVERSION, 2)
    raw = (data[0] << 8) | data[1]
    if raw & 0x8000:
        raw -= 1 << 16

    volts = raw * (FSR_VOLTS / 32768.0)
    return raw, volts

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

            try:
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

                    print(" | ".join(printable))

                    if rows_since_flush >= FLUSH_EVERY_N_ROWS:
                        f.flush()
                        os.fsync(f.fileno())
                        rows_since_flush = 0

                    time.sleep(SAMPLE_PERIOD_S)

            except KeyboardInterrupt:
                print("Arrêt demandé (Ctrl+C). Données flushées sur disque.")

    finally:
        bus.close()

if __name__ == "__main__":
    main()