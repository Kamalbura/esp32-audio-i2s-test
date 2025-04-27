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

---

## 🛠️ Hardware You Need

- **ESP32 board** (any dev board with I2S support)
- **I2S microphone** (e.g., INMP441)
- **USB cable** (for ESP32-PC connection)
- **Jumper wires** (for connections)

---

## 🔌 How to Connect the INMP441 I2S Mic to ESP32

| INMP441 Pin | ESP32 Pin Example |
|-------------|------------------|
| VCC         | 3.3V             |
| GND         | GND              |
| WS (LRCL)   | GPIO 25          |
| SD          | GPIO 32          |
| SCK (BCLK)  | GPIO 33          |

**Check your ESP32 code for the exact pin mapping!**  
Double-check wiring before powering up.

---

## 💻 Software Requirements

- **Python 3.8+**
- [sounddevice](https://python-sounddevice.readthedocs.io/)
- [numpy](https://numpy.org/)
- [matplotlib](https://matplotlib.org/)
- [scipy](https://scipy.org/)
- [pyserial](https://pyserial.readthedocs.io/)

Install all dependencies with:
```bash
pip install sounddevice numpy matplotlib scipy pyserial
```

---

## 🚀 Quick Start Guide

### 1. Flash the ESP32

- Upload the Arduino sketch (`ESP32_Mic_Analyzer.ino`) to your ESP32 using Arduino IDE or PlatformIO.
- This sketch reads audio from the I2S mic and streams it over USB serial.

### 2. Connect the Microphone

- Wire your INMP441 to the ESP32 as shown above.
- Double-check connections for VCC, GND, WS, SD, and SCK.

### 3. Run the Python Scripts

- **Edit the serial port** in the Python scripts to match your ESP32's port (e.g., `COM5` on Windows or `/dev/ttyUSB0` on Linux/Mac).
- For **audio analysis and visualization**, run:
  ```bash
  python serial_audio_analyze.py
  ```
- For **direct audio playback**, run:
  ```bash
  python serial_audio_playback.py
  ```

---

## 📂 File Descriptions

| File | Description |
|------|-------------|
| [`ESP32_Mic_Analyzer.ino`](./ESP32_Mic_Analyzer.ino) | Arduino sketch for ESP32. Reads audio from I2S mic and streams raw PCM data over serial. **Flash this to your ESP32.** |
| [`serial_audio_analyze.py`](./serial_audio_analyze.py) | Python script for real-time audio analysis, visualization, environment classification, and playback. **Run this on your PC.** |
| [`serial_audio_playback.py`](./serial_audio_playback.py) | Python script for low-latency audio playback from ESP32 serial stream. **Run this for simple listening.** |
| [`README.md`](./README.md) | This guide. Explains setup, usage, and file roles. |

---

## 🧑‍💻 How It Works

1. **ESP32** reads audio from the I2S mic and sends it as 16-bit PCM samples over USB serial.
2. **Python scripts** on your PC read the serial data, process it, and either:
   - Visualize and analyze the audio (`serial_audio_analyze.py`)
   - Play the audio directly (`serial_audio_playback.py`)
3. **Noise reduction** and **environment classification** are performed in real time.

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

---

## 📚 Learn More

- [ESP32 Arduino core](https://github.com/espressif/arduino-esp32)
- [INMP441 datasheet](https://www.analog.com/media/en/technical-documentation/data-sheets/inmp441.pdf)
- [sounddevice docs](https://python-sounddevice.readthedocs.io/)

---

## 📝 License

MIT License

---

## 🙌 Contributing

Pull requests and suggestions are welcome!  
If you build something cool with this, share it on GitHub and let the community know!

---
