import time
import pigpio

class SDL_Pi_HM3301:
    def __init__(self, pi, sda_pin, scl_pin):
        self.pi = pi
        self.handle = pi.i2c_open(1, 0x40)
        self.sda = sda_pin
        self.scl = scl_pin

    def get_data(self):
        count, data = self.pi.i2c_read_device(self.handle, 29)
        if count != 29:
            print(f"Error: Expected 29 bytes, got {count}")
            return []
        print(f"Data read: {data}")
        return data

    def parse_data(self, data):
        if len(data) < 29:
            print("Error: Data length is less than expected 29 bytes")
            return
        self.PM_1_0_conctrt_std = data[4]<<8 | data[5]
        self.PM_2_5_conctrt_std = data[6]<<8 | data[7]
        self.PM_10_conctrt_std = data[8]<<8 | data[9]
        self.PM_1_0_conctrt_atm = data[10]<<8 | data[11]
        self.PM_2_5_conctrt_atm = data[12]<<8 | data[13]
        self.PM_10_conctrt_atm = data[14]<<8 | data[15]
        self.PM_0_3_cnt = data[16]<<8 | data[17]
        self.PM_0_5_cnt = data[18]<<8 | data[19]
        self.PM_1_0_cnt = data[20]<<8 | data[21]
        self.PM_2_5_cnt = data[22]<<8 | data[23]
        self.PM_5_0_cnt = data[24]<<8 | data[25]
        self.PM_10_cnt = data[26]<<8 | data[27]
        print("Parsed data successfully")

    def close(self):
        self.pi.i2c_close(self.handle)

# Example usage
if __name__ == "__main__":
    pi = pigpio.pi()
    hm3301 = SDL_Pi_HM3301(pi, 20, 19)
    
    try:
        while True:
            data = hm3301.get_data()
            if data:
                hm3301.parse_data(data)
            time.sleep(1)
    except KeyboardInterrupt:
        hm3301.close()
        print("closing hm3301")