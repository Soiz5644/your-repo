import time
import grovepi
import json
import os
import csv
from datetime import datetime

# Set the analog port for the sensor
sensor_port = 1  # A1 on GrovePi+
DELAY = 1.0  # Delay between readings

# File paths
CALIBRATION_FILE = "calibration_data.json"
CSV_FILE = "moisture_readings.csv"

# Variables for calibration
SENSOR_MIN = None
SENSOR_MAX = None
calibrating = True

# Function to save calibration data to a JSON file
def save_calibration(min_val, max_val):
    try:
        calibration_data = {"min": min_val, "max": max_val}
        with open(CALIBRATION_FILE, "w") as file:
            json.dump(calibration_data, file)
        print(f"Calibration saved to {CALIBRATION_FILE}.")
    except Exception as e:
        print(f"Error saving calibration data: {e}")

# Function to load calibration data from a JSON file
def load_calibration():
    if os.path.exists(CALIBRATION_FILE):
        try:
            with open(CALIBRATION_FILE, "r") as file:
                calibration_data = json.load(file)
                print(f"Calibration data loaded: {calibration_data}")
                return calibration_data["min"], calibration_data["max"]
        except (json.JSONDecodeError, KeyError) as e:
            print(f"Error reading calibration file: {e}. Starting fresh.")
            return None, None
    else:
        print("No calibration data found. Starting fresh.")
        return None, None

# Function to write data to CSV file
def write_to_csv(timestamp, value, moisture_percent):
    try:
        file_exists = os.path.exists(CSV_FILE)
        with open(CSV_FILE, mode="a", newline="") as file:
            writer = csv.writer(file)
            if not file_exists:
                writer.writerow(["Time", "Raw value", "Moisture (%)"])  # Write header only if the file doesn't exist
            writer.writerow([timestamp, value, moisture_percent])
        print(f"Data saved to {CSV_FILE}: {timestamp}, {value}, {moisture_percent}%")
    except Exception as e:
        print(f"Error writing to CSV file: {e}")

# Load previously saved calibration values
SENSOR_MIN, SENSOR_MAX = load_calibration()

if SENSOR_MIN is not None and SENSOR_MAX is not None:
    print(f"Calibration data loaded: Min={SENSOR_MIN}, Max={SENSOR_MAX}")
    calibrating = False  # Skip calibration if data exists
else:
    print("No valid calibration data found. Starting calibration.")
    calibrating = True

print("============= Starting sensor readings =============")

while True:
    try:
        # Read the sensor value
        value = grovepi.analogRead(sensor_port)
        print(f"Reading: {value}")

        if calibrating:
            print("Calibration in progress. Place the sensor in different environments.")

            # Set minimum and maximum values during calibration
            if SENSOR_MIN is None or value < SENSOR_MIN:
                SENSOR_MIN = value
            if SENSOR_MAX is None or value > SENSOR_MAX:
                SENSOR_MAX = value

            print(f"Min: {SENSOR_MIN}, Max: {SENSOR_MAX}")
            print("Press Ctrl+C when calibration is done to save.")

        else:
            # Convert the value to a percentage
            if SENSOR_MIN is not None and SENSOR_MAX is not None:
                percent = round(((value - SENSOR_MAX) / (SENSOR_MIN - SENSOR_MAX)) * 100)
                percent = max(0, min(100, percent))  # Clamp between 0 and 100
                print(f"Moisture: {percent}%")

                # Get current time
                timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")

                # Write data to CSV
                write_to_csv(timestamp, value, percent)
            else:
                print("Sensor not calibrated! Run calibration first.")

        time.sleep(DELAY)

    except KeyboardInterrupt:
        # Save calibration data on exit
        if calibrating:
            print("\nCalibration complete. Saving values...")
            save_calibration(SENSOR_MIN, SENSOR_MAX)
            print(f"Calibration saved: Min={SENSOR_MIN}, Max={SENSOR_MAX}")
        else:
            print("\nExiting...")
        break
