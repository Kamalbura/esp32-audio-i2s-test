"""
ESP32 Microphone Audio Visualizer and Playback

This script receives audio data from the ESP32 over serial,
visualizes it in real-time, and can play it back through the computer speakers.

Requirements:
- Python 3.6 or later
- pip install pyserial matplotlib numpy sounddevice
"""

import serial
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
import sounddevice as sd
import argparse
import queue
import threading
import time

# Constants
HEADER_MAGIC = 0xAA55
SAMPLE_RATE = 16000
FRAME_SIZE = 256  # Should match STREAM_PACKET_SIZE in Arduino code
BUFFER_SIZE = 10  # Number of frames to buffer for playback

# Command line arguments
parser = argparse.ArgumentParser(description="ESP32 Audio Visualizer")
parser.add_argument('--port', type=str, required=True, help='Serial port (e.g., COM3 or /dev/ttyUSB0)')
parser.add_argument('--baud', type=int, default=115200, help='Baud rate')
parser.add_argument('--play', action='store_true', help='Enable audio playback')
args = parser.parse_args()

# Set up serial connection
try:
    ser = serial.Serial(args.port, args.baud, timeout=1)
    print(f"Connected to {args.port} at {args.baud} baud")
except Exception as e:
    print(f"Error opening serial port: {e}")
    exit(1)

# Audio buffer for playback
audio_queue = queue.Queue(maxsize=BUFFER_SIZE)
play_audio = args.play

# Set up the figure for visualization
fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 8))
fig.suptitle('ESP32 Microphone Audio Visualization')

# Time domain plot
line1, = ax1.plot(np.zeros(FRAME_SIZE), 'b-')
ax1.set_ylim(-32768, 32767)  # 16-bit audio range
ax1.set_xlim(0, FRAME_SIZE)
ax1.set_ylabel('Amplitude')
ax1.set_xlabel('Sample')
ax1.set_title('Time Domain')
ax1.grid(True)

# Frequency domain plot
line2, = ax2.plot(np.zeros(FRAME_SIZE//2), 'r-')
ax2.set_ylim(0, 5000)  # FFT magnitude range, may need adjustment
ax2.set_xlim(0, FRAME_SIZE//2)
ax2.set_ylabel('Magnitude')
ax2.set_xlabel('Frequency Bin')
ax2.set_title('Frequency Domain')
ax2.grid(True)

# Statistics display
stats_text = ax1.text(0.02, 0.95, '', transform=ax1.transAxes, fontsize=10,
                    verticalalignment='top', bbox=dict(boxstyle='round', facecolor='wheat', alpha=0.5))

# For calculating FPS
frame_count = 0
last_time = time.time()

def read_audio_frame():
    """Read audio data from the serial port, looking for our magic header"""
    # Look for the magic header
    header_found = False
    while not header_found:
        if ser.in_waiting < 4:  # Need at least 4 bytes for header and size
            return None
        
        # Read first byte, if not the start of magic, continue
        b1 = ser.read(1)[0]
        if b1 != (HEADER_MAGIC >> 8) & 0xFF:
            continue
            
        # Read second byte, check if it completes the magic header
        b2 = ser.read(1)[0]
        if b2 != HEADER_MAGIC & 0xFF:
            continue
            
        # Magic header found
        header_found = True
    
    # Read the packet size (2 bytes)
    size_bytes = ser.read(2)
    packet_size = (size_bytes[0] << 8) | size_bytes[1]
    
    # Ensure we have enough data
    while ser.in_waiting < packet_size * 2:  # 2 bytes per sample
        time.sleep(0.001)
    
    # Read the sample data (2 bytes per sample, signed 16-bit)
    data = ser.read(packet_size * 2)
    samples = np.frombuffer(data, dtype=np.int16)
    
    return samples

def audio_callback(outdata, frames, time_info, status):
    """Callback function for audio playback"""
    if status:
        print(status)
    
    try:
        data = audio_queue.get_nowait()
        # Ensure data size matches expected output size
        if len(data) < frames:
            outdata[:len(data)] = data.reshape(-1, 1)
            outdata[len(data):] = 0
        else:
            outdata[:] = data[:frames].reshape(-1, 1)
    except queue.Empty:
        print("Buffer underrun")
        outdata.fill(0)
        return

def update_plot(frame):
    """Update function for animation"""
    global frame_count, last_time
    
    # Read audio data
    samples = read_audio_frame()
    if samples is None:
        return line1, line2, stats_text
    
    # Calculate FFT for frequency display
    fft_data = np.abs(np.fft.rfft(samples))
    
    # Update time domain plot
    line1.set_ydata(samples)
    
    # Update frequency domain plot
    line2.set_ydata(fft_data[:FRAME_SIZE//2])
    
    # Calculate statistics
    rms = np.sqrt(np.mean(samples**2))
    peak = np.max(np.abs(samples))
    
    # Calculate FPS
    frame_count += 1
    now = time.time()
    if now - last_time >= 1.0:
        fps = frame_count / (now - last_time)
        frame_count = 0
        last_time = now
    else:
        fps = frame_count / max(0.001, now - last_time)
    
    # Update statistics text
    stats_text.set_text(f'RMS: {rms:.1f}\nPeak: {peak:.1f}\nFPS: {fps:.1f}')
    
    # Add to audio queue if playback is enabled
    if play_audio:
        try:
            audio_queue.put_nowait(samples / 32768.0)  # Normalize to -1.0 to 1.0 range
        except queue.Full:
            pass  # Skip this frame if the queue is full
    
    return line1, line2, stats_text

def start_audio_stream():
    """Start the audio playback stream"""
    stream = sd.OutputStream(
        samplerate=SAMPLE_RATE,
        channels=1,
        callback=audio_callback,
        blocksize=FRAME_SIZE
    )
    stream.start()
    return stream

# Main function
def main():
    # First, send the 's' command to ESP32 to start streaming
    print("Sending 's' command to start streaming...")
    ser.write(b's')
    time.sleep(1)  # Give ESP32 time to start streaming
    
    # Start audio playback if enabled
    if play_audio:
        print("Starting audio playback...")
        audio_stream = start_audio_stream()
    else:
        audio_stream = None
    
    # Start the animation
    print("Starting visualization...")
    ani = FuncAnimation(fig, update_plot, blit=True, interval=10, cache_frame_data=False)
    plt.tight_layout()
    
    try:
        plt.show()
    except KeyboardInterrupt:
        print("Interrupted by user")
    finally:
        # Clean up
        if audio_stream:
            audio_stream.stop()
        
        # Send 's' command again to stop streaming
        ser.write(b's')
        ser.close()
        print("Serial connection closed")

if __name__ == "__main__":
    main()
