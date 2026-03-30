import csv
import os
import time

FLUSH_EVERY_N_ROWS = 1

class CSVLogger:
    def __init__(self, filename):
        self.filename = filename
        self.rows = []
        self.row_count = 0
        self.file = None

        # Open the CSV file in append mode
        self.file = open(self.filename, 'a', newline='')
        self.writer = csv.writer(self.file)

        # Write header if file is empty
        if os.stat(self.filename).st_size == 0:
            self.writer.writerow(['Timestamp', 'ADC Value'])  # Adjust header names as needed

    def log(self, adc_value):
        timestamp = time.strftime('%Y-%m-%d %H:%M:%S', time.gmtime())
        self.writer.writerow([timestamp, adc_value])
        self.rows.append((timestamp, adc_value))
        self.row_count += 1
        
        # Flush after every write
        self.file.flush()
        os.fsync(self.file.fileno())

        # Flush every FLUSH_EVERY_N_ROWS
        if self.row_count >= FLUSH_EVERY_N_ROWS:
            self.rows.clear()
            self.row_count = 0  # Reset the row count

    def close(self):
        self.file.close()

try:
    logger = CSVLogger('data.csv')  # Change this to the correct path if necessary
    # Simulate ADC reading logic here
    while True:
        adc_value = read_adc()  # Replace with actual ADC reading logic
        logger.log(adc_value)
except KeyboardInterrupt:
    logger.close()