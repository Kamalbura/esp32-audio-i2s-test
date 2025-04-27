# ESP32 I2S Microphone Audio Analyzer & Playback

Welcome! This project helps you stream, analyze, and play back real-time audio from an **I2S microphone** (like the INMP441) connected to an **ESP32**.  
It's designed for **beginners**—no advanced electronics or programming knowledge required.

---

## 📦 What This Repo Does

- **Streams high-quality audio** from an I2S mic on ESP32 to your PC via USB serial.
- **Visualizes and analyzes** the audio in real time using Python.
- **Plays back** the audio on your PC with low latency.
- **Classifies environments** (e.g., Calm, Noisy, Speech).
- **Reduces noise** for clearer playback.
- **Bluetooth & DumbDisplay:** Analyze audio wirelessly on your phone.
- **Web interfaces:** Test and visualize mic input in your browser (simple and advanced).

---

## 📱 Alternative Interfaces & Features

### DumbDisplay & Bluetooth Audio Analysis

- **DumbDisplay**: Use the [DumbDisplay app](https://play.google.com/store/apps/details?id=io.github.astashov.dumbdisplay) with Bluetooth or USB OTG to visualize and analyze audio from your ESP32 mic directly on your phone.  
  The `dd-working.ino` sketch streams audio and provides a touch-friendly UI for mic, record, and playback, including amplification controls.

### 🌐 Web Interfaces for Mic Testing

- **Simple Web Interface** (`mic-webserver-done-simple.ino`):  
  Displays a live waveform of the microphone input using Chart.js. Great for quick checks and basic visualization.
- **Advanced Web Interface** (`waveform_mic-web-stream.ino`):  
  Shows waveform history, enables live mic playback in your browser, and provides real-time environment classification (Calm, Normal, Noisy).

### 🛠️ Serial and Python Tools

- **Serial Test (`serial-test.ino`)**:  
  Tests the I2S mic using the Arduino Serial Plotter for quick hardware verification.
- **Audio Visualizer (`audio_visualizer.py` or `serial_audio_analyze.py`)**:  
  Visualizes and analyzes audio from the ESP32 using Python, with environment classification.
- **Audio Playback (`serial_audio_playback.py`)**:  
  Streams high-quality audio from the ESP32 to your PC speakers or headphones.

---

## 📂 File Descriptions

| File/Folder | Description |
|-------------|-------------|
| [`ESP32_Mic_Analyzer/ESP32_Mic_Analyzer.ino`](./ESP32_Mic_Analyzer/ESP32_Mic_Analyzer.ino) | Main Arduino sketch for ESP32. Streams audio from I2S mic over serial. |
| [`serial_audio_analyze.py`](./serial_audio_analyze.py) | Python script for real-time audio analysis, visualization, and playback. |
| [`serial_audio_playback.py`](./serial_audio_playback.py) | Python script for high-quality audio playback from ESP32 serial stream. |
| [`audio_visualizer.py`](./audio_visualizer.py) | Alternative Python visualizer for serial audio (with header support). |
| [`serial-test/serial-test.ino`](./serial-test/serial-test.ino) | Arduino sketch for testing the mic using the Serial Plotter. |
| [`mic-webserver-done-simple/mic-webserver-done-simple.ino`](./mic-webserver-done-simple/mic-webserver-done-simple.ino) | Simple web interface for live waveform display (uses Chart.js). |
| [`waveform_mic-web-stream/waveform_mic-web-stream.ino`](./waveform_mic-web-stream/waveform_mic-web-stream.ino) | Advanced web interface with waveform history, live playback, and environment classification. |
| [`dd-working/dd-working.ino`](./dd-working/dd-working.ino) | DumbDisplay/Bluetooth/USB interface for phone-based audio visualization and control. |
| [`SpeakerTest/SpeakerTest.ino`](./SpeakerTest/SpeakerTest.ino) | Test tones and mic-to-speaker loopback for I2S speaker/amplifier. |
| [`README.md`](./README.md) | This guide. Explains setup, usage, and file roles. |

---

## 🧑‍💻 How It Works

1. **ESP32** reads audio from the I2S mic and sends it as 16-bit PCM samples over USB serial, Bluetooth, or WiFi.
2. **Python scripts** on your PC read the serial data, process it, and either:
   - Visualize and analyze the audio (`serial_audio_analyze.py`, `audio_visualizer.py`)
   - Play the audio directly (`serial_audio_playback.py`)
3. **Web interfaces** let you view live waveforms and environment classification in your browser.
4. **DumbDisplay** lets you analyze and control audio wirelessly on your phone.
5. **Noise reduction** and **environment classification** are performed in real time.

---

## 📝 Tips for Beginners

- **Serial Port:** Find your ESP32's port using Arduino IDE or Device Manager (Windows) / `ls /dev/tty*` (Linux/Mac).
- **Buffer Size & Baud Rate:** If you hear glitches, try lowering the baud rate or buffer size in the Python scripts.
- **Mic Placement:** Keep the mic away from noise sources for best results.
- **Troubleshooting:** If you get serial errors, make sure no other program is using the port.

---

## 🧩 Advanced Features

- **Noise Reduction:** DC removal, lowpass filtering, and spectral gating for clean audio.
- **Environment Classification:** Detects if the environment is calm, noisy, or contains speech.
- **Low Latency:** Small buffers for near real-time feedback.
- **Bluetooth/Phone Visualization:** Use DumbDisplay for wireless, touch-friendly audio analysis.

---

## 📚 Learn More

- [ESP32 Arduino core](https://github.com/espressif/arduino-esp32)
- [INMP441 datasheet](https://www.analog.com/media/en/technical-documentation/data-sheets/inmp441.pdf)
- [sounddevice docs](https://python-sounddevice.readthedocs.io/)
- [DumbDisplay app](https://play.google.com/store/apps/details?id=io.github.astashov.dumbdisplay)

---

## 📝 License

MIT License

---

## 🙌 Contributing

Pull requests and suggestions are welcome!  
If you build something cool with this, share it on GitHub and let the community know!

---

**Repo:** [esp32-audio-i2s-test](https://github.com/Kamalbura/esp32-audio-i2s-test)

---

## 🛠️ Hardware Setup & Pinouts

### Required Hardware

- **ESP32 board** (any dev board with I2S support)
- **I2S microphone** (e.g., INMP441)
- **(Optional) I2S speaker/amplifier** (for loopback/speaker tests)
- **USB cable** (for ESP32-PC connection)
- **Jumper wires** (for connections)
- **Breadboard** (recommended for prototyping)

### INMP441 to ESP32 Pin Mapping

| INMP441 Pin | ESP32 Pin Example | Description         |
|-------------|------------------|---------------------|
| VCC         | 3.3V             | Power (do NOT use 5V) |
| GND         | GND              | Ground              |
| WS (LRCL)   | GPIO 25          | Word Select (L/R Clock) |
| SD          | GPIO 33          | Serial Data (DOUT)  |
| SCK (BCLK)  | GPIO 32          | Bit Clock           |

**Note:**  
- Double-check your ESP32 sketch for the exact pin mapping. Some sketches (like `SpeakerTest.ino` or `serial-test.ino`) may use different pins (see comments in each file).
- Always connect VCC to 3.3V, not 5V, to avoid damaging the mic.

### Typical ESP32 ↔ INMP441 Wiring

```
INMP441   ESP32
-------   -----
VCC    →  3.3V
GND    →  GND
WS     →  GPIO 25
SD     →  GPIO 33
SCK    →  GPIO 32
```

### Initial Hardware Checklist

- [ ] Connect the INMP441 mic to the ESP32 as shown above.
- [ ] Use short, shielded wires for best audio quality.
- [ ] Power the ESP32 via USB from your PC.
- [ ] Double-check all connections before powering up.
- [ ] (Optional) Connect an I2S speaker for playback tests (see `SpeakerTest.ino`).
