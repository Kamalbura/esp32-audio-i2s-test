import serial
import sounddevice as sd
import numpy as np
from collections import deque

SERIAL_PORT = 'COM15'  # Replace with your port, e.g., 'COM5' or '/dev/ttyUSB0'
BAUD = 2000000
SAMPLE_RATE = 16000
BUFFER_SIZE = 256

ser = serial.Serial(SERIAL_PORT, BAUD, timeout=1)

def audio_callback(outdata, frames, time, status):
    # Fill outdata with audio from serial
    try:
        data = ser.read(BUFFER_SIZE * 2)
        if len(data) == BUFFER_SIZE * 2:
            samples = np.frombuffer(data, dtype=np.int16)
            outdata[:len(samples), 0] = samples / 32768.0
            if len(samples) < frames:
                outdata[len(samples):, 0] = 0
        else:
            outdata[:, 0] = 0
    except Exception as e:
        outdata[:, 0] = 0

def stream_audio():
    try:
        with sd.OutputStream(
            samplerate=SAMPLE_RATE,
            channels=1,
            dtype='float32',
            blocksize=BUFFER_SIZE,
            callback=audio_callback
        ):
            print("Streaming audio... Press Ctrl+C to stop.")
            while True:
                sd.sleep(1000)
    except Exception as e:
        print("Audio error:", e)

if __name__ == '__main__':
    stream_audio()
