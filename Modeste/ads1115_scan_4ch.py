import time
from smbus import SMBus

ADDR = 0x48
REG_CONVERSION = 0x00
REG_CONFIG     = 0x01

OS_SINGLE    = 0x8000
MODE_SINGLE  = 0x0100
DR_128SPS    = 0x0080
COMP_DISABLE = 0x0003

# MUX single-ended
MUX = {
    "A0": 0x4000,  # 100
    "A1": 0x5000,  # 101
    "A2": 0x6000,  # 110
    "A3": 0x7000,  # 111
}

# gain=1 => ±4.096V
PGA = 0x0200
FSR = 4.096

def read_chan(bus, mux_bits):
    config = OS_SINGLE | mux_bits | PGA | MODE_SINGLE | DR_128SPS | COMP_DISABLE
    bus.write_i2c_block_data(ADDR, REG_CONFIG, [(config >> 8) & 0xFF, config & 0xFF])
    time.sleep(0.01)
    data = bus.read_i2c_block_data(ADDR, REG_CONVERSION, 2)
    raw = (data[0] << 8) | data[1]
    if raw & 0x8000:
        raw -= 1 << 16
    v = raw * (FSR / 32768.0)
    return raw, v

bus = SMBus(1)
try:
    while True:
        out = []
        for name, mux_bits in MUX.items():
            raw, v = read_chan(bus, mux_bits)
            out.append(f"{name}: {raw:6d} {v:0.3f}V")
        print(" | ".join(out))
        time.sleep(0.5)
finally:
    bus.close()