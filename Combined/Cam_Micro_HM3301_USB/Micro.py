import subprocess

# Define the command and its arguments
command = [
    "arecord",
    "--device=hw:1,0",
    "--format", "S16_LE",
    "--rate", "44100",
    "-c1",
    "test.wav"
]

try:
    print("Recording... Press Ctrl+C to stop.")
    subprocess.run(command, check=True)
except KeyboardInterrupt:
    print("\nRecording interrupted by user.")
except subprocess.CalledProcessError as e:
    print(f"Error occurred while recording: {e}")
except FileNotFoundError:
    print("The 'arecord' utility is not found. Ensure it is installed on your Raspberry Pi.")
finally:
    print("Exiting script.")
