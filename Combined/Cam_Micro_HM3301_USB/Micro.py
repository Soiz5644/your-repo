import pyaudio
import wave

def record_audio(filename, duration, device_id):
    # Set up parameters for recording
    format = pyaudio.paInt16  # Equivalent to S16_LE
    channels = 1  # -c1
    rate = 44100  # --rate 44100
    frames_per_buffer = 1024

    # Initialize PyAudio
    audio = pyaudio.PyAudio()

    # Open the stream
    stream = audio.open(format=format,
                        channels=channels,
                        rate=rate,
                        input=True,
                        input_device_index=device_id,
                        frames_per_buffer=frames_per_buffer)

    print("Recording...")

    frames = []
    for _ in range(0, int(rate / frames_per_buffer * duration)):
        data = stream.read(frames_per_buffer)
        frames.append(data)

    print("Finished recording")

    # Stop and close the stream
    stream.stop_stream()
    stream.close()
    audio.terminate()

    # Save the recording as a WAV file
    with wave.open(filename, 'wb') as wf:
        wf.setnchannels(channels)
        wf.setsampwidth(audio.get_sample_size(format))
        wf.setframerate(rate)
        wf.writeframes(b''.join(frames))

if __name__ == "__main__":
    record_audio("test.wav", duration=5, device_id=1)  # Update device_id if necessary