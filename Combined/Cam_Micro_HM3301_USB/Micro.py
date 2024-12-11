import sounddevice as sd
import wave

# Function to list audio devices
def list_audio_devices():
    print(sd.query_devices())

# Function to record audio
def record_audio(device_id, duration=5, filename="test_audio.wav"):
    def audio_callback(indata, frames, time, status):
        if status:
            print(f"Status: {status}", flush=True)
        audio_file.writeframes(indata.tobytes())

    try:
        with wave.open(filename, 'wb') as audio_file:
            audio_file.setnchannels(1)  # Set to 1 channel
            audio_file.setsampwidth(2)
            audio_file.setframerate(44100)  # Sample rate for audio
            with sd.InputStream(device=device_id, samplerate=44100, channels=1, callback=audio_callback):
                print("Recording audio...")
                sd.sleep(duration * 1000)  # Record audio for the duration in milliseconds
                print("Finished recording audio")
    except Exception as e:
        print(f"Error recording audio: {e}")

if __name__ == "__main__":
    # List audio devices
    list_audio_devices()

    # Prompt user to enter the device ID
    device_id = int(input("Enter the device ID for the microphone: "))

    # Record audio
    record_audio(device_id)