import serial
import numpy as np
import sounddevice as sd
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation

SERIAL_PORT = 'COM15'  # Change as needed
BAUD = 2000000
SAMPLE_RATE = 16000
BUFFER_SIZE = 256

ser = serial.Serial(SERIAL_PORT, BAUD, timeout=1)

# Environment thresholds
CALM_THRESHOLD = 500
NOISY_THRESHOLD = 5000

# Simple IIR low-pass filter (optional, set alpha=0 for no filtering)
def iir_lowpass(samples, alpha=0.0):
    if alpha == 0.0:
        return samples.astype(np.float32)
    filtered = np.zeros_like(samples, dtype=np.float32)
    filtered[0] = samples[0]
    for i in range(1, len(samples)):
        filtered[i] = alpha * samples[i] + (1 - alpha) * filtered[i-1]
    return filtered

def classify_environment(rms, low_energy, high_energy):
    if rms < CALM_THRESHOLD or (low_energy > high_energy * 2):
        return "Calm"
    elif rms > NOISY_THRESHOLD or (high_energy > low_energy * 2):
        return "Noisy"
    else:
        return "Normal"

def calculate_rms(samples):
    return np.sqrt(np.mean(samples.astype(np.float32) ** 2))

def calculate_peak(samples):
    return np.max(np.abs(samples))

def calculate_freq_distribution(samples):
    diff = np.abs(np.diff(samples))
    low_energy = np.sum(diff[diff < 1000])
    high_energy = np.sum(diff[diff >= 1000])
    low_energy /= len(samples)
    high_energy /= len(samples)
    return low_energy, high_energy

# For plotting
history_len = 100
rms_history = [0] * history_len
env_history = ["Normal"] * history_len
waveform = np.zeros(BUFFER_SIZE)

fig, (ax_wave, ax_rms) = plt.subplots(2, 1, figsize=(8, 6))
line_wave, = ax_wave.plot(waveform, label='Waveform')
ax_wave.set_ylim(-32768, 32767)
ax_wave.set_title('Audio Waveform')
ax_wave.legend()

line_rms, = ax_rms.plot(rms_history, label='RMS')
ax_rms.set_ylim(0, 5000)
ax_rms.set_title('RMS History')
ax_rms.legend()
env_text = ax_rms.text(0.7, 0.8, 'Env: Normal', transform=ax_rms.transAxes, fontsize=14, color='black')

# --- Robust audio playback section ---
# Use a ring buffer to decouple serial reading from audio callback

from collections import deque
# Further reduce ring buffer size for lower latency (try 2 blocks)
audio_ring = deque(maxlen=BUFFER_SIZE * 2)  # ~32ms at 16kHz, 256 samples

# --- Advanced audio processing: DC removal + lowpass filter ---
def dc_block(samples, R=0.995):
    y = np.zeros_like(samples, dtype=np.float32)
    prev_y = 0
    prev_x = 0
    for i, x in enumerate(samples):
        y[i] = x - prev_x + R * prev_y
        prev_x = x
        prev_y = y[i]
    return y

def butter_lowpass(samples, cutoff=3000, fs=16000, order=4):
    from scipy.signal import butter, lfilter
    b, a = butter(order, cutoff / (0.5 * fs), btype='low')
    return lfilter(b, a, samples)

def process_audio(samples):
    # Remove DC, then apply lowpass filter for smoothness
    dc_removed = dc_block(samples)
    smoothed = butter_lowpass(dc_removed, cutoff=3000, fs=SAMPLE_RATE, order=4)
    return smoothed

def serial_reader():
    """Continuously read from serial and fill the ring buffer."""
    while True:
        data = ser.read(BUFFER_SIZE * 2)
        if len(data) == BUFFER_SIZE * 2:
            samples = np.frombuffer(data, dtype=np.int16)
            audio_ring.extend(samples)
        else:
            audio_ring.extend([0] * BUFFER_SIZE)

def audio_callback(outdata, frames, time, status):
    try:
        # Use as much as possible from the ring buffer to reduce wait
        block = []
        for i in range(frames):
            if audio_ring:
                block.append(audio_ring.popleft())
            else:
                block.append(0)
        block = np.array(block, dtype=np.int16)
        # Process for smooth playback
        processed = process_audio(block)
        outdata[:, 0] = processed / 32768.0
    except Exception as e:
        outdata[:, 0] = 0

def update_plot(frame):
    global waveform, rms_history, env_history

    # Use a local buffer for plotting, don't consume ring buffer
    if len(audio_ring) >= BUFFER_SIZE:
        samples = np.array(list(audio_ring)[:BUFFER_SIZE], dtype=np.int16)
    else:
        samples = np.zeros(BUFFER_SIZE, dtype=np.int16)

    # Process for analysis (same as playback)
    filtered = process_audio(samples)

    rms = calculate_rms(filtered)
    peak = calculate_peak(filtered)
    low_energy, high_energy = calculate_freq_distribution(filtered)
    env = classify_environment(rms, low_energy, high_energy)

    # Update histories in-place
    rms_history.append(rms)
    del rms_history[0]
    env_history.append(env)
    del env_history[0]

    # Update plots
    line_wave.set_ydata(filtered)
    line_wave.set_xdata(np.arange(len(filtered)))
    ax_wave.set_ylim(filtered.min() - 1000, filtered.max() + 1000)
    line_rms.set_ydata(rms_history)
    line_rms.set_xdata(np.arange(len(rms_history)))
    env_color = {'Calm': 'green', 'Normal': 'blue', 'Noisy': 'red'}[env]
    env_text.set_text(f'Env: {env}')
    env_text.set_color(env_color)

    return line_wave, line_rms, env_text

def main():
    import threading
    # Start serial reader thread
    t = threading.Thread(target=serial_reader, daemon=True)
    t.start()

    try:
        audio_stream = sd.OutputStream(
            samplerate=SAMPLE_RATE,
            channels=1,
            dtype='float32',
            blocksize=BUFFER_SIZE,
            callback=audio_callback
        )
        audio_stream.start()

        ani = FuncAnimation(fig, update_plot, interval=10, blit=False)
        plt.tight_layout()
        plt.show()

        audio_stream.stop()
        audio_stream.close()
    except Exception as e:
        print("Audio error:", e)

if __name__ == '__main__':
    main()
